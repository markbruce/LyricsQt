#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

namespace lyricsqt {

// Reads QQ Music Electron <audio>.currentTime via Node inspector (SIGUSR1 → :9229).
class QqMusicCdpPositionSource : public QObject
{
    Q_OBJECT
public:
    explicit QqMusicCdpPositionSource(QObject *parent = nullptr);
    ~QqMusicCdpPositionSource() override;

    void setEnabled(bool enabled);
    bool isEnabled() const;
    bool hasFreshPosition() const;
    double positionSec() const;
    double durationSec() const;
    bool paused() const;

    static QString resolveScriptPath();

signals:
    void positionUpdated(double positionSec, double durationSec, bool paused);
    void availabilityChanged(bool available);

private slots:
    void onReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

private:
    void startProcess();
    void stopProcess();
    void handleLine(const QByteArray &line);

    QProcess m_process;
    bool m_enabled = false;
    bool m_available = false;
    double m_positionSec = 0.0;
    double m_durationSec = 0.0;
    bool m_paused = true;
    qint64 m_lastUpdateMs = 0;
};

} // namespace lyricsqt
