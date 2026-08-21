#pragma once

#include <lyricsqt/ILyricsProvider.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

namespace lyricsqt {

class NetEaseProvider : public ILyricsProvider
{
    Q_OBJECT
public:
    explicit NetEaseProvider(QObject *parent = nullptr);

    QString id() const override;
    void search(const LyricsSearchRequest &req) override;
    void cancel() override;

private:
    struct SongHit {
        qint64 id = 0;
        QString name;
        QString artist;
        QString album;
        int durationMs = 0;
    };

    void abortActive();
    void startSearch(const LyricsSearchRequest &req);
    void startLyric(const SongHit &song);
    void handleSearchFinished();
    void handleLyricFinished();
    QVector<SongHit> parseSearchHits(const QJsonObject &root) const;
    LyricsDocument documentFromLyricJson(const QJsonObject &root, const SongHit &song) const;

    QNetworkAccessManager m_nam;
    QPointer<QNetworkReply> m_reply;
    quint64 m_generation = 0;
    LyricsSearchRequest m_pending;
    QVector<SongHit> m_hits;
    int m_hitIndex = 0;
    bool m_cookieRetryDone = false;
};

} // namespace lyricsqt
