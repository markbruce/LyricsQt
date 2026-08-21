#include <lyricsqt/AppSettings.h>

#include <QStandardPaths>

namespace lyricsqt {

namespace {

constexpr auto kDesktopLyricsEnabled = "DesktopLyricsEnabled";
constexpr auto kMenuBarLyricsEnabled = "MenuBarLyricsEnabled";
constexpr auto kExportEnabled = "ExportEnabled";
constexpr auto kPreferredPlayerId = "PreferredPlayerId";
constexpr auto kGlobalOffsetMs = "GlobalLyricsOffset";
constexpr auto kLoadLyricsBesideTrack = "LoadLyricsBesideTrack";
constexpr auto kLyricsSavingPath = "LyricsSavingPath";

} // namespace

AppSettings::AppSettings(QObject *parent)
    : AppSettings(QStringLiteral("lyricsqt"), QStringLiteral("LyricsQt"), parent)
{
}

AppSettings::AppSettings(const QString &organization, const QString &application, QObject *parent)
    : QObject(parent)
    , m_settings(organization, application)
{
}

bool AppSettings::desktopLyricsEnabled() const
{
    return m_settings.value(QLatin1String(kDesktopLyricsEnabled), true).toBool();
}

void AppSettings::setDesktopLyricsEnabled(bool enabled)
{
    if (desktopLyricsEnabled() == enabled) {
        return;
    }
    m_settings.setValue(QLatin1String(kDesktopLyricsEnabled), enabled);
    emit changed(QLatin1String(kDesktopLyricsEnabled));
}

bool AppSettings::menuBarLyricsEnabled() const
{
    return m_settings.value(QLatin1String(kMenuBarLyricsEnabled), false).toBool();
}

void AppSettings::setMenuBarLyricsEnabled(bool enabled)
{
    if (menuBarLyricsEnabled() == enabled) {
        return;
    }
    m_settings.setValue(QLatin1String(kMenuBarLyricsEnabled), enabled);
    emit changed(QLatin1String(kMenuBarLyricsEnabled));
}

bool AppSettings::exportEnabled() const
{
    return m_settings.value(QLatin1String(kExportEnabled), false).toBool();
}

void AppSettings::setExportEnabled(bool enabled)
{
    if (exportEnabled() == enabled) {
        return;
    }
    m_settings.setValue(QLatin1String(kExportEnabled), enabled);
    emit changed(QLatin1String(kExportEnabled));
}

QString AppSettings::preferredPlayerId() const
{
    return m_settings.value(QLatin1String(kPreferredPlayerId)).toString();
}

void AppSettings::setPreferredPlayerId(const QString &id)
{
    if (preferredPlayerId() == id) {
        return;
    }
    m_settings.setValue(QLatin1String(kPreferredPlayerId), id);
    emit changed(QLatin1String(kPreferredPlayerId));
}

int AppSettings::globalOffsetMs() const
{
    return m_settings.value(QLatin1String(kGlobalOffsetMs), 0).toInt();
}

void AppSettings::setGlobalOffsetMs(int offsetMs)
{
    if (globalOffsetMs() == offsetMs) {
        return;
    }
    m_settings.setValue(QLatin1String(kGlobalOffsetMs), offsetMs);
    emit changed(QLatin1String(kGlobalOffsetMs));
}

bool AppSettings::loadLyricsBesideTrack() const
{
    return m_settings.value(QLatin1String(kLoadLyricsBesideTrack), true).toBool();
}

void AppSettings::setLoadLyricsBesideTrack(bool enabled)
{
    if (loadLyricsBesideTrack() == enabled) {
        return;
    }
    m_settings.setValue(QLatin1String(kLoadLyricsBesideTrack), enabled);
    emit changed(QLatin1String(kLoadLyricsBesideTrack));
}

QString AppSettings::lyricsSavingPath() const
{
    const QString custom = m_settings.value(QLatin1String(kLyricsSavingPath)).toString();
    if (!custom.isEmpty()) {
        return custom;
    }
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/lyrics");
}

void AppSettings::setLyricsSavingPath(const QString &path)
{
    if (m_settings.value(QLatin1String(kLyricsSavingPath)).toString() == path) {
        return;
    }
    m_settings.setValue(QLatin1String(kLyricsSavingPath), path);
    emit changed(QLatin1String(kLyricsSavingPath));
}

} // namespace lyricsqt
