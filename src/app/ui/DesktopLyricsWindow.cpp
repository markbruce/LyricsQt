#include "DesktopLyricsWindow.h"

#include <lyricsqt/AppSettings.h>
#include <lyricsqt/LyricsSession.h>
#include <lyricsqt/PlayerService.h>

#include <QGuiApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QScreen>
#include <QShowEvent>
#include <QVBoxLayout>

DesktopLyricsWindow::DesktopLyricsWindow(lyricsqt::AppSettings *settings,
                                         lyricsqt::LyricsSession *session,
                                         lyricsqt::PlayerService *player,
                                         QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint)
    , m_settings(settings)
    , m_session(session)
    , m_player(player)
{
    Q_ASSERT(m_settings);
    Q_ASSERT(m_session);
    Q_ASSERT(m_player);

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setWindowTitle(QStringLiteral("LyricsQt Desktop Lyrics"));

    m_primary = new QLabel(this);
    m_primary->setAlignment(Qt::AlignCenter);
    m_primary->setWordWrap(true);
    m_primary->setStyleSheet(QStringLiteral(
        "QLabel { color: white; font-size: 28px; font-weight: 600;"
        " background: transparent; }"));

    m_secondary = new QLabel(this);
    m_secondary->setAlignment(Qt::AlignCenter);
    m_secondary->setWordWrap(true);
    m_secondary->setStyleSheet(QStringLiteral(
        "QLabel { color: rgba(255,255,255,180); font-size: 18px;"
        " background: transparent; }"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 16, 24, 16);
    layout->setSpacing(6);
    layout->addWidget(m_primary);
    layout->addWidget(m_secondary);

    setMinimumWidth(320);
    setMinimumHeight(72);
    resize(640, 120);

    connect(m_session, &lyricsqt::LyricsSession::currentLineChanged,
            this, &DesktopLyricsWindow::onCurrentLineChanged);
    connect(m_session, &lyricsqt::LyricsSession::lyricsChanged,
            this, &DesktopLyricsWindow::onLyricsChanged);
    connect(m_settings, &lyricsqt::AppSettings::changed,
            this, &DesktopLyricsWindow::onSettingsChanged);
    connect(m_player, &lyricsqt::PlayerService::playbackChanged,
            this, [this](bool) { updateVisibility(); });

    refreshText();
    applyPositionFromSettings();
    updateVisibility();
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
    applyPositionFromSettings();
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
        return;
    }

    const auto &line = lyrics->lines.at(index);
    m_primary->setText(line.content);

    QString secondary;
    if (!line.translation.isEmpty()) {
        secondary = line.translation;
    } else if (index + 1 < lyrics->lines.size()) {
        secondary = lyrics->lines.at(index + 1).content;
    }

    m_secondary->setText(secondary);
    m_secondary->setVisible(!secondary.isEmpty());
    adjustSize();
}

void DesktopLyricsWindow::updateVisibility()
{
    const bool enabled = m_settings->desktopLyricsEnabled();
    const bool hideWhenPaused = m_settings->disableLyricsWhenPaused() && !m_player->isPlaying();
    if (enabled && !hideWhenPaused) {
        show();
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
