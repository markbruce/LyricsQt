#include <lyricsqt/LrclibProvider.h>

#include <lyricsqt/LrcParser.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace lyricsqt {

namespace {

constexpr char kProviderId[] = "lrclib";
constexpr char kApiBase[] = "https://lrclib.net/api";

} // namespace

LrclibProvider::LrclibProvider(QObject *parent)
    : ILyricsProvider(parent)
{
}

QString LrclibProvider::id() const
{
    return QString::fromLatin1(kProviderId);
}

void LrclibProvider::cancel()
{
    abortActive();
    m_generation = 0;
    m_triedSearchFallback = false;
}

void LrclibProvider::abortActive()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void LrclibProvider::search(const LyricsSearchRequest &req)
{
    abortActive();
    m_generation = req.generation;
    m_pending = req;
    m_triedSearchFallback = false;

    if (req.title.isEmpty() && req.artist.isEmpty()) {
        emit searchFailed(id(), m_generation, QStringLiteral("empty title and artist"));
        return;
    }

    // Prefer precise get when duration is known; otherwise search.
    if (req.durationSec > 0.0) {
        startGet(req);
    } else {
        startSearch(req);
    }
}

void LrclibProvider::startGet(const LyricsSearchRequest &req)
{
    QUrl url(QString::fromLatin1(kApiBase) + QStringLiteral("/get"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("track_name"), req.title);
    query.addQueryItem(QStringLiteral("artist_name"), req.artist);
    if (!req.album.isEmpty()) {
        query.addQueryItem(QStringLiteral("album_name"), req.album);
    }
    query.addQueryItem(QStringLiteral("duration"),
                       QString::number(qRound(req.durationSec)));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("LyricsQt/0.1 (+https://github.com/lyricsqt)"));
    m_reply = m_nam.get(request);
    connect(m_reply, &QNetworkReply::finished, this, &LrclibProvider::handleGetFinished);
}

void LrclibProvider::startSearch(const LyricsSearchRequest &req)
{
    QUrl url(QString::fromLatin1(kApiBase) + QStringLiteral("/search"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("track_name"), req.title);
    query.addQueryItem(QStringLiteral("artist_name"), req.artist);
    if (!req.album.isEmpty()) {
        query.addQueryItem(QStringLiteral("album_name"), req.album);
    }
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("LyricsQt/0.1 (+https://github.com/lyricsqt)"));
    m_reply = m_nam.get(request);
    connect(m_reply, &QNetworkReply::finished, this, &LrclibProvider::handleSearchFinished);
}

LyricsDocument LrclibProvider::documentFromJsonObject(const QJsonObject &obj) const
{
    LyricsDocument doc;
    doc.sourceId = id();
    doc.title = obj.value(QStringLiteral("trackName")).toString();
    doc.artist = obj.value(QStringLiteral("artistName")).toString();
    doc.album = obj.value(QStringLiteral("albumName")).toString();

    const QString synced = obj.value(QStringLiteral("syncedLyrics")).toString();
    if (!synced.isEmpty()) {
        doc = LrcParser::parse(synced);
        doc.sourceId = id();
        if (doc.title.isEmpty()) {
            doc.title = obj.value(QStringLiteral("trackName")).toString();
        }
        if (doc.artist.isEmpty()) {
            doc.artist = obj.value(QStringLiteral("artistName")).toString();
        }
        if (doc.album.isEmpty()) {
            doc.album = obj.value(QStringLiteral("albumName")).toString();
        }
        return doc;
    }

    const QString plain = obj.value(QStringLiteral("plainLyrics")).toString();
    if (!plain.isEmpty()) {
        // Unsynced fallback: one line at t=0 so callers still get content.
        LyricsLine line;
        line.positionSec = 0.0;
        line.content = plain;
        doc.lines.append(line);
    }
    return doc;
}

QVector<LyricsDocument> LrclibProvider::documentsFromSearchArray(const QJsonArray &arr) const
{
    QVector<LyricsDocument> out;
    out.reserve(arr.size());
    for (const QJsonValue &value : arr) {
        if (!value.isObject()) {
            continue;
        }
        LyricsDocument doc = documentFromJsonObject(value.toObject());
        if (!doc.lines.isEmpty()) {
            out.append(doc);
        }
    }
    return out;
}

void LrclibProvider::handleGetFinished()
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

    if (reply->error() != QNetworkReply::NoError
        || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 404) {
        // Fall back to search once.
        if (!m_triedSearchFallback) {
            m_triedSearchFallback = true;
            startSearch(m_pending);
            return;
        }
        emit searchFailed(id(), gen,
                          reply->error() != QNetworkReply::NoError
                              ? reply->errorString()
                              : QStringLiteral("not found"));
        return;
    }

    const QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
    if (!json.isObject()) {
        if (!m_triedSearchFallback) {
            m_triedSearchFallback = true;
            startSearch(m_pending);
            return;
        }
        emit searchFailed(id(), gen, QStringLiteral("invalid get response"));
        return;
    }

    LyricsDocument doc = documentFromJsonObject(json.object());
    if (doc.lines.isEmpty()) {
        if (!m_triedSearchFallback) {
            m_triedSearchFallback = true;
            startSearch(m_pending);
            return;
        }
        emit searchFailed(id(), gen, QStringLiteral("empty lyrics"));
        return;
    }

    emit resultsReady(id(), gen, {doc});
}

void LrclibProvider::handleSearchFinished()
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
    if (!json.isArray()) {
        emit searchFailed(id(), gen, QStringLiteral("invalid search response"));
        return;
    }

    const QVector<LyricsDocument> docs = documentsFromSearchArray(json.array());
    if (docs.isEmpty()) {
        emit searchFailed(id(), gen, QStringLiteral("no results"));
        return;
    }

    emit resultsReady(id(), gen, docs);
}

} // namespace lyricsqt
