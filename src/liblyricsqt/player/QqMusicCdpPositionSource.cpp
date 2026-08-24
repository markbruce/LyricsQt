#include "QqMusicCdpPositionSource.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

namespace lyricsqt {

namespace {

constexpr qint64 kFreshMs = 2000;

} // namespace

QqMusicCdpPositionSource::QqMusicCdpPositionSource(QObject *parent)
    : QObject(parent)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &QqMusicCdpPositionSource::onReadyRead);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &QqMusicCdpPositionSource::onProcessFinished);
    connect(&m_process, &QProcess::errorOccurred,
            this, &QqMusicCdpPositionSource::onProcessError);
}

QqMusicCdpPositionSource::~QqMusicCdpPositionSource()
{
    stopProcess();
}

void QqMusicCdpPositionSource::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    if (m_enabled) {
        startProcess();
    } else {
        stopProcess();
    }
}

bool QqMusicCdpPositionSource::isEnabled() const
{
    return m_enabled;
}

bool QqMusicCdpPositionSource::hasFreshPosition() const
{
    if (!m_available || m_lastUpdateMs == 0) {
        return false;
    }
    return (QDateTime::currentMSecsSinceEpoch() - m_lastUpdateMs) <= kFreshMs;
}

double QqMusicCdpPositionSource::positionSec() const
{
    return m_positionSec;
}

double QqMusicCdpPositionSource::durationSec() const
{
    return m_durationSec;
}

bool QqMusicCdpPositionSource::paused() const
{
    return m_paused;
}

QString QqMusicCdpPositionSource::resolveScriptPath()
{
    const QByteArray env = qgetenv("LYRICSQT_QQMUSIC_CDP_SCRIPT");
    if (!env.isEmpty()) {
        return QString::fromLocal8Bit(env);
    }

    const QDir appDir(QCoreApplication::applicationDirPath());
    const QStringList candidates = {
        appDir.absoluteFilePath(QStringLiteral("qqmusic_cdp_position.py")),
        appDir.absoluteFilePath(QStringLiteral("../share/lyricsqt/qqmusic_cdp_position.py")),
        appDir.absoluteFilePath(QStringLiteral("../../scripts/qqmusic_cdp_position.py")),
        appDir.absoluteFilePath(QStringLiteral("../../../scripts/qqmusic_cdp_position.py")),
        appDir.absoluteFilePath(QStringLiteral("../../../../scripts/qqmusic_cdp_position.py")),
#ifdef LYRICSQT_SOURCE_DIR
        QStringLiteral(LYRICSQT_SOURCE_DIR "/scripts/qqmusic_cdp_position.py"),
#endif
    };
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    return {};
}

void QqMusicCdpPositionSource::startProcess()
{
    if (m_process.state() != QProcess::NotRunning) {
        return;
    }

    const QString script = resolveScriptPath();
    if (script.isEmpty()) {
        if (m_available) {
            m_available = false;
            emit availabilityChanged(false);
        }
        return;
    }

    m_process.start(QStringLiteral("python3"), {script});
}

void QqMusicCdpPositionSource::stopProcess()
{
    if (m_process.state() == QProcess::NotRunning) {
        if (m_available) {
            m_available = false;
            emit availabilityChanged(false);
        }
        return;
    }
    m_process.kill();
    m_process.waitForFinished(1500);
    if (m_available) {
        m_available = false;
        emit availabilityChanged(false);
    }
}

void QqMusicCdpPositionSource::onReadyRead()
{
    while (m_process.canReadLine()) {
        handleLine(m_process.readLine().trimmed());
    }
}

void QqMusicCdpPositionSource::handleLine(const QByteArray &line)
{
    if (line.isEmpty()) {
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(line);
    if (!doc.isObject()) {
        return;
    }
    const QJsonObject obj = doc.object();
    if (!obj.value(QStringLiteral("ok")).toBool()) {
        return;
    }
    if (!obj.contains(QStringLiteral("currentTime"))) {
        // connected handshake line
        return;
    }

    m_positionSec = obj.value(QStringLiteral("currentTime")).toDouble();
    m_durationSec = obj.value(QStringLiteral("duration")).toDouble();
    m_paused = obj.value(QStringLiteral("paused")).toBool(true);
    m_lastUpdateMs = QDateTime::currentMSecsSinceEpoch();

    if (!m_available) {
        m_available = true;
        emit availabilityChanged(true);
    }
    emit positionUpdated(m_positionSec, m_durationSec, m_paused);
}

void QqMusicCdpPositionSource::onProcessFinished(int, QProcess::ExitStatus)
{
    if (m_available) {
        m_available = false;
        emit availabilityChanged(false);
    }
    if (m_enabled) {
        // Retry shortly — QQ Music may restart or inspector may drop.
        QTimer::singleShot(1500, this, [this]() {
            if (m_enabled && m_process.state() == QProcess::NotRunning) {
                startProcess();
            }
        });
    }
}

void QqMusicCdpPositionSource::onProcessError(QProcess::ProcessError)
{
    // finished handler covers restart; keep silent here.
}

} // namespace lyricsqt
