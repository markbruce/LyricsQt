#include <lyricsqt/ExportServer.h>

#include "ExportAdaptor.h"

#include <lyricsqt/LyricsSession.h>
#include <lyricsqt/PlayerService.h>
#include <lyricsqt/TrackInfo.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QtGlobal>

#include <cstdio>

namespace lyricsqt {

namespace {

constexpr auto kDBusService = "org.lyricsqt.Export";
constexpr auto kDBusPath = "/org/lyricsqt/Export";
constexpr auto kDBusInterface = "org.lyricsqt.Export";

QByteArray linePayload(const QString &line)
{
    QByteArray bytes = line.toUtf8();
    bytes.append('\n');
    return bytes;
}

} // namespace

ExportServer::ExportServer(LyricsSession *session, PlayerService *player, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_player(player)
    , m_socketPath(defaultSocketPath())
{
    Q_ASSERT(m_session);
    Q_ASSERT(m_player);

    new ExportAdaptor(this);

    connect(m_session, &LyricsSession::currentLineChanged,
            this, &ExportServer::onCurrentLineChanged);
    connect(m_session, &LyricsSession::lyricsChanged,
            this, &ExportServer::onLyricsChanged);
    connect(m_player, &PlayerService::trackChanged,
            this, &ExportServer::onTrackChanged);
    connect(m_player, &PlayerService::playbackChanged,
            this, &ExportServer::onPlaybackChanged);

    refreshTrackFromPlayer();
    refreshLineFromSession();
}

ExportServer::~ExportServer()
{
    stop();
}

QString ExportServer::defaultSocketPath()
{
    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (!runtime.isEmpty()) {
        return runtime + QStringLiteral("/lyricsqt.sock");
    }
    return QStringLiteral("/tmp/lyricsqt.sock");
}

void ExportServer::setPipeMode(bool enabled)
{
    m_pipeMode = enabled;
}

bool ExportServer::pipeMode() const
{
    return m_pipeMode;
}

void ExportServer::setSocketPath(const QString &path)
{
    if (m_running) {
        qWarning("ExportServer: setSocketPath ignored while running");
        return;
    }
    if (!path.isEmpty()) {
        m_socketPath = path;
    }
}

bool ExportServer::start()
{
    if (m_running) {
        return true;
    }

    if (!m_server) {
        m_server = new QLocalServer(this);
        m_server->setSocketOptions(QLocalServer::UserAccessOption);
        connect(m_server, &QLocalServer::newConnection,
                this, &ExportServer::onNewConnection);
    }

    QLocalServer::removeServer(m_socketPath);
    if (!m_server->listen(m_socketPath)) {
        qWarning("ExportServer: failed to listen on %s: %s",
                 qPrintable(m_socketPath),
                 qPrintable(m_server->errorString()));
        return false;
    }

    if (!registerDBus()) {
        qWarning("ExportServer: D-Bus registration failed; socket export still active");
    }

    m_running = true;
    if (m_dbusRegistered) {
        notifyDBusPropertiesChanged();
    }
    // Connected clients get a snapshot in onNewConnection; stdout gets a sync
    // line when pipe mode is on so Waybar-style consumers see current text.
    if (m_pipeMode) {
        const QByteArray payload = linePayload(m_currentLine);
        fwrite(payload.constData(), 1, static_cast<size_t>(payload.size()), stdout);
        fflush(stdout);
    }
    return true;
}

void ExportServer::stop()
{
    if (!m_running && m_clients.isEmpty() && !(m_server && m_server->isListening())) {
        unregisterDBus();
        return;
    }

    for (QLocalSocket *client : std::as_const(m_clients)) {
        client->disconnect(this);
        client->close();
        client->deleteLater();
    }
    m_clients.clear();

    if (m_server) {
        m_server->close();
        QLocalServer::removeServer(m_socketPath);
    }

    unregisterDBus();
    m_running = false;
}

bool ExportServer::isRunning() const
{
    return m_running;
}

QString ExportServer::socketPath() const
{
    return m_socketPath;
}

QString ExportServer::currentLine() const
{
    return m_currentLine;
}

QString ExportServer::title() const
{
    return m_title;
}

QString ExportServer::artist() const
{
    return m_artist;
}

bool ExportServer::playing() const
{
    return m_playing;
}

void ExportServer::onCurrentLineChanged(int)
{
    refreshLineFromSession();
}

void ExportServer::onLyricsChanged()
{
    refreshLineFromSession();
}

void ExportServer::onTrackChanged()
{
    refreshTrackFromPlayer();
}

void ExportServer::onPlaybackChanged(bool)
{
    refreshTrackFromPlayer();
}

void ExportServer::onNewConnection()
{
    if (!m_server) {
        return;
    }
    while (QLocalSocket *client = m_server->nextPendingConnection()) {
        m_clients.append(client);
        connect(client, &QLocalSocket::disconnected,
                this, &ExportServer::onClientDisconnected);
        // Snapshot for clients that connect mid-track.
        client->write(linePayload(m_currentLine));
        client->flush();
    }
}

void ExportServer::onClientDisconnected()
{
    auto *client = qobject_cast<QLocalSocket *>(sender());
    if (!client) {
        return;
    }
    m_clients.removeAll(client);
    client->deleteLater();
}

void ExportServer::refreshLineFromSession()
{
    QString text;
    const auto *lyrics = m_session->lyrics();
    const int index = m_session->currentLineIndex();
    if (lyrics && index >= 0 && index < lyrics->lines.size()) {
        text = lyrics->lines.at(index).content;
    }
    publishLine(text);
}

void ExportServer::refreshTrackFromPlayer()
{
    const TrackInfo track = m_player->currentTrack();
    const bool playing = m_player->isPlaying();
    const bool changed = (m_title != track.title)
        || (m_artist != track.artist)
        || (m_playing != playing);
    m_title = track.title;
    m_artist = track.artist;
    m_playing = playing;
    if (changed && m_dbusRegistered) {
        notifyDBusPropertiesChanged();
    }
}

void ExportServer::publishLine(const QString &line)
{
    if (m_currentLine == line) {
        // Still notify D-Bus track props may have changed independently;
        // line itself unchanged — skip socket/stdout spam.
        return;
    }
    m_currentLine = line;
    emit currentLineTextChanged(m_currentLine);

    if (!m_running && !m_pipeMode) {
        return;
    }

    const QByteArray payload = linePayload(m_currentLine);
    if (m_running) {
        writeToClients(payload);
    }
    if (m_pipeMode) {
        fwrite(payload.constData(), 1, static_cast<size_t>(payload.size()), stdout);
        fflush(stdout);
    }
    if (m_dbusRegistered) {
        notifyDBusPropertiesChanged();
    }
}

void ExportServer::writeToClients(const QByteArray &payload)
{
    for (QLocalSocket *client : std::as_const(m_clients)) {
        if (client->state() == QLocalSocket::ConnectedState) {
            client->write(payload);
            client->flush();
        }
    }
}

bool ExportServer::registerDBus()
{
    if (m_dbusRegistered) {
        return true;
    }
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        return false;
    }
    if (!bus.registerObject(QLatin1String(kDBusPath), this)) {
        return false;
    }
    if (!bus.registerService(QLatin1String(kDBusService))) {
        bus.unregisterObject(QLatin1String(kDBusPath));
        return false;
    }
    m_dbusRegistered = true;
    return true;
}

void ExportServer::unregisterDBus()
{
    if (!m_dbusRegistered) {
        return;
    }
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.unregisterService(QLatin1String(kDBusService));
    bus.unregisterObject(QLatin1String(kDBusPath));
    m_dbusRegistered = false;
}

void ExportServer::notifyDBusPropertiesChanged()
{
    if (!m_dbusRegistered) {
        return;
    }

    QVariantMap changed;
    changed.insert(QStringLiteral("CurrentLine"), m_currentLine);
    changed.insert(QStringLiteral("Title"), m_title);
    changed.insert(QStringLiteral("Artist"), m_artist);
    changed.insert(QStringLiteral("Playing"), m_playing);

    QDBusMessage signal = QDBusMessage::createSignal(
        QLatin1String(kDBusPath),
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("PropertiesChanged"));
    signal << QLatin1String(kDBusInterface)
           << changed
           << QStringList();
    QDBusConnection::sessionBus().send(signal);
}

} // namespace lyricsqt
