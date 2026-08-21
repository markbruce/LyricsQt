#include "MprisPlayerBackend.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QDBusVariant>
#include <QUrl>
#include <QVariant>
#include <QtGlobal>

namespace lyricsqt {

namespace {

constexpr auto kMprisPrefix = "org.mpris.MediaPlayer2.";
constexpr auto kObjectPath = "/org/mpris/MediaPlayer2";
constexpr auto kPlayerIface = "org.mpris.MediaPlayer2.Player";
constexpr auto kPropsIface = "org.freedesktop.DBus.Properties";

QString artistFromVariant(const QVariant &value)
{
    if (value.canConvert<QStringList>()) {
        return value.toStringList().join(QStringLiteral(", "));
    }
    if (value.typeId() == QMetaType::QString) {
        return value.toString();
    }

    // Some players deliver as QDBusArgument array of strings.
    if (value.canConvert<QDBusArgument>()) {
        const auto arg = qvariant_cast<QDBusArgument>(value);
        if (arg.currentType() == QDBusArgument::ArrayType) {
            QStringList artists;
            arg.beginArray();
            while (!arg.atEnd()) {
                QString artist;
                arg >> artist;
                if (!artist.isEmpty()) {
                    artists.append(artist);
                }
            }
            arg.endArray();
            return artists.join(QStringLiteral(", "));
        }
    }

    return value.toString();
}

qint64 lengthFromVariant(const QVariant &value)
{
    bool ok = false;
    const qint64 length = value.toLongLong(&ok);
    return ok ? length : 0;
}

} // namespace

MprisPlayerBackend::MprisPlayerBackend(QObject *parent)
    : QObject(parent)
{
}

MprisPlayerBackend::~MprisPlayerBackend()
{
    disconnectFromService();
}

QStringList MprisPlayerBackend::availablePlayers()
{
    QStringList players;
    auto *iface = QDBusConnection::sessionBus().interface();
    if (!iface) {
        return players;
    }

    const QDBusReply<QStringList> reply = iface->registeredServiceNames();
    if (!reply.isValid()) {
        return players;
    }

    for (const QString &name : reply.value()) {
        if (name.startsWith(QLatin1String(kMprisPrefix))) {
            players.append(name);
        }
    }
    players.sort();
    return players;
}

QString MprisPlayerBackend::serviceName() const
{
    return m_serviceName;
}

bool MprisPlayerBackend::isConnected() const
{
    return !m_serviceName.isEmpty();
}

void MprisPlayerBackend::connectToService(const QString &serviceName)
{
    if (m_serviceName == serviceName && isConnected()) {
        refreshAll();
        return;
    }

    disconnectFromService();

    if (serviceName.isEmpty()) {
        return;
    }

    m_serviceName = serviceName;
    subscribeSignals();
    emit connectionChanged(true);
    refreshAll();
}

void MprisPlayerBackend::disconnectFromService()
{
    if (!isConnected()) {
        return;
    }

    unsubscribeSignals();
    m_serviceName.clear();
    m_track = TrackInfo{};
    m_playbackStatus = QStringLiteral("Stopped");
    m_positionSec = 0.0;
    emit connectionChanged(false);
}

TrackInfo MprisPlayerBackend::track() const
{
    return m_track;
}

QString MprisPlayerBackend::playbackStatus() const
{
    return m_playbackStatus;
}

bool MprisPlayerBackend::isPlaying() const
{
    return m_playbackStatus == QLatin1String("Playing");
}

double MprisPlayerBackend::positionSec() const
{
    return m_positionSec;
}

void MprisPlayerBackend::refreshAll()
{
    if (!isConnected()) {
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_serviceName,
        QLatin1String(kObjectPath),
        QLatin1String(kPropsIface),
        QStringLiteral("GetAll"));
    msg << QLatin1String(kPlayerIface);

    auto *watcher = new QDBusPendingCallWatcher(
        QDBusConnection::sessionBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &MprisPlayerBackend::onGetAllFinished);
}

void MprisPlayerBackend::updatePosition()
{
    if (!isConnected()) {
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(
        m_serviceName,
        QLatin1String(kObjectPath),
        QLatin1String(kPropsIface),
        QStringLiteral("Get"));
    msg << QLatin1String(kPlayerIface) << QStringLiteral("Position");

    auto *watcher = new QDBusPendingCallWatcher(
        QDBusConnection::sessionBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &MprisPlayerBackend::onGetPositionFinished);
}

void MprisPlayerBackend::seekTo(double positionSec)
{
    if (!isConnected()) {
        return;
    }

    const qint64 targetUs = static_cast<qint64>(positionSec * 1'000'000.0);
    QDBusInterface player(
        m_serviceName,
        QLatin1String(kObjectPath),
        QLatin1String(kPlayerIface),
        QDBusConnection::sessionBus());

    if (!m_track.id.isEmpty()) {
        const QDBusObjectPath trackId(m_track.id);
        player.call(QStringLiteral("SetPosition"), QVariant::fromValue(trackId), targetUs);
        return;
    }

    const qint64 offsetUs = targetUs - static_cast<qint64>(m_positionSec * 1'000'000.0);
    player.call(QStringLiteral("Seek"), offsetUs);
}

void MprisPlayerBackend::onPropertiesChanged(const QString &interfaceName,
                                             const QVariantMap &changedProperties,
                                             const QStringList &invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties);
    if (interfaceName != QLatin1String(kPlayerIface)) {
        return;
    }

    if (changedProperties.contains(QStringLiteral("Metadata"))) {
        applyMetadata(unwrapDBusVariant(changedProperties.value(QStringLiteral("Metadata"))).toMap());
    }
    if (changedProperties.contains(QStringLiteral("PlaybackStatus"))) {
        applyPlaybackStatus(
            unwrapDBusVariant(changedProperties.value(QStringLiteral("PlaybackStatus"))).toString());
    }
    if (changedProperties.contains(QStringLiteral("Position"))) {
        applyPositionUs(
            unwrapDBusVariant(changedProperties.value(QStringLiteral("Position"))).toLongLong());
    }
}

void MprisPlayerBackend::onSeeked(qint64 positionUs)
{
    applyPositionUs(positionUs);
}

void MprisPlayerBackend::onGetAllFinished(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (reply.isError()) {
        return;
    }

    const QVariantMap props = reply.value();
    if (props.contains(QStringLiteral("Metadata"))) {
        applyMetadata(unwrapDBusVariant(props.value(QStringLiteral("Metadata"))).toMap());
    }
    if (props.contains(QStringLiteral("PlaybackStatus"))) {
        applyPlaybackStatus(
            unwrapDBusVariant(props.value(QStringLiteral("PlaybackStatus"))).toString());
    }
    if (props.contains(QStringLiteral("Position"))) {
        applyPositionUs(unwrapDBusVariant(props.value(QStringLiteral("Position"))).toLongLong());
    }
}

void MprisPlayerBackend::onGetPositionFinished(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    QDBusPendingReply<QDBusVariant> reply = *watcher;
    if (reply.isError()) {
        return;
    }

    applyPositionUs(unwrapDBusVariant(QVariant::fromValue(reply.value())).toLongLong());
}

void MprisPlayerBackend::subscribeSignals()
{
    if (m_signalsConnected || m_serviceName.isEmpty()) {
        return;
    }

    auto bus = QDBusConnection::sessionBus();
    bus.connect(
        m_serviceName,
        QLatin1String(kObjectPath),
        QLatin1String(kPropsIface),
        QStringLiteral("PropertiesChanged"),
        this,
        SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));
    bus.connect(
        m_serviceName,
        QLatin1String(kObjectPath),
        QLatin1String(kPlayerIface),
        QStringLiteral("Seeked"),
        this,
        SLOT(onSeeked(qint64)));
    m_signalsConnected = true;
}

