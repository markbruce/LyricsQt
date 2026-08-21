#pragma once

#include <lyricsqt/ILyricsProvider.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

namespace lyricsqt {

class KugouProvider : public ILyricsProvider
{
    Q_OBJECT
public:
    explicit KugouProvider(QObject *parent = nullptr);

    QString id() const override;
    void search(const LyricsSearchRequest &req) override;
    void cancel() override;

private:
    struct SongHit {
        QString id;
        QString accesskey;
        QString song;
        QString singer;
        int durationMs = 0;
    };

    void abortActive();
    void startSearch(const LyricsSearchRequest &req);
    void startDownload(const SongHit &song);
    void handleSearchFinished();
    void handleDownloadFinished();
    QVector<SongHit> parseSearchHits(const QJsonObject &root) const;
    LyricsDocument documentFromDownloadJson(const QJsonObject &root, const SongHit &song) const;

    QNetworkAccessManager m_nam;
    QPointer<QNetworkReply> m_reply;
    quint64 m_generation = 0;
    LyricsSearchRequest m_pending;
    QVector<SongHit> m_hits;
    int m_hitIndex = 0;
};

} // namespace lyricsqt
