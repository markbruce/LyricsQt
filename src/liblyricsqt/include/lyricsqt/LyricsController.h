#pragma once

#include <QObject>

namespace lyricsqt {

class LyricsSession;
class LyricsStore;
class PlayerService;
struct TrackInfo;

class LyricsController : public QObject
{
    Q_OBJECT
public:
    LyricsController(PlayerService *player,
                     LyricsSession *session,
                     LyricsStore *store,
                     QObject *parent = nullptr);

private:
    void onTrackChanged(const TrackInfo &track);
    void syncPlayback();

    PlayerService *m_player = nullptr;
    LyricsSession *m_session = nullptr;
    LyricsStore *m_store = nullptr;
};

} // namespace lyricsqt
