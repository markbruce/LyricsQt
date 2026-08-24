#include "DesktopLyricsWindow.h"

#include "KaraokeLyricLabel.h"

#include <lyricsqt/AppSettings.h>
#include <lyricsqt/LyricsSession.h>
#include <lyricsqt/PlayerService.h>

#include <QAbstractButton>
#include <QCursor>
#include <QEnterEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QMouseEvent>
#include <QMoveEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QResizeEvent>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace {

constexpr int kEdgeResizePx = 10;
constexpr int kHoverBackgroundAlpha = 72;
constexpr int kCtrlButtonSize = 28;
constexpr int kOverlayGap = 4;
constexpr int kLockMargin = 6;
constexpr int kMinPanelWidth = 360;
constexpr int kMaxPanelWidth = 2400;

class RoundGlyphButton : public QAbstractButton
{
public:
    RoundGlyphButton(const QString &glyph, QWidget *parent = nullptr)
        : QAbstractButton(parent)
        , m_glyph(glyph)
    {
        setFixedSize(kCtrlButtonSize, kCtrlButtonSize);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_TranslucentBackground);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        const QRectF r = QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, isDown() ? 160 : 110));
        p.drawEllipse(r);
        p.setPen(QColor(255, 255, 255, 220));
        QFont f = font();
        f.setBold(true);
        f.setPixelSize(m_glyph.size() > 1 ? 11 : 13);
        p.setFont(f);
        p.drawText(rect(), Qt::AlignCenter, m_glyph);
    }

private:
    QString m_glyph;
};

class LockToggleButton : public QAbstractButton
{
public:
    explicit LockToggleButton(QWidget *parent = nullptr)
        : QAbstractButton(parent)
    {
        setCheckable(true);
        setFixedSize(kCtrlButtonSize, kCtrlButtonSize);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_TranslucentBackground);
        setToolTip(QStringLiteral("Lock / unlock desktop lyrics"));
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QRectF r = QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, isChecked() ? 140 : 110));
        p.drawEllipse(r);

        const QColor icon = QColor(255, 255, 255, isChecked() ? 235 : 200);
        p.setPen(QPen(icon, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);

        const QRectF body(r.left() + 7.5, r.center().y() + 0.5, 11.0, 8.5);
        const QRectF shackle(r.left() + 9.0, r.top() + 5.0, 8.0, 8.0);
        if (isChecked()) {
            p.drawArc(shackle, 0, 180 * 16);
        } else {
            p.save();
            p.translate(2.0, -0.5);
            p.drawArc(shackle, 20 * 16, 150 * 16);
            p.restore();
        }
        p.setBrush(icon);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(body, 1.8, 1.8);

        p.setBrush(QColor(0, 0, 0, 180));
        p.drawEllipse(QPointF(body.center().x(), body.top() + 3.2), 1.3, 1.3);
        p.drawRect(QRectF(body.center().x() - 0.7, body.top() + 3.8, 1.4, 2.6));
    }
};

} // namespace

