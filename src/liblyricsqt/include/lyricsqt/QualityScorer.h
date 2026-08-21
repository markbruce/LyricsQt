#pragma once

#include <lyricsqt/LyricsDocument.h>
#include <lyricsqt/TrackInfo.h>

namespace lyricsqt {

class QualityScorer
{
public:
    /// Heuristic quality score for matching a candidate lyrics document to a track.
    /// Higher is better. Factors: title/artist match, duration proximity,
    /// word-level tags, and translation presence.
    static double score(const TrackInfo &track, const LyricsDocument &doc);
};

} // namespace lyricsqt
