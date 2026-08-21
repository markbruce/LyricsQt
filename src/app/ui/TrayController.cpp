#include "TrayController.h"

#include <lyricsqt/AppSettings.h>
#include <lyricsqt/LyricsSession.h>

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>

TrayController::TrayController(lyricsqt::AppSettings *settings,
                               lyricsqt::LyricsSession *session,
                               QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_session(session)
{
    Q_ASSERT(m_settings);
    Q_ASSERT(m_session);

    m_tray = new QSystemTrayIcon(this);
    const QIcon icon(QStringLiteral(":/icons/lyricsqt.png"));
    m_tray->setIcon(icon.isNull() ? QIcon(QStringLiteral(":/icons/lyricsqt.svg")) : icon);
    m_tray->setToolTip(QStringLiteral("LyricsQt"));

    m_menu = new QMenu;
    auto *showHud = m_menu->addAction(QStringLiteral("Show HUD"));
    connect(showHud, &QAction::triggered, this, &TrayController::showHudRequested);

    m_toggleDesktopAction = m_menu->addAction(QStringLiteral("Toggle Desktop Lyrics"));
    m_toggleDesktopAction->setCheckable(true);
    m_toggleDesktopAction->setChecked(m_settings->desktopLyricsEnabled());
    connect(m_toggleDesktopAction, &QAction::triggered, this, &TrayController::toggleDesktopLyrics);

    m_menu->addSeparator();
    auto *offsetPlus = m_menu->addAction(QStringLiteral("Offset +100 ms"));
    connect(offsetPlus, &QAction::triggered, this, [this]() { adjustOffset(100); });
    auto *offsetMinus = m_menu->addAction(QStringLiteral("Offset -100 ms"));
    connect(offsetMinus, &QAction::triggered, this, [this]() { adjustOffset(-100); });

    m_menu->addSeparator();
    auto *prefs = m_menu->addAction(QStringLiteral("Preferences…"));
    connect(prefs, &QAction::triggered, this, &TrayController::preferencesRequested);

    auto *quit = m_menu->addAction(QStringLiteral("Quit"));
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);

    m_tray->setContextMenu(m_menu);
    m_tray->show();

    connect(m_session, &lyricsqt::LyricsSession::currentLineChanged,
            this, &TrayController::onCurrentLineChanged);
    connect(m_session, &lyricsqt::LyricsSession::lyricsChanged,
            this, &TrayController::onLyricsChanged);
    connect(m_settings, &lyricsqt::AppSettings::changed,
            this, &TrayController::onSettingsChanged);

    updateTooltip();
}

TrayController::~TrayController()
{
    delete m_menu;
    m_menu = nullptr;
}

void TrayController::onCurrentLineChanged(int)
{
    updateTooltip();
}

void TrayController::onLyricsChanged()
{
    updateTooltip();
}

void TrayController::onSettingsChanged(const QString &key)
{
    if (key == QLatin1String("DesktopLyricsEnabled") && m_toggleDesktopAction) {
        m_toggleDesktopAction->setChecked(m_settings->desktopLyricsEnabled());
    } else if (key == QLatin1String("MenuBarLyricsEnabled")) {
        updateTooltip();
    }
}

void TrayController::updateTooltip()
{
    if (!m_settings->menuBarLyricsEnabled()) {
        m_tray->setToolTip(QStringLiteral("LyricsQt"));
        return;
    }

    const auto *lyrics = m_session->lyrics();
    const int index = m_session->currentLineIndex();
    if (!lyrics || index < 0 || index >= lyrics->lines.size()) {
        m_tray->setToolTip(QStringLiteral("LyricsQt"));
        return;
    }

    m_tray->setToolTip(lyrics->lines.at(index).content);
}

void TrayController::toggleDesktopLyrics()
{
    m_settings->setDesktopLyricsEnabled(m_toggleDesktopAction->isChecked());
}

void TrayController::adjustOffset(int deltaMs)
{
    const int next = m_settings->globalOffsetMs() + deltaMs;
    m_settings->setGlobalOffsetMs(next);
    m_session->setExtraOffsetMs(next);
}
