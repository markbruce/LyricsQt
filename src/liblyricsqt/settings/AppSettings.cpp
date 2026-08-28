#include <lyricsqt/AppSettings.h>

#include <lyricsqt/LyricsFilter.h>

#include <cctype>

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
constexpr auto kDesktopLyricsWidth = "DesktopLyricsWidth";
constexpr auto kDesktopLyricsFontPt = "DesktopLyricsFontPt";
constexpr auto kDesktopLyricsLocked = "DesktopLyricsLocked";
constexpr auto kDesktopLyricsUnplayedColor = "DesktopLyricsUnplayedColor";
constexpr auto kDesktopLyricsPlayedColor = "DesktopLyricsPlayedColor";
constexpr auto kDesktopLyricsOutlineColor = "DesktopLyricsOutlineColor";
constexpr auto kDesktopLyricsTextOpacity = "DesktopLyricsTextOpacity";
constexpr auto kDisableLyricsWhenPaused = "DisableLyricsWhenPaused";
constexpr int kDefaultDesktopLyricsWidth = 720;
constexpr int kMinDesktopLyricsWidth = 360;
constexpr int kMaxDesktopLyricsWidth = 2400;
constexpr int kDefaultDesktopLyricsFontPt = 30;
constexpr int kMinDesktopLyricsFontPt = 14;
constexpr int kMaxDesktopLyricsFontPt = 72;
constexpr int kDefaultDesktopLyricsTextOpacity = 100;
constexpr int kMinDesktopLyricsTextOpacity = 10;
constexpr int kMaxDesktopLyricsTextOpacity = 100;

bool looksLikeCssColor(const QString &value)
{
    if (value.isEmpty() || !value.startsWith(QLatin1Char('#'))) {
        return false;
    }
    const int n = value.size() - 1;
    if (n != 6 && n != 8) {
        return false;
    }
    for (int i = 1; i < value.size(); ++i) {
        if (!isxdigit(value[i].toLatin1())) {
            return false;
        }
    }
    return true;
}

QString sanitizeCssColor(const QString &value, const QString &fallback)
{
    const QString trimmed = value.trimmed();
    if (looksLikeCssColor(trimmed)) {
        return trimmed.toLower();
    }
    return fallback;
}
constexpr auto kEnabledProviderIds = "EnabledProviderIds";
constexpr auto kNoSearchingTrackIds = "NoSearchingTrackIds";
constexpr auto kLyricsFilterEnabled = "LyricsFilterEnabled";
constexpr auto kLyricsSmartFilterEnabled = "LyricsSmartFilterEnabled";
constexpr auto kLyricsFilterKeys = "LyricsFilterKeys";
constexpr auto kPreferBilingualLyrics = "PreferBilingualLyrics";
constexpr auto kAutostartEnabled = "AutostartEnabled";
constexpr auto kQuitWithPlayer = "QuitWithPlayer";
constexpr auto kStrictSearchEnabled = "StrictSearchEnabled";

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

QString AppSettings::defaultDesktopLyricsUnplayedColor()
{
    return QStringLiteral("#3d9eff");
}

QString AppSettings::defaultDesktopLyricsPlayedColor()
{
    return QStringLiteral("#ffe03d");
}

