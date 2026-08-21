#include <lyricsqt/LyricsController.h>

#include <lyricsqt/AppSettings.h>
#include <lyricsqt/LyricsDocument.h>
#include <lyricsqt/LyricsFilter.h>
#include <lyricsqt/LyricsSession.h>
#include <lyricsqt/LyricsStore.h>
#include <lyricsqt/PlayerService.h>
#include <lyricsqt/ProviderHub.h>
#include <lyricsqt/TrackInfo.h>

#include <QDebug>

namespace lyricsqt {

LyricsController::LyricsController(PlayerService *player,
                                   LyricsSession *session,
                                   LyricsStore *store,
                                   ProviderHub *providers,
                                   AppSettings *settings,
                                   QObject *parent)
    : QObject(parent)
    , m_player(player)
    , m_session(session)
    , m_store(store)
    , m_providers(providers)
    , m_settings(settings)
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

    if (m_providers) {
        connect(m_providers, &ProviderHub::lyricsFound, this, &LyricsController::onRemoteLyrics);
    }

    // Apply current track once at construction (startup / late wire-up).
    onTrackChanged(m_player->currentTrack());
}

void LyricsController::onTrackChanged(const TrackInfo &track)
{
    m_currentTrack = track;
    m_session->clearLyrics();

    if (m_providers) {
        m_providers->cancel();
    }

    if (track.isEmpty()) {
        qDebug().noquote() << QStringLiteral("[LyricsController] track cleared");
        syncPlayback();
        return;
    }

    qDebug().noquote()
        << QStringLiteral("[LyricsController] trackChanged title=%1 artist=%2")
               .arg(track.title, track.artist);

    if (auto doc = m_store->loadLocal(track)) {
        const LyricsDocument filtered = LyricsFilter::apply(*doc, m_settings);
        m_session->setLyrics(filtered);
        qDebug().noquote()
            << QStringLiteral("[LyricsController] applied local lyrics lines=%1 path=%2")
                   .arg(filtered.lines.size())
                   .arg(filtered.localPath);
    } else {
        qDebug().noquote() << QStringLiteral("[LyricsController] no local lyrics yet");
        const QString ignoreKey = !track.id.isEmpty()
            ? track.id
            : (track.title + QLatin1Char('|') + track.artist);
        const bool ignored = m_settings && m_settings->isNoSearchingTrackId(ignoreKey);
        if (ignored) {
            qDebug().noquote()
                << QStringLiteral("[LyricsController] skip provider search; track in no-search list");
        } else if (m_providers) {
            qDebug().noquote() << QStringLiteral("[LyricsController] starting provider search");
            m_providers->search(track);
        }
    }

    syncPlayback();
}

void LyricsController::onRemoteLyrics(const TrackInfo &track, const LyricsDocument &doc)
{
    // Ignore stale results if the user already skipped to another track.
    if (track != m_currentTrack) {
        return;
    }

    // Local / HUD import wins: do not overwrite existing session lyrics with remote.
    if (m_session->hasLyrics()) {
        qDebug().noquote()
            << QStringLiteral("[LyricsController] skip remote lyrics; session already has lyrics");
        return;
    }

    LyricsDocument applied = LyricsFilter::apply(doc, m_settings);
    const QUrl saved = m_store->save(track, applied);
    if (saved.isLocalFile()) {
        applied.localPath = saved.toLocalFile();
    }

    m_session->setLyrics(applied);
    qDebug().noquote()
        << QStringLiteral("[LyricsController] applied remote lyrics source=%1 quality=%2 lines=%3")
               .arg(applied.sourceId)
               .arg(applied.quality)
               .arg(applied.lines.size());
    syncPlayback();
}

void LyricsController::syncPlayback()
{
    m_session->setPlayback(m_player->isPlaying(), m_player->positionSec());
}

} // namespace lyricsqt
