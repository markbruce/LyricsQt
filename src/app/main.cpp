#include <QApplication>
#include <QDebug>
#include <QLabel>

#include <lyricsqt/AppSettings.h>
#include <lyricsqt/PlayerService.h>
#include <lyricsqt/Version.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("lyricsqt"));
    QApplication::setApplicationName(QStringLiteral("LyricsQt"));

    lyricsqt::AppSettings settings;
    lyricsqt::PlayerService player(&settings);

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

    // Emit current state once at startup for easier manual verification.
    {
        const auto track = player.currentTrack();
        qDebug().noquote()
            << QStringLiteral("[MPRIS] initial player=%1 title=%2 artist=%3 playing=%4")
                   .arg(player.activePlayerId(), track.title, track.artist)
                   .arg(player.isPlaying());
    }

    QLabel label(QStringLiteral("LyricsQt %1").arg(QLatin1String(lyricsqt::version())));
    label.resize(320, 80);
    label.show();

    return app.exec();
}
