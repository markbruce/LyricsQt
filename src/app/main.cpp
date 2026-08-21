#include <QApplication>
#include <QDebug>
#include <QIcon>

#include <lyricsqt/AppSettings.h>
#include <lyricsqt/LyricsController.h>
#include <lyricsqt/LyricsSession.h>
#include <lyricsqt/LyricsStore.h>
#include <lyricsqt/KugouProvider.h>
#include <lyricsqt/LrclibProvider.h>
#include <lyricsqt/NetEaseProvider.h>
#include <lyricsqt/PlayerService.h>
#include <lyricsqt/ProviderHub.h>
#include <lyricsqt/QQMusicProvider.h>
#include <lyricsqt/Version.h>

#include "ui/DesktopLyricsWindow.h"
#include "ui/LyricsHudWindow.h"
#include "ui/SearchLyricsDialog.h"
#include "ui/TrayController.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("lyricsqt"));
    QApplication::setApplicationName(QStringLiteral("LyricsQt"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/lyricsqt.png")));
    QApplication::setQuitOnLastWindowClosed(false);

    lyricsqt::AppSettings settings;
    lyricsqt::PlayerService player(&settings);
    lyricsqt::LyricsSession session;
    lyricsqt::LyricsStore store(&settings);
    lyricsqt::ProviderHub providers;
    providers.addProvider(new lyricsqt::LrclibProvider);
    providers.addProvider(new lyricsqt::NetEaseProvider);
    providers.addProvider(new lyricsqt::QQMusicProvider);
    providers.addProvider(new lyricsqt::KugouProvider);
    providers.setEnabledProviderIds(settings.enabledProviderIds());
    lyricsqt::LyricsController controller(&player, &session, &store, &providers, &settings);

    session.setExtraOffsetMs(settings.globalOffsetMs());
    QObject::connect(&settings, &lyricsqt::AppSettings::changed,
                     &session, [&settings, &session, &providers](const QString &key) {
                         if (key == QLatin1String("GlobalLyricsOffset")) {
                             session.setExtraOffsetMs(settings.globalOffsetMs());
                         } else if (key == QLatin1String("EnabledProviderIds")) {
                             providers.setEnabledProviderIds(settings.enabledProviderIds());
                         }
                     });

    QObject::connect(&player, &lyricsqt::PlayerService::trackChanged,
                     &app, [&player](const lyricsqt::TrackInfo &track) {
                         qDebug().noquote()
                             << QStringLiteral("[MPRIS] track title=%1 artist=%2 playing=%3 player=%4")
                                    .arg(track.title, track.artist)
                                    .arg(player.isPlaying())
                                    .arg(player.activePlayerId());
                     });
    QObject::connect(&player, &lyricsqt::PlayerService::playbackChanged,
                     &app, [&player](bool playing) {
                         const auto track = player.currentTrack();
                         qDebug().noquote()
                             << QStringLiteral("[MPRIS] playback playing=%1 title=%2 artist=%3")
                                    .arg(playing)
                                    .arg(track.title, track.artist);
                     });
    QObject::connect(&session, &lyricsqt::LyricsSession::currentLineChanged,
                     &app, [&session](int index) {
                         const auto *lyrics = session.lyrics();
                         if (!lyrics || index < 0 || index >= lyrics->lines.size()) {
                             qDebug().noquote() << QStringLiteral("[Lyrics] line=-1");
                             return;
                         }
                         qDebug().noquote()
                             << QStringLiteral("[Lyrics] line=%1 text=%2")
                                    .arg(index)
                                    .arg(lyrics->lines.at(index).content);
                     });

    {
        const auto track = player.currentTrack();
        qDebug().noquote()
            << QStringLiteral("[MPRIS] initial player=%1 title=%2 artist=%3 playing=%4")
                   .arg(player.activePlayerId(), track.title, track.artist)
                   .arg(player.isPlaying());
        qDebug().noquote()
            << QStringLiteral("[LyricsStore] savingPath=%1 besideTrack=%2")
                   .arg(settings.lyricsSavingPath())
                   .arg(settings.loadLyricsBesideTrack());
        qDebug().noquote()
            << QStringLiteral("[App] LyricsQt %1 HUD + desktop overlay + tray ready")
                   .arg(QLatin1String(lyricsqt::version()));
    }

    DesktopLyricsWindow desktopWindow(&settings, &session, &player);
    LyricsHudWindow hudWindow(&session, &player, &store, &providers);
    TrayController tray(&settings, &session);
    SearchLyricsDialog searchDialog(&player, &session, &store, &providers, &settings);

    const auto openSearchDialog = [&searchDialog]() {
        searchDialog.reloadFromCurrentTrack();
        searchDialog.show();
        searchDialog.raise();
        searchDialog.activateWindow();
    };

    const auto markWrongLyrics = [&]() {
        const lyricsqt::TrackInfo track = player.currentTrack();
        if (track.isEmpty()) {
            return;
        }
        if (!track.id.isEmpty()) {
            settings.addNoSearchingTrackId(track.id);
        }
        providers.cancel();
        session.clearLyrics();
        qDebug().noquote()
            << QStringLiteral("[App] wrong lyrics; ignored id=%1 title=%2")
                   .arg(track.id, track.title);
    };

    QObject::connect(&tray, &TrayController::showHudRequested, &hudWindow, [&hudWindow]() {
        hudWindow.show();
        hudWindow.raise();
        hudWindow.activateWindow();
    });
    QObject::connect(&tray, &TrayController::preferencesRequested, &app, []() {
        qDebug().noquote() << QStringLiteral("[Tray] Preferences (stub)");
    });
    QObject::connect(&tray, &TrayController::searchLyricsRequested, &app, openSearchDialog);
    QObject::connect(&tray, &TrayController::wrongLyricsRequested, &app, markWrongLyrics);
    QObject::connect(&hudWindow, &LyricsHudWindow::searchLyricsRequested, &app, openSearchDialog);
    QObject::connect(&hudWindow, &LyricsHudWindow::wrongLyricsRequested, &app, markWrongLyrics);

    Q_UNUSED(controller);
    Q_UNUSED(desktopWindow);
    return app.exec();
}