void MprisPlayerBackend::unsubscribeSignals()
{
    if (!m_signalsConnected || m_serviceName.isEmpty()) {
        m_signalsConnected = false;
        return;
    }

    auto bus = QDBusConnection::sessionBus();
    bus.disconnect(
        m_serviceName,
        QLatin1String(kObjectPath),
        QLatin1String(kPropsIface),
        QStringLiteral("PropertiesChanged"),
        this,
        SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));
    bus.disconnect(
        m_serviceName,
        QLatin1String(kObjectPath),
        QLatin1String(kPlayerIface),
        QStringLiteral("Seeked"),
        this,
        SLOT(onSeeked(qint64)));
    m_signalsConnected = false;
}

void MprisPlayerBackend::applyMetadata(const QVariantMap &metadata)
{
    const TrackInfo next = trackFromMetadata(metadata);
    if (m_track == next) {
        return;
    }
    m_track = next;
    emit trackChanged(m_track);
}

void MprisPlayerBackend::applyPlaybackStatus(const QString &status)
{
    if (m_playbackStatus == status) {
        return;
    }
    m_playbackStatus = status;
    emit playbackStatusChanged(m_playbackStatus);
}

void MprisPlayerBackend::applyPositionUs(qint64 positionUs)
{
    const double sec = static_cast<double>(positionUs) / 1'000'000.0;
    if (qFuzzyCompare(m_positionSec + 1.0, sec + 1.0)) {
        return;
    }
    m_positionSec = sec;
    emit positionChanged(m_positionSec);
}

