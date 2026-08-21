#pragma once

#include <lyricsqt/TrackInfo.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class QDBusPendingCallWatcher;

namespace lyricsqt {

class MprisPlayerBackend : public QObject
{
    Q_OBJECT
public:
    explicit MprisPlayerBackend(QObject *parent = nullptr);
    ~MprisPlayerBackend() override;

    static QStringList availablePlayers();

    QString serviceName() const;
    bool isConnected() const;

    void connectToService(const QString &serviceName);
    void disconnectFromService();

    TrackInfo track() const;
    QString playbackStatus() const;
    bool isPlaying() const;
    double positionSec() const;

    void refreshAll();
    void updatePosition();
    void seekTo(double positionSec);

signals:
    void trackChanged(const lyricsqt::TrackInfo &track);
    void playbackStatusChanged(const QString &status);
    void positionChanged(double positionSec);
    void connectionChanged(bool connected);

private slots:
    void onPropertiesChanged(const QString &interfaceName,
                              const QVariantMap &changedProperties,
                              const QStringList &invalidatedProperties);
    void onSeeked(qint64 positionUs);
    void onGetAllFinished(QDBusPendingCallWatcher *watcher);
    void onGetPositionFinished(QDBusPendingCallWatcher *watcher);

private:
    void subscribeSignals();
    void unsubscribeSignals();
    void applyMetadata(const QVariantMap &metadata);
    void applyPlaybackStatus(const QString &status);
    void applyPositionUs(qint64 positionUs);
    QVariant getPlayerProperty(const QString &name) const;
    static TrackInfo trackFromMetadata(const QVariantMap &metadata);
    static QVariant unwrapDBusVariant(const QVariant &value);

    QString m_serviceName;
    TrackInfo m_track;
    QString m_playbackStatus = QStringLiteral("Stopped");
    double m_positionSec = 0.0;
    bool m_signalsConnected = false;
};

} // namespace lyricsqt
