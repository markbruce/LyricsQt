#pragma once

#include <QObject>
#include <QString>

class QLocalServer;
class QLocalSocket;

namespace lyricsqt {

class LyricsSession;
class PlayerService;

/// Panel / extension IPC: unix socket line protocol, optional stdout (--pipe),
/// and session D-Bus interface org.lyricsqt.Export.
class ExportServer : public QObject
{
    Q_OBJECT
public:
    ExportServer(LyricsSession *session, PlayerService *player, QObject *parent = nullptr);
    ~ExportServer() override;

    static QString defaultSocketPath();

    void setPipeMode(bool enabled);
    bool pipeMode() const;

    void setSocketPath(const QString &path);
    QString socketPath() const;

    bool start();
    void stop();
    bool isRunning() const;

    QString currentLine() const;
    QString title() const;
    QString artist() const;
    bool playing() const;

signals:
    void currentLineTextChanged(const QString &line);

private slots:
    void onCurrentLineChanged(int index);
    void onLyricsChanged();
    void onTrackChanged();
    void onPlaybackChanged(bool playing);
    void onNewConnection();
    void onClientDisconnected();

private:
    void refreshLineFromSession();
    void refreshTrackFromPlayer();
    void publishLine(const QString &line);
    void writeToClients(const QByteArray &payload);
    void notifyDBusPropertiesChanged();
    bool registerDBus();
    void unregisterDBus();

    LyricsSession *m_session = nullptr;
    PlayerService *m_player = nullptr;
    QLocalServer *m_server = nullptr;
    QList<QLocalSocket *> m_clients;
    QString m_socketPath;
    bool m_pipeMode = false;
    bool m_running = false;
    bool m_dbusRegistered = false;

    QString m_currentLine;
    QString m_title;
    QString m_artist;
    bool m_playing = false;
};

} // namespace lyricsqt
