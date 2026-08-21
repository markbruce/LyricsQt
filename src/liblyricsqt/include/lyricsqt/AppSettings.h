#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>

namespace lyricsqt {

class AppSettings : public QObject
{
    Q_OBJECT
public:
    explicit AppSettings(QObject *parent = nullptr);
    explicit AppSettings(const QString &organization, const QString &application, QObject *parent = nullptr);

    static QStringList defaultProviderIds();

    bool desktopLyricsEnabled() const;
    void setDesktopLyricsEnabled(bool enabled);

    bool menuBarLyricsEnabled() const;
    void setMenuBarLyricsEnabled(bool enabled);

    bool exportEnabled() const;
    void setExportEnabled(bool enabled);

    QString preferredPlayerId() const;
    void setPreferredPlayerId(const QString &id);

    int globalOffsetMs() const;
    void setGlobalOffsetMs(int offsetMs);

    bool loadLyricsBesideTrack() const;
    void setLoadLyricsBesideTrack(bool enabled);

    QString lyricsSavingPath() const;
    void setLyricsSavingPath(const QString &path);

    double desktopPositionXFactor() const;
    void setDesktopPositionXFactor(double factor);

    double desktopPositionYFactor() const;
    void setDesktopPositionYFactor(double factor);

    bool disableLyricsWhenPaused() const;
    void setDisableLyricsWhenPaused(bool enabled);

    /// Enabled provider ids in search order. Empty setting → defaultProviderIds().
    QStringList enabledProviderIds() const;
    void setEnabledProviderIds(const QStringList &ids);

    QStringList noSearchingTrackIds() const;
    void setNoSearchingTrackIds(const QStringList &ids);
    void addNoSearchingTrackId(const QString &trackId);
    bool isNoSearchingTrackId(const QString &trackId) const;

    bool lyricsFilterEnabled() const;
    void setLyricsFilterEnabled(bool enabled);

    bool lyricsSmartFilterEnabled() const;
    void setLyricsSmartFilterEnabled(bool enabled);

    /// User/keyword filter patterns. Empty setting → LyricsFilter::defaultKeywords().
    QStringList lyricsFilterKeys() const;
    void setLyricsFilterKeys(const QStringList &keys);

    bool preferBilingualLyrics() const;
    void setPreferBilingualLyrics(bool enabled);

    /// XDG autostart toggle; app syncs ~/.config/autostart via AutostartHelper.
    bool autostartEnabled() const;
    void setAutostartEnabled(bool enabled);

    /// Quit app when preferred/active MPRIS player leaves the bus.
    bool quitWithPlayer() const;
    void setQuitWithPlayer(bool enabled);

    bool strictSearchEnabled() const;
    void setStrictSearchEnabled(bool enabled);

signals:
    void changed(const QString &key);

private:
    QSettings m_settings;
};

} // namespace lyricsqt
