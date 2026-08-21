#pragma once

#include <lyricsqt/TrackInfo.h>

#include <QObject>

namespace lyricsqt {

class LyricsSession;
class LyricsStore;
class PlayerService;
class ProviderHub;
struct LyricsDocument;

class LyricsController : public QObject
{
    Q_OBJECT
public:
    LyricsController(PlayerService *player,
                     LyricsSession *session,
                     LyricsStore *store,
                     ProviderHub *providers = nullptr,
                     QObject *parent = nullptr);

private:
    void onTrackChanged(const TrackInfo &track);
    void onRemoteLyrics(const TrackInfo &track, const LyricsDocument &doc);
    void syncPlayback();

    PlayerService *m_player = nullptr;
    LyricsSession *m_session = nullptr;
    LyricsStore *m_store = nullptr;
    ProviderHub *m_providers = nullptr;
    TrackInfo m_currentTrack;
};

} // namespace lyricsqt
