#pragma once

#include <QPoint>
#include <QWidget>

class KaraokeLyricLabel;
class QAbstractButton;
class QTimer;
class QWidget;

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
    ~DesktopLyricsWindow() override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class DragMode {
        None,
        Move,
        ResizeLeft,
        ResizeRight,
    };

    void onCurrentLineChanged(int index);
    void onLyricsChanged();
    void onSettingsChanged(const QString &key);
    void refreshText();
    void updateKaraokeProgress();
    void updateVisibility();
    void applyPositionFromSettings();
    void savePositionToSettings();
    void applyWidthFromSettings();
    void saveWidthToSettings();
    void applyFontsFromSettings();
    void applyColorsFromSettings();
    void applyLockedState();
    void updateHeightToContent();
    void positionLockOverlay();
    void syncLockOverlayVisibility();
    void updateOverlayControlsForLockState();
    void adjustFontPt(int delta);
    bool pointerOverLyricsOrLock() const;
    void toggleLocked();
    DragMode hitTest(const QPoint &pos) const;
    void updateHoverCursor(const QPoint &pos);
    qreal lineProgress() const;
    bool isLocked() const;

    lyricsqt::AppSettings *m_settings = nullptr;
    lyricsqt::LyricsSession *m_session = nullptr;
    lyricsqt::PlayerService *m_player = nullptr;
    KaraokeLyricLabel *m_primary = nullptr;
    KaraokeLyricLabel *m_secondary = nullptr;
    QWidget *m_lockOverlay = nullptr;
    QAbstractButton *m_lockButton = nullptr;
    QAbstractButton *m_fontSmallerButton = nullptr;
    QAbstractButton *m_fontLargerButton = nullptr;
    QTimer *m_progressTimer = nullptr;
    QPoint m_dragOffset;
    QPoint m_resizeOriginGlobal;
    int m_resizeOriginWidth = 0;
    int m_resizeOriginX = 0;
    DragMode m_dragMode = DragMode::None;
    bool m_hovered = false;
};