DesktopLyricsWindow::DesktopLyricsWindow(lyricsqt::AppSettings *settings,
                                         lyricsqt::LyricsSession *session,
                                         lyricsqt::PlayerService *player,
                                         QWidget *parent)
    : QWidget(parent,
              Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint
                  | Qt::WindowDoesNotAcceptFocus)
    , m_settings(settings)
    , m_session(session)
    , m_player(player)
{
    Q_ASSERT(m_settings);
    Q_ASSERT(m_session);
    Q_ASSERT(m_player);

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_Hover, true);
    setAttribute(Qt::WA_X11NetWmWindowTypeNotification, true);
    setMouseTracking(true);
    setWindowTitle(QStringLiteral("LyricsQt Desktop Lyrics"));
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    // Ensure any previous input mask from older builds is cleared.
    clearMask();

    m_primary = new KaraokeLyricLabel(this);
    m_primary->setMouseTracking(true);

    m_secondary = new KaraokeLyricLabel(this);
    m_secondary->setUnplayedColor(QColor(0x9B, 0xC9, 0xFF));
    m_secondary->setPlayedColor(QColor(0x9B, 0xC9, 0xFF));
    m_secondary->setProgress(0.0);
    m_secondary->setMouseTracking(true);

    // Separate top-level chrome so lyrics stay painted while locked
    // (WindowTransparentForInput on the lyrics window would block a child lock).
    m_lockOverlay = new QWidget(nullptr,
                                Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint
                                    | Qt::WindowDoesNotAcceptFocus);
    m_lockOverlay->setAttribute(Qt::WA_TranslucentBackground);
    m_lockOverlay->setAttribute(Qt::WA_ShowWithoutActivating);
    m_lockOverlay->installEventFilter(this);

    auto *bar = new QHBoxLayout(m_lockOverlay);
    bar->setContentsMargins(0, 0, 0, 0);
    bar->setSpacing(kOverlayGap);

    m_fontSmallerButton = new RoundGlyphButton(QStringLiteral("A-"), m_lockOverlay);
    m_fontSmallerButton->setToolTip(QStringLiteral("Smaller font (or scroll down)"));
    connect(m_fontSmallerButton, &QAbstractButton::clicked, this, [this]() {
        adjustFontPt(-2);
    });

    m_lockButton = new LockToggleButton(m_lockOverlay);
    connect(m_lockButton, &QAbstractButton::clicked, this, [this]() {
        toggleLocked();
    });

    m_fontLargerButton = new RoundGlyphButton(QStringLiteral("A+"), m_lockOverlay);
    m_fontLargerButton->setToolTip(QStringLiteral("Larger font (or scroll up)"));
    connect(m_fontLargerButton, &QAbstractButton::clicked, this, [this]() {
        adjustFontPt(2);
    });

    bar->addWidget(m_fontSmallerButton);
    bar->addWidget(m_lockButton);
    bar->addWidget(m_fontLargerButton);
    m_lockOverlay->hide();
    updateOverlayControlsForLockState();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 18, 28, 18);
    layout->setSpacing(8);
    layout->addWidget(m_primary);
    layout->addWidget(m_secondary);

    setMinimumWidth(kMinPanelWidth);
    setMaximumWidth(kMaxPanelWidth);
    setMinimumHeight(88);
    applyFontsFromSettings();
    applyWidthFromSettings();
    updateHeightToContent();

    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(33);
    connect(m_progressTimer, &QTimer::timeout, this, &DesktopLyricsWindow::updateKaraokeProgress);

    connect(m_session, &lyricsqt::LyricsSession::currentLineChanged,
            this, &DesktopLyricsWindow::onCurrentLineChanged);
    connect(m_session, &lyricsqt::LyricsSession::lyricsChanged,
            this, &DesktopLyricsWindow::onLyricsChanged);
    connect(m_settings, &lyricsqt::AppSettings::changed,
            this, &DesktopLyricsWindow::onSettingsChanged);
    connect(m_player, &lyricsqt::PlayerService::playbackChanged, this, [this](bool playing) {
        updateVisibility();
        if (playing) {
            m_progressTimer->start();
        } else {
            m_progressTimer->stop();
            updateKaraokeProgress();
        }
    });

    refreshText();
    applyPositionFromSettings();
    applyLockedState();
    updateVisibility();
    if (m_player->isPlaying()) {
        m_progressTimer->start();
    }
}

DesktopLyricsWindow::~DesktopLyricsWindow()
{
    if (m_lockOverlay) {
        m_lockOverlay->hide();
        delete m_lockOverlay;
        m_lockOverlay = nullptr;
        m_lockButton = nullptr;
        m_fontSmallerButton = nullptr;
        m_fontLargerButton = nullptr;
    }
}

bool DesktopLyricsWindow::isLocked() const
{
    return m_settings->desktopLyricsLocked();
}

void DesktopLyricsWindow::toggleLocked()
{
    m_settings->setDesktopLyricsLocked(!isLocked());
}

