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
    void searchLyricsRequested();
    void wrongLyricsRequested();

private:
    void onCurrentLineChanged(int index);
    void onLyricsChanged();
    void onSettingsChanged(const QString &key);
    void updateTooltip();
    void toggleDesktopLyrics();
    void toggleTrayLine();
    void toggleDesktopLock();
    void adjustOffset(int deltaMs);

    lyricsqt::AppSettings *m_settings = nullptr;
    lyricsqt::LyricsSession *m_session = nullptr;
    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_toggleDesktopAction = nullptr;
    QAction *m_toggleTrayLineAction = nullptr;
    QAction *m_lockDesktopAction = nullptr;
};
