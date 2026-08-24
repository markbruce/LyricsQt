#include <lyricsqt/PlayerService.h>

#include "MprisPlayerBackend.h"
#include "QqMusicCdpPositionSource.h"

#include <lyricsqt/AppSettings.h>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>

namespace lyricsqt {

namespace {

constexpr auto kMprisPrefix = "org.mpris.MediaPlayer2.";
constexpr auto kObjectPath = "/org/mpris/MediaPlayer2";
constexpr auto kPlayerIface = "org.mpris.MediaPlayer2.Player";
constexpr auto kPropsIface = "org.freedesktop.DBus.Properties";

QString normalizePreferredId(const QString &id)
{
    if (id.isEmpty()) {
        return id;
    }
    if (id.startsWith(QLatin1String(kMprisPrefix))) {
        return id;
    }
    return QLatin1String(kMprisPrefix) + id;
}

bool matchesPreferred(const QString &serviceName, const QString &preferredRaw)
{
    if (preferredRaw.isEmpty()) {
        return false;
    }

    const QString preferred = normalizePreferredId(preferredRaw);
    if (serviceName == preferred) {
        return true;
    }

    // Allow short ids such as "spotify" or "chromium" to match
    // org.mpris.MediaPlayer2.spotify / org.mpris.MediaPlayer2.chromium.instanceNNN
    const QString suffix = serviceName.mid(QLatin1String(kMprisPrefix).size());
    return suffix == preferredRaw
        || suffix.startsWith(preferredRaw + QLatin1Char('.'));
}

} // namespace

PlayerService::PlayerService(QObject *parent)
    : PlayerService(nullptr, parent)
{
}

PlayerService::PlayerService(AppSettings *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_backend(new MprisPlayerBackend(this))
    , m_qqCdp(new QqMusicCdpPositionSource(this))
{
    if (m_settings) {
        m_preferredPlayerId = m_settings->preferredPlayerId();
        connect(m_settings, &AppSettings::changed, this, [this](const QString &key) {
            if (key == QLatin1String("PreferredPlayerId")) {
                setPreferredPlayerId(m_settings->preferredPlayerId());
            }
        });
    }

    m_positionTimer.setInterval(250);
    connect(&m_positionTimer, &QTimer::timeout, m_backend, &MprisPlayerBackend::updatePosition);

    m_rediscoverTimer.setInterval(2000);
    connect(&m_rediscoverTimer, &QTimer::timeout, this, &PlayerService::selectAndConnectPlayer);
    m_rediscoverTimer.start();

    connect(m_backend, &MprisPlayerBackend::trackChanged, this, &PlayerService::onBackendTrackChanged);
    connect(m_backend, &MprisPlayerBackend::playbackStatusChanged,
            this, &PlayerService::onBackendPlaybackStatusChanged);
    connect(m_backend, &MprisPlayerBackend::positionChanged,
            this, &PlayerService::onBackendPositionChanged);
    connect(m_qqCdp, &QqMusicCdpPositionSource::positionUpdated,
            this, &PlayerService::onQqCdpPositionUpdated);

    QDBusConnection::sessionBus().connect(
        QStringLiteral("org.freedesktop.DBus"),
        QStringLiteral("/org/freedesktop/DBus"),
        QStringLiteral("org.freedesktop.DBus"),
        QStringLiteral("NameOwnerChanged"),
        this,
        SLOT(onNameOwnerChanged(QString,QString,QString)));

    selectAndConnectPlayer();
}

PlayerService::~PlayerService() = default;

void PlayerService::setPreferredPlayerId(const QString &id)
{
    if (m_preferredPlayerId == id) {
        return;
    }
    m_preferredPlayerId = id;
    selectAndConnectPlayer();
}

QString PlayerService::preferredPlayerId() const
{
    return m_preferredPlayerId;
}

QString PlayerService::activePlayerId() const
{
    return m_backend->serviceName();
}

TrackInfo PlayerService::currentTrack() const
{
    return m_backend->track();
}

bool PlayerService::isPlaying() const
{
    return m_playing;
}

double PlayerService::positionSec() const
{
    if (m_qqCdp && m_qqCdp->hasFreshPosition()) {
        return m_qqCdp->positionSec();
    }
    return m_backend->positionSec();
}

void PlayerService::refresh()
{
    selectAndConnectPlayer();
    if (m_backend->isConnected()) {
        m_backend->refreshAll();
    }
}

void PlayerService::seekTo(double positionSec)
{
    m_backend->seekTo(positionSec);
}

void PlayerService::selectAndConnectPlayer()
{
    const QStringList players = MprisPlayerBackend::availablePlayers();
    const QString chosen = choosePlayer(players);
    const QString previous = m_backend->serviceName();

    if (chosen.isEmpty()) {
        if (m_backend->isConnected() || m_playing || !previous.isEmpty()) {
            // disconnectFromService emits cleared track/playback/position;
            // slots forward those to PlayerService consumers.
            if (m_backend->isConnected()) {
                m_backend->disconnectFromService();
            } else {
                clearPlaybackState();
            }
            emit activePlayerChanged(QString());
            updatePollingTimer();
        }
        updateQqMusicCdpBridge();
        return;
    }

    if (previous != chosen) {
        // connectToService disconnects the previous player first, which emits
        // empty track + Stopped so UI never keeps the old player's state.
        m_backend->connectToService(chosen);
        emit activePlayerChanged(chosen);
    } else if (!m_backend->isConnected()) {
        m_backend->connectToService(chosen);
        emit activePlayerChanged(chosen);
    }
    updateQqMusicCdpBridge();
}

QString PlayerService::choosePlayer(const QStringList &players) const
{
    if (players.isEmpty()) {
        return {};
    }

    if (!m_preferredPlayerId.isEmpty()) {
        for (const QString &player : players) {
            if (matchesPreferred(player, m_preferredPlayerId)) {
                return player;
            }
        }
    }

    for (const QString &player : players) {
        if (playbackStatusFor(player) == QLatin1String("Playing")) {
            return player;
        }
    }

    return players.first();
}

QString PlayerService::playbackStatusFor(const QString &serviceName) const
{
    QDBusInterface props(
        serviceName,
        QLatin1String(kObjectPath),
        QLatin1String(kPropsIface),
        QDBusConnection::sessionBus());
    const QDBusReply<QDBusVariant> reply =
        props.call(QStringLiteral("Get"), QLatin1String(kPlayerIface), QStringLiteral("PlaybackStatus"));
    if (!reply.isValid()) {
        return {};
    }
    return reply.value().variant().toString();
}

void PlayerService::clearPlaybackState()
{
    if (m_playing) {
        m_playing = false;
        emit playbackChanged(false);
    }
    emit trackChanged(TrackInfo{});
    emit positionChanged(0.0);
}

void PlayerService::onBackendTrackChanged(const TrackInfo &track)
{
    emit trackChanged(track);
}

void PlayerService::onBackendPlaybackStatusChanged(const QString &status)
{
    const bool playing = status == QLatin1String("Playing");
    if (m_playing != playing) {
        m_playing = playing;
        emit playbackChanged(m_playing);
    }
    updatePollingTimer();

    // Only re-select when something starts Playing; Stopped during a player
    // switch must not recurse into selectAndConnectPlayer mid-disconnect.
    if (m_preferredPlayerId.isEmpty() && playing) {
        selectAndConnectPlayer();
    }
}

void PlayerService::onBackendPositionChanged(double positionSec)
{
    // QQ Music Chromium MPRIS Position is stuck at 0; prefer CDP audio clock.
    if (m_qqCdp && m_qqCdp->hasFreshPosition()) {
        return;
    }
    emit positionChanged(positionSec);
}

void PlayerService::onQqCdpPositionUpdated(double positionSec, double, bool)
{
    emit positionChanged(positionSec);
}

bool PlayerService::shouldUseQqMusicCdp() const
{
    const QString id = m_backend->serviceName();
    // Official Linux QQ Music registers as Chromium MPRIS with Position=0.
    return id.contains(QLatin1String("chromium"), Qt::CaseInsensitive);
}

void PlayerService::updateQqMusicCdpBridge()
{
    if (!m_qqCdp) {
        return;
    }
    m_qqCdp->setEnabled(shouldUseQqMusicCdp());
}

void PlayerService::updatePollingTimer()
{
    if (m_playing && m_backend->isConnected()) {
        if (!m_positionTimer.isActive()) {
            m_positionTimer.start();
        }
    } else {
        m_positionTimer.stop();
    }
    updateQqMusicCdpBridge();
}

void PlayerService::onNameOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    if (!name.startsWith(QLatin1String(kMprisPrefix))) {
        return;
    }

    // Quit when designated (preferred) or currently active player leaves the bus.
    if (m_settings && m_settings->quitWithPlayer()
        && !oldOwner.isEmpty() && newOwner.isEmpty()) {
        const bool designatedLeft = !m_preferredPlayerId.isEmpty()
            && matchesPreferred(name, m_preferredPlayerId);
        const bool activeLeft = m_preferredPlayerId.isEmpty()
            && name == m_backend->serviceName();
        if (designatedLeft || activeLeft) {
            QCoreApplication::quit();
            return;
        }
    }

    selectAndConnectPlayer();
}

} // namespace lyricsqt