void DesktopLyricsWindow::mousePressEvent(QMouseEvent *event)
{
    if (isLocked()) {
        event->ignore();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        m_dragMode = hitTest(event->pos());
        if (m_dragMode == DragMode::None) {
            m_dragMode = DragMode::Move;
        }
        if (m_dragMode == DragMode::Move) {
            m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        } else {
            m_resizeOriginGlobal = event->globalPosition().toPoint();
            m_resizeOriginWidth = width();
            m_resizeOriginX = x();
        }
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void DesktopLyricsWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (isLocked()) {
        unsetCursor();
        event->ignore();
        return;
    }
    if (m_dragMode == DragMode::Move && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
        return;
    }
    if ((m_dragMode == DragMode::ResizeLeft || m_dragMode == DragMode::ResizeRight)
        && (event->buttons() & Qt::LeftButton)) {
        const int delta = event->globalPosition().toPoint().x() - m_resizeOriginGlobal.x();
        int newWidth = m_resizeOriginWidth;
        int newX = m_resizeOriginX;
        if (m_dragMode == DragMode::ResizeRight) {
            newWidth = m_resizeOriginWidth + delta;
        } else {
            newWidth = m_resizeOriginWidth - delta;
            newX = m_resizeOriginX + delta;
        }
        newWidth = qBound(kMinPanelWidth, newWidth, kMaxPanelWidth);
        if (m_dragMode == DragMode::ResizeLeft) {
            newX = m_resizeOriginX + (m_resizeOriginWidth - newWidth);
        }
        // Do not use setFixedWidth: it raises minimumWidth to the current width
        // and makes shrinking impossible (only grow would work).
        resize(newWidth, height());
        updateHeightToContent();
        move(newX, y());
        event->accept();
        return;
    }

    updateHoverCursor(event->pos());
    QWidget::mouseMoveEvent(event);
}

void DesktopLyricsWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (isLocked()) {
        event->ignore();
        return;
    }
    if (event->button() == Qt::LeftButton && m_dragMode != DragMode::None) {
        const DragMode finished = m_dragMode;
        m_dragMode = DragMode::None;
        if (finished == DragMode::Move) {
            savePositionToSettings();
        } else {
            saveWidthToSettings();
            savePositionToSettings();
        }
        updateHoverCursor(event->pos());
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void DesktopLyricsWindow::wheelEvent(QWheelEvent *event)
{
    if (isLocked()) {
        event->ignore();
        return;
    }
    const int delta = event->angleDelta().y();
    if (delta == 0) {
        QWidget::wheelEvent(event);
        return;
    }
    const int step = delta > 0 ? 2 : -2;
    adjustFontPt(step);
    event->accept();
}

void DesktopLyricsWindow::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    if (isLocked()) {
        return;
    }
    if (!m_hovered) {
        m_hovered = true;
        positionLockOverlay();
        syncLockOverlayVisibility();
        update();
    }
}

void DesktopLyricsWindow::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if (m_dragMode != DragMode::None) {
        return;
    }
    // Lock is a separate window; moving onto it would otherwise hide it immediately.
    if (pointerOverLyricsOrLock()) {
        return;
    }
    if (m_hovered) {
        m_hovered = false;
        unsetCursor();
        syncLockOverlayVisibility();
        update();
    }
}

bool DesktopLyricsWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_lockOverlay) {
        if (event->type() == QEvent::Enter) {
            if (!isLocked()) {
                m_hovered = true;
                syncLockOverlayVisibility();
                update();
            }
        } else if (event->type() == QEvent::Leave) {
            if (!isLocked() && m_dragMode == DragMode::None) {
                // Defer so we can see where the pointer went.
                QTimer::singleShot(0, this, [this]() {
                    if (isLocked() || m_dragMode != DragMode::None) {
                        return;
                    }
                    if (!pointerOverLyricsOrLock()) {
                        m_hovered = false;
                        unsetCursor();
                        syncLockOverlayVisibility();
                        update();
                    }
                });
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void DesktopLyricsWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    applyWidthFromSettings();
    applyPositionFromSettings();
    // Keep input-transparent flag consistent without re-entering showEvent loops.
    if (bool(windowFlags() & Qt::WindowTransparentForInput) != isLocked()) {
        const bool visible = isVisible();
        setWindowFlag(Qt::WindowTransparentForInput, isLocked());
        if (visible) {
            show();
        }
    }
    raise();
    positionLockOverlay();
    syncLockOverlayVisibility();
}

void DesktopLyricsWindow::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    if (m_lockOverlay) {
        m_lockOverlay->hide();
    }
}

void DesktopLyricsWindow::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    positionLockOverlay();
}

void DesktopLyricsWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    positionLockOverlay();
}

void DesktopLyricsWindow::paintEvent(QPaintEvent *)
{
    if (isLocked() || (!m_hovered && m_dragMode == DragMode::None)) {
        return;
    }
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, kHoverBackgroundAlpha));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 12, 12);
}

void DesktopLyricsWindow::onCurrentLineChanged(int)
{
    refreshText();
}

void DesktopLyricsWindow::onLyricsChanged()
{
    refreshText();
}

void DesktopLyricsWindow::onSettingsChanged(const QString &key)
{
    if (key == QLatin1String("DesktopLyricsEnabled")
        || key == QLatin1String("DisableLyricsWhenPaused")) {
        updateVisibility();
    } else if (key == QLatin1String("DesktopLyricsXPositionFactor")
               || key == QLatin1String("DesktopLyricsYPositionFactor")) {
        if (m_dragMode == DragMode::None) {
            applyPositionFromSettings();
        }
    } else if (key == QLatin1String("DesktopLyricsWidth")) {
        if (m_dragMode == DragMode::None) {
            applyWidthFromSettings();
            updateHeightToContent();
        }
    } else if (key == QLatin1String("DesktopLyricsFontPt")) {
        applyFontsFromSettings();
        updateHeightToContent();
    } else if (key == QLatin1String("DesktopLyricsLocked")) {
        applyLockedState();
    } else if (key == QLatin1String("PreferBilingualLyrics")) {
        refreshText();
    }
}

