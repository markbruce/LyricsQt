#pragma once

#include <QPoint>
#include <QWidget>

class QLabel;

namespace lyricsqt {
class AppSettings;
class LyricsSession;
class PlayerService;
}

class DesktopLyricsWindow : public QWidget
{
    Q_OBJECT
public:
    DesktopLyricsWindow(lyricsqt::AppSettings *settings,
                        lyricsqt::LyricsSession *session,
                        lyricsqt::PlayerService *player,
                        QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void onCurrentLineChanged(int index);
    void onLyricsChanged();
    void onSettingsChanged(const QString &key);
    void refreshText();
    void updateVisibility();
    void applyPositionFromSettings();
    void savePositionToSettings();

    lyricsqt::AppSettings *m_settings = nullptr;
    lyricsqt::LyricsSession *m_session = nullptr;
    lyricsqt::PlayerService *m_player = nullptr;
    QLabel *m_primary = nullptr;
    QLabel *m_secondary = nullptr;
    QPoint m_dragOffset;
    bool m_dragging = false;
};