QString AppSettings::defaultDesktopLyricsOutlineColor()
{
    return QStringLiteral("#e6000000");
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

int AppSettings::desktopLyricsWidth() const
{
    const int width = m_settings.value(QLatin1String(kDesktopLyricsWidth), kDefaultDesktopLyricsWidth).toInt();
    return qBound(kMinDesktopLyricsWidth, width, kMaxDesktopLyricsWidth);
}

void AppSettings::setDesktopLyricsWidth(int width)
{
    const int clamped = qBound(kMinDesktopLyricsWidth, width, kMaxDesktopLyricsWidth);
    if (desktopLyricsWidth() == clamped) {
        return;
    }
    m_settings.setValue(QLatin1String(kDesktopLyricsWidth), clamped);
    emit changed(QLatin1String(kDesktopLyricsWidth));
}

int AppSettings::desktopLyricsFontPt() const
{
    const int pt = m_settings.value(QLatin1String(kDesktopLyricsFontPt), kDefaultDesktopLyricsFontPt).toInt();
    return qBound(kMinDesktopLyricsFontPt, pt, kMaxDesktopLyricsFontPt);
}

void AppSettings::setDesktopLyricsFontPt(int pointSize)
{
    const int clamped = qBound(kMinDesktopLyricsFontPt, pointSize, kMaxDesktopLyricsFontPt);
    if (desktopLyricsFontPt() == clamped) {
        return;
    }
    m_settings.setValue(QLatin1String(kDesktopLyricsFontPt), clamped);
    emit changed(QLatin1String(kDesktopLyricsFontPt));
}

bool AppSettings::desktopLyricsLocked() const
{
    return m_settings.value(QLatin1String(kDesktopLyricsLocked), false).toBool();
}

void AppSettings::setDesktopLyricsLocked(bool locked)
{
    if (desktopLyricsLocked() == locked) {
        return;
    }
    m_settings.setValue(QLatin1String(kDesktopLyricsLocked), locked);
    emit changed(QLatin1String(kDesktopLyricsLocked));
}

QString AppSettings::desktopLyricsUnplayedColor() const
{
    return sanitizeCssColor(
        m_settings.value(QLatin1String(kDesktopLyricsUnplayedColor)).toString(),
        defaultDesktopLyricsUnplayedColor());
}

void AppSettings::setDesktopLyricsUnplayedColor(const QString &color)
{
    const QString sanitized = sanitizeCssColor(color, defaultDesktopLyricsUnplayedColor());
    if (desktopLyricsUnplayedColor() == sanitized
        && m_settings.contains(QLatin1String(kDesktopLyricsUnplayedColor))) {
        return;
    }
    m_settings.setValue(QLatin1String(kDesktopLyricsUnplayedColor), sanitized);
    emit changed(QLatin1String(kDesktopLyricsUnplayedColor));
}

QString AppSettings::desktopLyricsPlayedColor() const
{
    return sanitizeCssColor(
        m_settings.value(QLatin1String(kDesktopLyricsPlayedColor)).toString(),
        defaultDesktopLyricsPlayedColor());
}

void AppSettings::setDesktopLyricsPlayedColor(const QString &color)
{
    const QString sanitized = sanitizeCssColor(color, defaultDesktopLyricsPlayedColor());
    if (desktopLyricsPlayedColor() == sanitized
        && m_settings.contains(QLatin1String(kDesktopLyricsPlayedColor))) {
        return;
    }
    m_settings.setValue(QLatin1String(kDesktopLyricsPlayedColor), sanitized);
    emit changed(QLatin1String(kDesktopLyricsPlayedColor));
}

QString AppSettings::desktopLyricsOutlineColor() const
{
    const QString raw = m_settings.value(QLatin1String(kDesktopLyricsOutlineColor)).toString().trimmed();
    if (raw.compare(QLatin1String("none"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("none");
    }
    return sanitizeCssColor(raw, defaultDesktopLyricsOutlineColor());
}

void AppSettings::setDesktopLyricsOutlineColor(const QString &color)
{
    const QString trimmed = color.trimmed();
    QString sanitized;
    if (trimmed.compare(QLatin1String("none"), Qt::CaseInsensitive) == 0) {
        sanitized = QStringLiteral("none");
    } else {
        sanitized = sanitizeCssColor(trimmed, defaultDesktopLyricsOutlineColor());
    }
    if (desktopLyricsOutlineColor() == sanitized
        && m_settings.contains(QLatin1String(kDesktopLyricsOutlineColor))) {
        return;
    }
    m_settings.setValue(QLatin1String(kDesktopLyricsOutlineColor), sanitized);
    emit changed(QLatin1String(kDesktopLyricsOutlineColor));
}

int AppSettings::desktopLyricsTextOpacity() const
{
    const int value = m_settings.value(QLatin1String(kDesktopLyricsTextOpacity),
                                       kDefaultDesktopLyricsTextOpacity)
                          .toInt();
    return qBound(kMinDesktopLyricsTextOpacity, value, kMaxDesktopLyricsTextOpacity);
}

void AppSettings::setDesktopLyricsTextOpacity(int percent)
{
    const int clamped = qBound(kMinDesktopLyricsTextOpacity, percent, kMaxDesktopLyricsTextOpacity);
    if (desktopLyricsTextOpacity() == clamped) {
        return;
    }
    m_settings.setValue(QLatin1String(kDesktopLyricsTextOpacity), clamped);
    emit changed(QLatin1String(kDesktopLyricsTextOpacity));
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

QStringList AppSettings::noSearchingTrackIds() const
{
    return m_settings.value(QLatin1String(kNoSearchingTrackIds)).toStringList();
}

void AppSettings::setNoSearchingTrackIds(const QStringList &ids)
{
    if (noSearchingTrackIds() == ids) {
        return;
    }
    m_settings.setValue(QLatin1String(kNoSearchingTrackIds), ids);
    emit changed(QLatin1String(kNoSearchingTrackIds));
}

void AppSettings::addNoSearchingTrackId(const QString &trackId)
{
    if (trackId.isEmpty() || isNoSearchingTrackId(trackId)) {
        return;
    }
    QStringList ids = noSearchingTrackIds();
    ids.append(trackId);
    setNoSearchingTrackIds(ids);
}

bool AppSettings::isNoSearchingTrackId(const QString &trackId) const
{
    if (trackId.isEmpty()) {
        return false;
    }
    return noSearchingTrackIds().contains(trackId);
}

bool AppSettings::lyricsFilterEnabled() const
{
    return m_settings.value(QLatin1String(kLyricsFilterEnabled), true).toBool();
}

void AppSettings::setLyricsFilterEnabled(bool enabled)
{
    if (lyricsFilterEnabled() == enabled) {
        return;
    }
    m_settings.setValue(QLatin1String(kLyricsFilterEnabled), enabled);
    emit changed(QLatin1String(kLyricsFilterEnabled));
}

bool AppSettings::lyricsSmartFilterEnabled() const
{
    return m_settings.value(QLatin1String(kLyricsSmartFilterEnabled), true).toBool();
}

void AppSettings::setLyricsSmartFilterEnabled(bool enabled)
{
    if (lyricsSmartFilterEnabled() == enabled) {
        return;
    }
    m_settings.setValue(QLatin1String(kLyricsSmartFilterEnabled), enabled);
    emit changed(QLatin1String(kLyricsSmartFilterEnabled));
}

QStringList AppSettings::lyricsFilterKeys() const
{
    const QVariant raw = m_settings.value(QLatin1String(kLyricsFilterKeys));
    if (!raw.isValid()) {
        return LyricsFilter::defaultKeywords();
    }
    return raw.toStringList();
}

void AppSettings::setLyricsFilterKeys(const QStringList &keys)
{
    if (lyricsFilterKeys() == keys) {
        return;
    }
    m_settings.setValue(QLatin1String(kLyricsFilterKeys), keys);
    emit changed(QLatin1String(kLyricsFilterKeys));
}

bool AppSettings::preferBilingualLyrics() const
{
    return m_settings.value(QLatin1String(kPreferBilingualLyrics), true).toBool();
}

void AppSettings::setPreferBilingualLyrics(bool enabled)
{
    if (preferBilingualLyrics() == enabled) {
        return;
    }
    m_settings.setValue(QLatin1String(kPreferBilingualLyrics), enabled);
    emit changed(QLatin1String(kPreferBilingualLyrics));
}

bool AppSettings::autostartEnabled() const
{
    return m_settings.value(QLatin1String(kAutostartEnabled), false).toBool();
}

void AppSettings::setAutostartEnabled(bool enabled)
{
    if (autostartEnabled() == enabled) {
        return;
    }
    m_settings.setValue(QLatin1String(kAutostartEnabled), enabled);
    emit changed(QLatin1String(kAutostartEnabled));
}

bool AppSettings::quitWithPlayer() const
{
    return m_settings.value(QLatin1String(kQuitWithPlayer), false).toBool();
}

void AppSettings::setQuitWithPlayer(bool enabled)
{
    if (quitWithPlayer() == enabled) {
        return;
    }
    m_settings.setValue(QLatin1String(kQuitWithPlayer), enabled);
    emit changed(QLatin1String(kQuitWithPlayer));
}

bool AppSettings::strictSearchEnabled() const
{
    return m_settings.value(QLatin1String(kStrictSearchEnabled), false).toBool();
}

void AppSettings::setStrictSearchEnabled(bool enabled)
{
    if (strictSearchEnabled() == enabled) {
        return;
    }
    m_settings.setValue(QLatin1String(kStrictSearchEnabled), enabled);
    emit changed(QLatin1String(kStrictSearchEnabled));
}

} // namespace lyricsqt