void DesktopLyricsWindow::refreshText()
{
    const auto *lyrics = m_session->lyrics();
    const int index = m_session->currentLineIndex();
    if (!lyrics || index < 0 || index >= lyrics->lines.size()) {
        m_primary->setText(QString());
        m_secondary->setText(QString());
        m_secondary->hide();
        m_primary->setProgress(0.0);
        updateHeightToContent();
        return;
    }

    const auto &line = lyrics->lines.at(index);
    m_primary->setText(line.content);

    QString secondary;
    if (m_settings->preferBilingualLyrics() && !line.translation.isEmpty()) {
        secondary = line.translation;
    } else if (index + 1 < lyrics->lines.size()) {
        secondary = lyrics->lines.at(index + 1).content;
    }

    m_secondary->setText(secondary);
    m_secondary->setVisible(!secondary.isEmpty());
    updateKaraokeProgress();
    updateHeightToContent();
}

void DesktopLyricsWindow::updateKaraokeProgress()
{
    m_primary->setProgress(lineProgress());
}

qreal DesktopLyricsWindow::lineProgress() const
{
    const auto *lyrics = m_session->lyrics();
    const int index = m_session->currentLineIndex();
    if (!lyrics || index < 0 || index >= lyrics->lines.size()) {
        return 0.0;
    }

    const double pos = m_session->effectivePositionSec()
        + (lyrics->offsetMs + m_session->extraOffsetMs()) / 1000.0;
    const double start = lyrics->lines.at(index).positionSec;
    double end = start + 4.0;
    if (index + 1 < lyrics->lines.size()) {
        end = lyrics->lines.at(index + 1).positionSec;
    }
    const double dur = end - start;
    if (dur <= 0.05) {
        return pos >= start ? 1.0 : 0.0;
    }
    return qBound(0.0, (pos - start) / dur, 1.0);
}

void DesktopLyricsWindow::updateVisibility()
{
    const bool enabled = m_settings->desktopLyricsEnabled();
    const bool hideWhenPaused = m_settings->disableLyricsWhenPaused() && !m_player->isPlaying();
    if (enabled && !hideWhenPaused) {
        show();
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
        raise();
        positionLockOverlay();
        syncLockOverlayVisibility();
    } else {
        hide();
    }
}

void DesktopLyricsWindow::applyPositionFromSettings()
{
    QScreen *screen = QGuiApplication::screenAt(pos());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }

    const QRect avail = screen->availableGeometry();
    const int maxX = qMax(0, avail.width() - width());
    const int maxY = qMax(0, avail.height() - height());
    const int x = avail.x() + static_cast<int>(m_settings->desktopPositionXFactor() * maxX);
    const int y = avail.y() + static_cast<int>(m_settings->desktopPositionYFactor() * maxY);
    move(x, y);
    positionLockOverlay();
}

void DesktopLyricsWindow::savePositionToSettings()
{
    QScreen *screen = QGuiApplication::screenAt(frameGeometry().center());
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }

    const QRect avail = screen->availableGeometry();
    const int maxX = avail.width() - width();
    const int maxY = avail.height() - height();
    const double fx = maxX > 0 ? (pos().x() - avail.x()) / static_cast<double>(maxX) : 0.5;
    const double fy = maxY > 0 ? (pos().y() - avail.y()) / static_cast<double>(maxY) : 0.5;
    m_settings->setDesktopPositionXFactor(fx);
    m_settings->setDesktopPositionYFactor(fy);
}

void DesktopLyricsWindow::applyWidthFromSettings()
{
    const int w = qBound(kMinPanelWidth, m_settings->desktopLyricsWidth(), kMaxPanelWidth);
    setMinimumWidth(kMinPanelWidth);
    setMaximumWidth(kMaxPanelWidth);
    resize(w, height());
}

void DesktopLyricsWindow::saveWidthToSettings()
{
    m_settings->setDesktopLyricsWidth(width());
}

void DesktopLyricsWindow::applyFontsFromSettings()
{
    const int primaryPt = m_settings->desktopLyricsFontPt();
    const int secondaryPt = qMax(12, static_cast<int>(qRound(primaryPt * 0.6)));
    m_primary->setLyricFont(QFont(QStringLiteral("Sans Serif"), primaryPt, QFont::Bold));
    m_secondary->setLyricFont(QFont(QStringLiteral("Sans Serif"), secondaryPt, QFont::DemiBold));
}

