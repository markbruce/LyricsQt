#include <lyricsqt/QQMusicProvider.h>

#include "ProviderHelpers.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace lyricsqt {

namespace {

constexpr char kProviderId[] = "qq";
constexpr char kUserAgent[] =
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
    "AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.4 Safari/605.1.15";
constexpr char kSearchUrl[] = "https://c.y.qq.com/soso/fcgi-bin/client_search_cp";
constexpr char kLyricUrl[] = "https://c.y.qq.com/lyric/fcgi-bin/fcg_query_lyric_new.fcg";
constexpr char kReferer[] = "https://y.qq.com/portal/player.html";

QString searchKeyword(const LyricsSearchRequest &req)
{
    if (!req.artist.isEmpty() && !req.title.isEmpty()) {
        return req.title + QLatin1Char(' ') + req.artist;
    }
    return !req.title.isEmpty() ? req.title : req.artist;
}

} // namespace

QQMusicProvider::QQMusicProvider(QObject *parent)
    : ILyricsProvider(parent)
{
}

QString QQMusicProvider::id() const
{
    return QString::fromLatin1(kProviderId);
}

void QQMusicProvider::cancel()
{
    abortActive();
    m_generation = 0;
    m_hits.clear();
    m_hitIndex = 0;
}

void QQMusicProvider::abortActive()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void QQMusicProvider::search(const LyricsSearchRequest &req)
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

void QQMusicProvider::startSearch(const LyricsSearchRequest &req)
{
    QUrl url(QString::fromLatin1(kSearchUrl));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("w"), searchKeyword(req));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    query.addQueryItem(QStringLiteral("p"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("n"), QStringLiteral("8"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));
    request.setRawHeader("Referer", QByteArray(kReferer));

    m_reply = m_nam.get(request);
    connect(m_reply, &QNetworkReply::finished, this, &QQMusicProvider::handleSearchFinished);
}

void QQMusicProvider::startLyric(const SongHit &song)
{
    QUrl url(QString::fromLatin1(kLyricUrl));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("songmid"), song.songmid);
    query.addQueryItem(QStringLiteral("g_tk"), QStringLiteral("5381"));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    query.addQueryItem(QStringLiteral("nobase64"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("inCharset"), QStringLiteral("utf8"));
    query.addQueryItem(QStringLiteral("outCharset"), QStringLiteral("utf-8"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));
    request.setRawHeader("Referer", QByteArray(kReferer));

    m_reply = m_nam.get(request);
    connect(m_reply, &QNetworkReply::finished, this, &QQMusicProvider::handleLyricFinished);
}

QVector<QQMusicProvider::SongHit> QQMusicProvider::parseSearchHits(const QJsonObject &root) const
{
    QVector<SongHit> out;
    const QJsonArray list =
        root.value(QStringLiteral("data")).toObject()
            .value(QStringLiteral("song")).toObject()
            .value(QStringLiteral("list")).toArray();

    out.reserve(list.size());
    for (const QJsonValue &value : list) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject obj = value.toObject();
        SongHit hit;
        hit.songmid = obj.value(QStringLiteral("songmid")).toString();
        hit.songname = obj.value(QStringLiteral("songname")).toString();
        hit.album = obj.value(QStringLiteral("albumname")).toString();
        hit.intervalSec = obj.value(QStringLiteral("interval")).toInt();

        QStringList artists;
        for (const QJsonValue &a : obj.value(QStringLiteral("singer")).toArray()) {
            const QString name = a.toObject().value(QStringLiteral("name")).toString();
            if (!name.isEmpty()) {
                artists.append(name);
            }
        }
        hit.artist = artists.join(QStringLiteral(", "));
        if (!hit.songmid.isEmpty() && !hit.songname.isEmpty()) {
            out.append(hit);
        }
    }
    return out;
}

LyricsDocument QQMusicProvider::documentFromLyricJson(const QJsonObject &root, const SongHit &song) const
{
    QString lyric = root.value(QStringLiteral("lyric")).toString();
    QString trans = root.value(QStringLiteral("trans")).toString();

    // Some endpoints still return base64 even with nobase64=1.
    if (!lyric.contains(QLatin1Char('[')) && !lyric.isEmpty()) {
        const QString decoded = provider_helpers::decodeBase64Utf8(lyric);
        if (!decoded.isEmpty()) {
            lyric = decoded;
        }
    }
    if (!trans.contains(QLatin1Char('[')) && !trans.isEmpty()) {
        const QString decoded = provider_helpers::decodeBase64Utf8(trans);
        if (!decoded.isEmpty()) {
            trans = decoded;
        }
    }

    if (lyric.trimmed().isEmpty()) {
        return {};
    }

    LyricsDocument doc = provider_helpers::parseLrc(lyric, id());
    if (!trans.trimmed().isEmpty()) {
        provider_helpers::mergeTranslation(doc, provider_helpers::parseLrc(trans, id()));
    }

    if (doc.title.isEmpty()) {
        doc.title = song.songname;
    }
    if (doc.artist.isEmpty()) {
        doc.artist = song.artist;
    }
    if (doc.album.isEmpty()) {
        doc.album = song.album;
    }
    return doc;
}

void QQMusicProvider::handleSearchFinished()
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

    const QByteArray body = reply->readAll();
    const QJsonDocument json = QJsonDocument::fromJson(body);
    QJsonObject root;
    if (json.isObject()) {
        root = json.object();
    } else {
        const QJsonDocument jp = QJsonDocument::fromJson(provider_helpers::stripJsonp(body).toUtf8());
        if (!jp.isObject()) {
            emit searchFailed(id(), gen, QStringLiteral("invalid search response"));
            return;
        }
        root = jp.object();
    }

    m_hits = parseSearchHits(root);
    if (m_hits.isEmpty()) {
        emit searchFailed(id(), gen, QStringLiteral("no results"));
        return;
    }

    m_hitIndex = 0;
    startLyric(m_hits.at(m_hitIndex));
}

void QQMusicProvider::handleLyricFinished()
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
            startLyric(m_hits.at(m_hitIndex));
            return;
        }
        emit searchFailed(id(), gen, reason);
    };

    if (reply->error() != QNetworkReply::NoError) {
        tryNext(reply->errorString());
        return;
    }

    const QByteArray body = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(body);
    if (!json.isObject()) {
        json = QJsonDocument::fromJson(provider_helpers::stripJsonp(body).toUtf8());
    }
    if (!json.isObject() || m_hitIndex < 0 || m_hitIndex >= m_hits.size()) {
        tryNext(QStringLiteral("invalid lyric response"));
        return;
    }

    LyricsDocument doc = documentFromLyricJson(json.object(), m_hits.at(m_hitIndex));
    if (doc.lines.isEmpty()) {
        tryNext(QStringLiteral("empty lyrics"));
        return;
    }

    emit resultsReady(id(), gen, {doc});
}

} // namespace lyricsqt
