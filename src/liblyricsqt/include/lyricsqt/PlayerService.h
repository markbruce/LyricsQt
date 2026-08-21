#pragma once

#include <lyricsqt/TrackInfo.h>

#include <QObject>
#include <QString>
#include <QTimer>

namespace lyricsqt {

class AppSettings;
class MprisPlayerBackend;

class PlayerService : public QObject
{
    Q_OBJECT
public:
    explicit PlayerService(QObject *parent = nullptr);
    explicit PlayerService(AppSettings *settings, QObject *parent = nullptr);
    ~PlayerService() override;

    void setPreferredPlayerId(const QString &id);
    QString preferredPlayerId() const;

    QString activePlayerId() const;
    TrackInfo currentTrack() const;
    bool isPlaying() const;
    double positionSec() const;

    void refresh();
    void seekTo(double positionSec);

signals:
    void trackChanged(const lyricsqt::TrackInfo &track);
    void playbackChanged(bool playing);
    void positionChanged(double positionSec);
    void activePlayerChanged(const QString &playerId);

private:
    void selectAndConnectPlayer();
    QString choosePlayer(const QStringList &players) const;
    QString playbackStatusFor(const QString &serviceName) const;
    void clearPlaybackState();
    void onBackendTrackChanged(const TrackInfo &track);
    void onBackendPlaybackStatusChanged(const QString &status);
    void onBackendPositionChanged(double positionSec);
    void updatePollingTimer();

private slots:
    void onNameOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);

private:
    AppSettings *m_settings = nullptr;
    QString m_preferredPlayerId;
    MprisPlayerBackend *m_backend = nullptr;
    QTimer m_positionTimer;
    QTimer m_rediscoverTimer;
    bool m_playing = false;
};

} // namespace lyricsqt