void DesktopLyricsWindow::applyLockedState()
{
    m_dragMode = DragMode::None;
    m_hovered = false;
    unsetCursor();
    clearMask(); // old builds used setMask which clipped painting; never again

    const bool locked = isLocked();
    if (m_lockButton) {
        const QSignalBlocker blocker(m_lockButton);
        m_lockButton->setChecked(locked);
        m_lockButton->setToolTip(locked
            ? QStringLiteral("Unlock via tray menu: Lock Desktop Lyrics")
            : QStringLiteral("Lock (click-through; unlock from tray menu)"));
    }

    const bool wasVisible = isVisible();
    // Paint still shows lyrics; input goes through the lyrics window when locked.
    setWindowFlag(Qt::WindowTransparentForInput, locked);
    if (wasVisible) {
        show();
        raise();
    }

    // Locked: no on-panel chrome (unlock is tray-only). Unlocked: hover shows controls.
    m_hovered = false;
    updateOverlayControlsForLockState();
    positionLockOverlay();
    syncLockOverlayVisibility();
    update();
}

void DesktopLyricsWindow::updateHeightToContent()
{
    if (layout()) {
        layout()->activate();
    }
    const int w = width();
    const int h = qMax(minimumHeight(), sizeHint().height());
    // Keep current width; only lock height to content.
    setMinimumWidth(kMinPanelWidth);
    setMaximumWidth(kMaxPanelWidth);
    resize(w, h);
    positionLockOverlay();
}

void DesktopLyricsWindow::adjustFontPt(int delta)
{
    if (isLocked() || delta == 0) {
        return;
    }
    m_settings->setDesktopLyricsFontPt(m_settings->desktopLyricsFontPt() + delta);
}

void DesktopLyricsWindow::updateOverlayControlsForLockState()
{
    if (!m_lockOverlay) {
        return;
    }
    const bool locked = isLocked();
    if (m_fontSmallerButton) {
        m_fontSmallerButton->setVisible(!locked);
    }
    if (m_fontLargerButton) {
        m_fontLargerButton->setVisible(!locked);
    }
    // Locked: only the lock glyph (still centered). Unlocked: A- · lock · A+
    if (locked) {
        m_lockOverlay->setFixedSize(kCtrlButtonSize, kCtrlButtonSize);
    } else {
        const int w = kCtrlButtonSize * 3 + kOverlayGap * 2;
        m_lockOverlay->setFixedSize(w, kCtrlButtonSize);
    }
    if (m_lockOverlay->layout()) {
        m_lockOverlay->layout()->activate();
    }
}

void DesktopLyricsWindow::positionLockOverlay()
{
    if (!m_lockOverlay) {
        return;
    }
    updateOverlayControlsForLockState();
    const int localX = (width() - m_lockOverlay->width()) / 2;
    const QPoint topCenter = mapToGlobal(QPoint(localX, kLockMargin));
    m_lockOverlay->move(topCenter);
    m_lockOverlay->raise();
}

void DesktopLyricsWindow::syncLockOverlayVisibility()
{
    if (!m_lockOverlay) {
        return;
    }
    // Locked: hide panel chrome entirely (unlock from tray). Unlocked: hover only.
    const bool showChrome = isVisible() && !isLocked()
        && (m_hovered || m_dragMode != DragMode::None);
    if (showChrome) {
        positionLockOverlay();
        m_lockOverlay->show();
        m_lockOverlay->raise();
    } else {
        m_lockOverlay->hide();
    }
}

bool DesktopLyricsWindow::pointerOverLyricsOrLock() const
{
    const QPoint g = QCursor::pos();
    if (frameGeometry().contains(g)) {
        return true;
    }
    return m_lockOverlay && m_lockOverlay->isVisible()
        && m_lockOverlay->frameGeometry().contains(g);
}

DesktopLyricsWindow::DragMode DesktopLyricsWindow::hitTest(const QPoint &pos) const
{
    if (isLocked()) {
        return DragMode::None;
    }
    if (pos.x() <= kEdgeResizePx) {
        return DragMode::ResizeLeft;
    }
    if (pos.x() >= width() - kEdgeResizePx) {
        return DragMode::ResizeRight;
    }
    return DragMode::None;
}

void DesktopLyricsWindow::updateHoverCursor(const QPoint &pos)
{
    if (isLocked()) {
        unsetCursor();
        return;
    }
    const DragMode edge = hitTest(pos);
    if (edge == DragMode::ResizeLeft || edge == DragMode::ResizeRight) {
        setCursor(Qt::SizeHorCursor);
    } else {
        unsetCursor();
    }
}
