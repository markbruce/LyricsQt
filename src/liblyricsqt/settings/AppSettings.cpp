#include <lyricsqt/AppSettings.h>

namespace lyricsqt {

namespace {

constexpr auto kDesktopLyricsEnabled = "DesktopLyricsEnabled";
constexpr auto kMenuBarLyricsEnabled = "MenuBarLyricsEnabled";
constexpr auto kExportEnabled = "ExportEnabled";
constexpr auto kPreferredPlayerId = "PreferredPlayerId";
constexpr auto kGlobalOffsetMs = "GlobalLyricsOffset";

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

} // namespace lyricsqt
