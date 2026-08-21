#pragma once

#include <QPoint>
#include <QWidget>

class KaraokeLyricLabel;
class QTimer;

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
    void paintEvent(QPaintEvent *event) override;

private:
    void onCurrentLineChanged(int index);
    void onLyricsChanged();
    void onSettingsChanged(const QString &key);
    void refreshText();
    void updateKaraokeProgress();
    void updateVisibility();
    void applyPositionFromSettings();
    void savePositionToSettings();
    qreal lineProgress() const;

    lyricsqt::AppSettings *m_settings = nullptr;
    lyricsqt::LyricsSession *m_session = nullptr;
    lyricsqt::PlayerService *m_player = nullptr;
    KaraokeLyricLabel *m_primary = nullptr;
    KaraokeLyricLabel *m_secondary = nullptr;
    QTimer *m_progressTimer = nullptr;
    QPoint m_dragOffset;
    bool m_dragging = false;
};
