#include <lyricsqt/LyricsController.h>

#include <lyricsqt/LyricsSession.h>
#include <lyricsqt/LyricsStore.h>
#include <lyricsqt/PlayerService.h>
#include <lyricsqt/TrackInfo.h>

#include <QDebug>

namespace lyricsqt {

LyricsController::LyricsController(PlayerService *player,
                                   LyricsSession *session,
                                   LyricsStore *store,
                                   QObject *parent)
    : QObject(parent)
    , m_player(player)
    , m_session(session)
    , m_store(store)
{
    Q_ASSERT(m_player);
    Q_ASSERT(m_session);
    Q_ASSERT(m_store);

    connect(m_player, &PlayerService::trackChanged, this, &LyricsController::onTrackChanged);
    connect(m_player, &PlayerService::playbackChanged, this, [this](bool) {
        syncPlayback();
    });
    connect(m_player, &PlayerService::positionChanged, this, [this](double) {
        syncPlayback();
    });

    // Apply current track once at construction (startup / late wire-up).
    onTrackChanged(m_player->currentTrack());
}

void LyricsController::onTrackChanged(const TrackInfo &track)
{
    m_session->clearLyrics();

    if (track.isEmpty()) {
        qDebug().noquote() << QStringLiteral("[LyricsController] track cleared");
        syncPlayback();
        return;
    }

    qDebug().noquote()
        << QStringLiteral("[LyricsController] trackChanged title=%1 artist=%2")
               .arg(track.title, track.artist);

    if (auto doc = m_store->loadLocal(track)) {
        m_session->setLyrics(*doc);
        qDebug().noquote()
            << QStringLiteral("[LyricsController] applied local lyrics lines=%1 path=%2")
                   .arg(doc->lines.size())
                   .arg(doc->localPath);
    } else {
        qDebug().noquote() << QStringLiteral("[LyricsController] no local lyrics yet");
    }

    syncPlayback();
}

void LyricsController::syncPlayback()
{
    m_session->setPlayback(m_player->isPlaying(), m_player->positionSec());
}

} // namespace lyricsqt
