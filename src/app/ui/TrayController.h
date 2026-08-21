#pragma once

#include <QObject>

class QAction;
class QMenu;
class QSystemTrayIcon;

namespace lyricsqt {
class AppSettings;
class LyricsSession;
}

class TrayController : public QObject
{
    Q_OBJECT
public:
    TrayController(lyricsqt::AppSettings *settings,
                   lyricsqt::LyricsSession *session,
                   QObject *parent = nullptr);
    ~TrayController() override;

signals:
    void showHudRequested();
    void preferencesRequested();

private:
    void onCurrentLineChanged(int index);
    void onLyricsChanged();
    void onSettingsChanged(const QString &key);
    void updateTooltip();
    void toggleDesktopLyrics();
    void adjustOffset(int deltaMs);

    lyricsqt::AppSettings *m_settings = nullptr;
    lyricsqt::LyricsSession *m_session = nullptr;
    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_toggleDesktopAction = nullptr;
};
