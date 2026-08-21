#include <lyricsqt/NetEaseProvider.h>

#include "ProviderHelpers.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace lyricsqt {

namespace {

constexpr char kProviderId[] = "netease";
constexpr char kUserAgent[] =
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
    "AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.4 Safari/605.1.15";
constexpr char kReferer[] = "https://music.163.com/";
constexpr char kSearchUrl[] = "https://music.163.com/api/cloudsearch/pc";
constexpr char kSearchFallbackUrl[] = "https://music.163.com/api/search/pc";
constexpr char kLyricUrl[] = "https://music.163.com/api/song/lyric";

QString searchKeyword(const LyricsSearchRequest &req)
{
    if (!req.artist.isEmpty() && !req.title.isEmpty()) {
        return req.title + QLatin1Char(' ') + req.artist;
    }
    return !req.title.isEmpty() ? req.title : req.artist;
}

} // namespace

NetEaseProvider::NetEaseProvider(QObject *parent)
    : ILyricsProvider(parent)
{
}

QString NetEaseProvider::id() const
{
    return QString::fromLatin1(kProviderId);
}

void NetEaseProvider::cancel()
{
    abortActive();
    m_generation = 0;
    m_hits.clear();
    m_hitIndex = 0;
    m_cookieRetryDone = false;
}

void NetEaseProvider::abortActive()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void NetEaseProvider::search(const LyricsSearchRequest &req)
{
    abortActive();
    m_generation = req.generation;
    m_pending = req;
    m_hits.clear();
    m_hitIndex = 0;
    m_cookieRetryDone = false;

    if (req.title.isEmpty() && req.artist.isEmpty()) {
        emit searchFailed(id(), m_generation, QStringLiteral("empty title and artist"));
        return;
    }

    startSearch(req);
}

void NetEaseProvider::startSearch(const LyricsSearchRequest &req)
{
    QUrl url(QString::fromLatin1(m_cookieRetryDone ? kSearchFallbackUrl : kSearchUrl));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("s"), searchKeyword(req));
    query.addQueryItem(QStringLiteral("type"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("limit"), QStringLiteral("8"));
    query.addQueryItem(QStringLiteral("offset"), QStringLiteral("0"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));
    request.setRawHeader("Referer", QByteArray(kReferer));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded"));

    // LyricsKit uses POST; body may be empty when query is in URL.
    m_reply = m_nam.post(request, QByteArray());
    connect(m_reply, &QNetworkReply::finished, this, &NetEaseProvider::handleSearchFinished);
}

void NetEaseProvider::startLyric(const SongHit &song)
{
    QUrl url(QString::fromLatin1(kLyricUrl));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("id"), QString::number(song.id));
    query.addQueryItem(QStringLiteral("lv"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("kv"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("tv"), QStringLiteral("-1"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kUserAgent));
    request.setRawHeader("Referer", QByteArray(kReferer));

    m_reply = m_nam.get(request);
    connect(m_reply, &QNetworkReply::finished, this, &NetEaseProvider::handleLyricFinished);
}

QVector<NetEaseProvider::SongHit> NetEaseProvider::parseSearchHits(const QJsonObject &root) const
{
    QVector<SongHit> out;
    QJsonArray songs = root.value(QStringLiteral("result")).toObject().value(QStringLiteral("songs")).toArray();
    if (songs.isEmpty()) {
        // Some responses nest under "songs" directly.
        songs = root.value(QStringLiteral("songs")).toArray();
    }

    out.reserve(songs.size());
    for (const QJsonValue &value : songs) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject obj = value.toObject();
        SongHit hit;
        hit.id = static_cast<qint64>(obj.value(QStringLiteral("id")).toDouble());
        hit.name = obj.value(QStringLiteral("name")).toString();
        hit.album = obj.value(QStringLiteral("al")).toObject().value(QStringLiteral("name")).toString();
        if (hit.album.isEmpty()) {
            hit.album = obj.value(QStringLiteral("album")).toObject().value(QStringLiteral("name")).toString();
        }
        hit.durationMs = obj.value(QStringLiteral("dt")).toInt();
        if (hit.durationMs <= 0) {
            hit.durationMs = obj.value(QStringLiteral("duration")).toInt();
        }

        QStringList artists;
        QJsonArray artistArr = obj.value(QStringLiteral("ar")).toArray();
        if (artistArr.isEmpty()) {
            artistArr = obj.value(QStringLiteral("artists")).toArray();
        }
        for (const QJsonValue &a : artistArr) {
            const QString name = a.toObject().value(QStringLiteral("name")).toString();
            if (!name.isEmpty()) {
                artists.append(name);
            }
        }
        hit.artist = artists.join(QStringLiteral(", "));
        if (hit.id > 0 && !hit.name.isEmpty()) {
            out.append(hit);
        }
    }
    return out;
}

LyricsDocument NetEaseProvider::documentFromLyricJson(const QJsonObject &root, const SongHit &song) const
{
    const QString lrcRaw = root.value(QStringLiteral("lrc")).toObject().value(QStringLiteral("lyric")).toString();
    const QString tlyricRaw =
        root.value(QStringLiteral("tlyric")).toObject().value(QStringLiteral("lyric")).toString();

    if (lrcRaw.trimmed().isEmpty()) {
        return {};
    }

    LyricsDocument doc =
        provider_helpers::parseLrc(provider_helpers::fixNetEaseTimeTags(lrcRaw), id());
    if (!tlyricRaw.trimmed().isEmpty()) {
        const LyricsDocument trans =
            provider_helpers::parseLrc(provider_helpers::fixNetEaseTimeTags(tlyricRaw), id());
        provider_helpers::mergeTranslation(doc, trans);
    }

    if (doc.title.isEmpty()) {
        doc.title = song.name;
    }
    if (doc.artist.isEmpty()) {
        doc.artist = song.artist;
    }
    if (doc.album.isEmpty()) {
        doc.album = song.album;
    }
    return doc;
}

void NetEaseProvider::handleSearchFinished()
{
    QNetworkReply *reply = m_reply;
    if (!reply) {
        return;
    }
    m_reply = nullptr;

    // Persist Set-Cookie for subsequent lyric requests (LyricsKit pattern).
    const QList<QNetworkCookie> cookies =
        QNetworkCookie::parseCookies(reply->rawHeader("Set-Cookie"));
    if (!cookies.isEmpty() && m_nam.cookieJar()) {
        for (const QNetworkCookie &cookie : cookies) {
            m_nam.cookieJar()->insertCookie(cookie);
        }
    }
    reply->deleteLater();

    const quint64 gen = m_generation;
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        if (!m_cookieRetryDone) {
            m_cookieRetryDone = true;
            startSearch(m_pending);
            return;
        }
        emit searchFailed(id(), gen, reply->errorString());
        return;
    }

    const QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
    if (!json.isObject()) {
        if (!m_cookieRetryDone) {
            m_cookieRetryDone = true;
            startSearch(m_pending);
            return;
        }
        emit searchFailed(id(), gen, QStringLiteral("invalid search response"));
        return;
    }

    m_hits = parseSearchHits(json.object());
    if (m_hits.isEmpty()) {
        if (!m_cookieRetryDone) {
            m_cookieRetryDone = true;
            startSearch(m_pending);
            return;
        }
        emit searchFailed(id(), gen, QStringLiteral("no results"));
        return;
    }

    m_hitIndex = 0;
    startLyric(m_hits.at(m_hitIndex));
}

void NetEaseProvider::handleLyricFinished()
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

    const QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
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
