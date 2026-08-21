#pragma once

#include <lyricsqt/ILyricsProvider.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

namespace lyricsqt {

class LrclibProvider : public ILyricsProvider
{
    Q_OBJECT
public:
    explicit LrclibProvider(QObject *parent = nullptr);

    QString id() const override;
    void search(const LyricsSearchRequest &req) override;
    void cancel() override;

private:
    void abortActive();
    void startGet(const LyricsSearchRequest &req);
    void startSearch(const LyricsSearchRequest &req);
    void handleGetFinished();
    void handleSearchFinished();
    LyricsDocument documentFromJsonObject(const QJsonObject &obj) const;
    QVector<LyricsDocument> documentsFromSearchArray(const QJsonArray &arr) const;

    QNetworkAccessManager m_nam;
    QPointer<QNetworkReply> m_reply;
    quint64 m_generation = 0;
    LyricsSearchRequest m_pending;
    bool m_triedSearchFallback = false;
};

} // namespace lyricsqt
