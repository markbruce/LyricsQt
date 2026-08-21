#include "AutostartHelper.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace lyricsqt {

QString AutostartHelper::autostartDesktopPath()
{
    const QString config = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return config + QStringLiteral("/autostart/lyricsqt.desktop");
}

QString AutostartHelper::desktopFileContents(const QString &execPath)
{
    // Quote path for spaces; desktop Exec allows quoted argv0.
    QString exec = execPath;
    if (exec.contains(QLatin1Char(' ')) || exec.contains(QLatin1Char('\t'))) {
        exec = QLatin1Char('"') + exec + QLatin1Char('"');
    }

    return QStringLiteral(
               "[Desktop Entry]\n"
               "Type=Application\n"
               "Name=LyricsQt\n"
               "GenericName=Lyrics\n"
               "Comment=Desktop lyrics synchronized with your music player\n"
               "Exec=%1\n"
               "Icon=lyricsqt\n"
               "Terminal=false\n"
               "Categories=AudioVideo;Audio;\n"
               "StartupNotify=false\n"
               "X-GNOME-Autostart-enabled=true\n")
        .arg(exec);
}

bool AutostartHelper::syncFromSettings(bool enabled)
{
    const QString path = autostartDesktopPath();
    if (!enabled) {
        if (!QFile::exists(path)) {
            return true;
        }
        return QFile::remove(path);
    }

    const QString execPath = QCoreApplication::applicationFilePath();
    if (execPath.isEmpty()) {
        return false;
    }

    const QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath())) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    const QByteArray data = desktopFileContents(execPath).toUtf8();
    return file.write(data) == data.size();
}

} // namespace lyricsqt
