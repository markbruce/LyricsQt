#pragma once

#include <lyricsqt/ILyricsProvider.h>
#include <lyricsqt/LyricsDocument.h>
#include <lyricsqt/TrackInfo.h>

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVector>

namespace lyricsqt {

class ProviderHub : public QObject
{
    Q_OBJECT
public:
    explicit ProviderHub(QObject *parent = nullptr);

    void addProvider(ILyricsProvider *provider);
    void setTimeoutMs(int ms);
    int timeoutMs() const { return m_timeoutMs; }

    /// Empty = all registered providers (registration order).
    /// Non-empty = only listed ids, in given order.
    void setEnabledProviderIds(const QStringList &ids);
    QStringList enabledProviderIds() const { return m_enabledProviderIds; }

    /// Auto-search: emits lyricsFound with the best candidate (if any).
    void search(const TrackInfo &track);
    /// Manual/UI search: emits candidatesReady with all scored candidates (no lyricsFound).
    void searchCollecting(const TrackInfo &track);
    void cancel();

signals:
    void lyricsFound(const TrackInfo &track, const LyricsDocument &doc);
    void candidatesReady(const TrackInfo &track, const QVector<LyricsDocument> &docs);
    void searchFinished(const TrackInfo &track, bool found);

private:
    void startSearch(const TrackInfo &track, bool collecting);
    void onProviderResults(const QString &providerId, quint64 generation,
                           const QVector<LyricsDocument> &docs);
    void onProviderFailed(const QString &providerId, quint64 generation, const QString &error);
    void onProviderSettled();
    void finishIfReady();
    void emitBestAndFinish();
    QVector<ILyricsProvider *> activeProviders() const;

    QVector<ILyricsProvider *> m_providers;
    QStringList m_enabledProviderIds;
    QTimer m_timeout;
    int m_timeoutMs = 10'000;
    quint64 m_generation = 0;
    TrackInfo m_track;
    QVector<LyricsDocument> m_candidates;
    int m_pendingProviders = 0;
    bool m_finished = true;
    bool m_collecting = false;
};

} // namespace lyricsqt
