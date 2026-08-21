#include "DesktopLyricsWindow.h"

#include "KaraokeLyricLabel.h"

#include <lyricsqt/AppSettings.h>
#include <lyricsqt/LyricsSession.h>
#include <lyricsqt/PlayerService.h>

#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QScreen>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

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
    setAttribute(Qt::WA_X11NetWmWindowTypeNotification, true);
    setWindowTitle(QStringLiteral("LyricsQt Desktop Lyrics"));
    setWindowFlag(Qt::WindowStaysOnTopHint, true);

    m_primary = new KaraokeLyricLabel(this);
    m_primary->setLyricFont(QFont(QStringLiteral("Sans Serif"), 30, QFont::Bold));

    m_secondary = new KaraokeLyricLabel(this);
    m_secondary->setLyricFont(QFont(QStringLiteral("Sans Serif"), 18, QFont::DemiBold));
    m_secondary->setUnplayedColor(QColor(0x9B, 0xC9, 0xFF));
    m_secondary->setPlayedColor(QColor(0x9B, 0xC9, 0xFF));
    m_secondary->setProgress(0.0);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 18, 28, 18);
    layout->setSpacing(8);
    layout->addWidget(m_primary);
    layout->addWidget(m_secondary);

    setMinimumWidth(360);
    setMinimumHeight(88);
    resize(720, 140);

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
    updateVisibility();
    if (m_player->isPlaying()) {
        m_progressTimer->start();
    }
}

void DesktopLyricsWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void DesktopLyricsWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragOffset);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void DesktopLyricsWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        savePositionToSettings();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void DesktopLyricsWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    applyPositionFromSettings();
    raise();
}

void DesktopLyricsWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 150));
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
        if (!m_dragging) {
            applyPositionFromSettings();
        }
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
        adjustSize();
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
    adjustSize();
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
