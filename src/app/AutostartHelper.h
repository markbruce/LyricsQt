#pragma once

#include <QString>

namespace lyricsqt {

/// Install or remove XDG autostart desktop entry under ~/.config/autostart/.
class AutostartHelper
{
public:
    /// Sync filesystem with the AutostartEnabled preference.
    /// When enabled, writes lyricsqt.desktop with Exec pointing at the running binary.
    /// When disabled, removes that file if present.
    static bool syncFromSettings(bool enabled);

    static QString autostartDesktopPath();
    static QString desktopFileContents(const QString &execPath);
};

} // namespace lyricsqt
