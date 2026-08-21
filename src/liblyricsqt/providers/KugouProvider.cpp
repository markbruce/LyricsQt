#include <lyricsqt/KugouProvider.h>

#include "ProviderHelpers.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace lyricsqt {

namespace {

constexpr char kProviderId[] = "kugou";
constexpr char kUserAgent[] =
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
    "AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.4 Safari/605.1.15";
// Prefer HTTPS; some networks redirect HTTP to a block page.
constexpr char kSearchUrl[] = "https://lyrics.kugou.com/search";
constexpr char kDownloadUrl[] = "https://lyrics.kugou.com/download";

QString searchKeyword(const LyricsSearchRequest &req)
{
    if (!req.artist.isEmpty() && !req.title.isEmpty()) {
        return req.title + QLatin1Char(' ') + req.artist;
    }
    return !req.title.isEmpty() ? req.title : req.artist;
}

} // namespace

KugouProvider::KugouProvider(QObject *parent)
    : ILyricsProvider(parent)
{
}

QString KugouProvider::id() const
{
    return QString::fromLatin1(kProviderId);
}

void KugouProvider::cancel()
{
    abortActive();
    m_generation = 0;
    m_hits.clear();
    m_hitIndex = 0;
}

void KugouProvider::abortActive()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void KugouProvider::search(const LyricsSearchRequest &req)
{
    abortActive();
    m_generation = req.generation;
    m_pending = req;
    m_hits.clear();
    m_hitIndex = 0;

    if (req.title.isEmpty() && req.artist.isEmpty()) {
        emit searchFailed(id(), m_generation, QStringLiteral("empty title and artist"));
        return;
    }

    startSearch(req);
}

void KugouProvider::startSearch(const LyricsSearchRequest &req)
{
    QUrl url(QString::fromLatin1(kSearchUrl));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("keyword"), searchKeyword(req));
    const int durationMs = req.durationSec > 0.0 ? qRound(req.durationSec * 1000.0) : 0;
    query.addQueryItem(QStringLiteral("duration"), QString::number(durationMs));
    query.addQueryItem(QStringLiteral("client"), QStringLiteral("pc"));
    query.addQueryItem(QStringLiteral("ver"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("man"), QStringLiteral("yes"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));

    m_reply = m_nam.get(request);
    connect(m_reply, &QNetworkReply::finished, this, &KugouProvider::handleSearchFinished);
}

void KugouProvider::startDownload(const SongHit &song)
{
    QUrl url(QString::fromLatin1(kDownloadUrl));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("id"), song.id);
    query.addQueryItem(QStringLiteral("accesskey"), song.accesskey);
    // Prefer plain LRC to avoid KRC decrypt complexity.
    query.addQueryItem(QStringLiteral("fmt"), QStringLiteral("lrc"));
    query.addQueryItem(QStringLiteral("charset"), QStringLiteral("utf8"));
    query.addQueryItem(QStringLiteral("client"), QStringLiteral("pc"));
    query.addQueryItem(QStringLiteral("ver"), QStringLiteral("1"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));

    m_reply = m_nam.get(request);
    connect(m_reply, &QNetworkReply::finished, this, &KugouProvider::handleDownloadFinished);
}

QVector<KugouProvider::SongHit> KugouProvider::parseSearchHits(const QJsonObject &root) const
{
    QVector<SongHit> out;
    const QJsonArray candidates = root.value(QStringLiteral("candidates")).toArray();
    out.reserve(candidates.size());
    for (const QJsonValue &value : candidates) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject obj = value.toObject();
        SongHit hit;
        hit.id = obj.value(QStringLiteral("id")).toVariant().toString();
        hit.accesskey = obj.value(QStringLiteral("accesskey")).toString();
        hit.song = obj.value(QStringLiteral("song")).toString();
        hit.singer = obj.value(QStringLiteral("singer")).toString();
        hit.durationMs = obj.value(QStringLiteral("duration")).toInt();
        if (!hit.id.isEmpty() && !hit.accesskey.isEmpty()) {
            out.append(hit);
        }
    }
    return out;
}

LyricsDocument KugouProvider::documentFromDownloadJson(const QJsonObject &root, const SongHit &song) const
{
    QString content = root.value(QStringLiteral("content")).toString();
    if (content.isEmpty()) {
        return {};
    }

    // Download payload is typically base64-encoded LRC text.
    if (!content.contains(QLatin1Char('['))) {
        const QString decoded = provider_helpers::decodeBase64Utf8(content);
        if (!decoded.isEmpty()) {
            content = decoded;
        }
    }

    LyricsDocument doc = provider_helpers::parseLrc(content, id());
    if (doc.title.isEmpty()) {
        doc.title = song.song;
    }
    if (doc.artist.isEmpty()) {
        doc.artist = song.singer;
    }
    return doc;
}

void KugouProvider::handleSearchFinished()
{
    QNetworkReply *reply = m_reply;
    if (!reply) {
        return;
    }
    m_reply = nullptr;
    reply->deleteLater();

    const quint64 gen = m_generation;
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        emit searchFailed(id(), gen, reply->errorString());
        return;
    }

    const QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
    if (!json.isObject()) {
        emit searchFailed(id(), gen, QStringLiteral("invalid search response"));
        return;
    }

    m_hits = parseSearchHits(json.object());
    if (m_hits.isEmpty()) {
        emit searchFailed(id(), gen, QStringLiteral("no results"));
        return;
    }

    m_hitIndex = 0;
    startDownload(m_hits.at(m_hitIndex));
}

void KugouProvider::handleDownloadFinished()
{
    QNetworkReply *reply = m_reply;
    if (!reply) {
        return;
    }
    m_reply = nullptr;
    reply->deleteLater();

    const quint64 gen = m_generation;
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        return;
    }

    auto tryNext = [this, gen](const QString &reason) {
        ++m_hitIndex;
        if (m_hitIndex < m_hits.size()) {
            startDownload(m_hits.at(m_hitIndex));
            return;
        }
        emit searchFailed(id(), gen, reason);
    };

    if (reply->error() != QNetworkReply::NoError) {
        tryNext(reply->errorString());
        return;
    }

    const QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
    if (!json.isObject() || m_hitIndex < 0 || m_hitIndex >= m_hits.size()) {
        tryNext(QStringLiteral("invalid download response"));
        return;
    }

    LyricsDocument doc = documentFromDownloadJson(json.object(), m_hits.at(m_hitIndex));
    if (doc.lines.isEmpty()) {
        tryNext(QStringLiteral("empty lyrics"));
        return;
    }

    emit resultsReady(id(), gen, {doc});
}

} // namespace lyricsqt