QVariant MprisPlayerBackend::getPlayerProperty(const QString &name) const
{
    if (!isConnected()) {
        return {};
    }

    QDBusInterface props(
        m_serviceName,
        QLatin1String(kObjectPath),
        QLatin1String(kPropsIface),
        QDBusConnection::sessionBus());
    const QDBusReply<QDBusVariant> reply = props.call(QStringLiteral("Get"),
                                                      QLatin1String(kPlayerIface),
                                                      name);
    if (!reply.isValid()) {
        return {};
    }
    return unwrapDBusVariant(QVariant::fromValue(reply.value()));
}

TrackInfo MprisPlayerBackend::trackFromMetadata(const QVariantMap &metadata)
{
    TrackInfo track;
    const QVariant trackIdVar = unwrapDBusVariant(metadata.value(QStringLiteral("mpris:trackid")));
    if (trackIdVar.canConvert<QDBusObjectPath>()) {
        track.id = qvariant_cast<QDBusObjectPath>(trackIdVar).path();
    } else {
        track.id = trackIdVar.toString();
    }

    track.title = unwrapDBusVariant(metadata.value(QStringLiteral("xesam:title"))).toString();
    track.artist = artistFromVariant(unwrapDBusVariant(metadata.value(QStringLiteral("xesam:artist"))));
    track.album = unwrapDBusVariant(metadata.value(QStringLiteral("xesam:album"))).toString();
    track.lengthUs = lengthFromVariant(unwrapDBusVariant(metadata.value(QStringLiteral("mpris:length"))));

    const QString art = unwrapDBusVariant(metadata.value(QStringLiteral("mpris:artUrl"))).toString();
    if (!art.isEmpty()) {
        track.artUrl = QUrl(art);
    }

    const QString url = unwrapDBusVariant(metadata.value(QStringLiteral("xesam:url"))).toString();
    if (!url.isEmpty()) {
        track.fileUrl = QUrl(url);
    }

    return track;
}

QVariant MprisPlayerBackend::unwrapDBusVariant(const QVariant &value)
{
    if (value.canConvert<QDBusVariant>()) {
        return unwrapDBusVariant(qvariant_cast<QDBusVariant>(value).variant());
    }

    if (value.typeId() == qMetaTypeId<QDBusArgument>()) {
        const auto arg = qvariant_cast<QDBusArgument>(value);
        if (arg.currentType() == QDBusArgument::MapType) {
            QVariantMap map;
            arg.beginMap();
            while (!arg.atEnd()) {
                arg.beginMapEntry();
                QString key;
                QVariant entryValue;
                arg >> key >> entryValue;
                arg.endMapEntry();
                map.insert(key, unwrapDBusVariant(entryValue));
            }
            arg.endMap();
            return map;
        }
        if (arg.currentType() == QDBusArgument::ArrayType) {
            QStringList list;
            arg.beginArray();
            while (!arg.atEnd()) {
                QString item;
                arg >> item;
                list.append(item);
            }
            arg.endArray();
            return list;
        }
        if (arg.currentType() == QDBusArgument::BasicType) {
            QVariant basic;
            arg >> basic;
            return basic;
        }
    }

    return value;
}

} // namespace lyricsqt
