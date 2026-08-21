#include <lyricsqt/AppSettings.h>

#include <QStandardPaths>
#include <QtGlobal>

namespace lyricsqt {

namespace {

constexpr auto kDesktopLyricsEnabled = "DesktopLyricsEnabled";
constexpr auto kMenuBarLyricsEnabled = "MenuBarLyricsEnabled";
constexpr auto kExportEnabled = "ExportEnabled";
constexpr auto kPreferredPlayerId = "PreferredPlayerId";
constexpr auto kGlobalOffsetMs = "GlobalLyricsOffset";
constexpr auto kLoadLyricsBesideTrack = "LoadLyricsBesideTrack";
constexpr auto kLyricsSavingPath = "LyricsSavingPath";
constexpr auto kDesktopPositionXFactor = "DesktopLyricsXPositionFactor";
constexpr auto kDesktopPositionYFactor = "DesktopLyricsYPositionFactor";
constexpr auto kDisableLyricsWhenPaused = "DisableLyricsWhenPaused";
constexpr auto kEnabledProviderIds = "EnabledProviderIds";

double clampFactor(double factor)
{
    if (factor < 0.0) {
        return 0.0;
    }
    if (factor > 1.0) {
        return 1.0;
    }
    return factor;
}

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

QStringList AppSettings::defaultProviderIds()
{
    return {
        QStringLiteral("lrclib"),
        QStringLiteral("netease"),
        QStringLiteral("qq"),
        QStringLiteral("kugou"),
    };
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

double AppSettings::desktopPositionXFactor() const
{
    return m_settings.value(QLatin1String(kDesktopPositionXFactor), 0.5).toDouble();
}

void AppSettings::setDesktopPositionXFactor(double factor)
{
    const double clamped = clampFactor(factor);
    if (qFuzzyCompare(desktopPositionXFactor() + 1.0, clamped + 1.0)) {
        return;
    }
    m_settings.setValue(QLatin1String(kDesktopPositionXFactor), clamped);
    emit changed(QLatin1String(kDesktopPositionXFactor));
}

double AppSettings::desktopPositionYFactor() const
{
    return m_settings.value(QLatin1String(kDesktopPositionYFactor), 0.5).toDouble();
}

void AppSettings::setDesktopPositionYFactor(double factor)
{
    const double clamped = clampFactor(factor);
    if (qFuzzyCompare(desktopPositionYFactor() + 1.0, clamped + 1.0)) {
        return;
    }
    m_settings.setValue(QLatin1String(kDesktopPositionYFactor), clamped);
    emit changed(QLatin1String(kDesktopPositionYFactor));
}

bool AppSettings::disableLyricsWhenPaused() const
{
    return m_settings.value(QLatin1String(kDisableLyricsWhenPaused), false).toBool();
}

void AppSettings::setDisableLyricsWhenPaused(bool enabled)
{
    if (disableLyricsWhenPaused() == enabled) {
        return;
    }
    m_settings.setValue(QLatin1String(kDisableLyricsWhenPaused), enabled);
    emit changed(QLatin1String(kDisableLyricsWhenPaused));
}

QStringList AppSettings::enabledProviderIds() const
{
    const QVariant raw = m_settings.value(QLatin1String(kEnabledProviderIds));
    if (!raw.isValid()) {
        return defaultProviderIds();
    }
    const QStringList ids = raw.toStringList();
    if (ids.isEmpty()) {
        return defaultProviderIds();
    }
    return ids;
}

void AppSettings::setEnabledProviderIds(const QStringList &ids)
{
    if (enabledProviderIds() == ids) {
        return;
    }
    m_settings.setValue(QLatin1String(kEnabledProviderIds), ids);
    emit changed(QLatin1String(kEnabledProviderIds));
}

} // namespace lyricsqt
