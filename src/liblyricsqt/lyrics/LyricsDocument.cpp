#include <lyricsqt/LyricsDocument.h>

#include <algorithm>

namespace lyricsqt {

int LyricsDocument::lineIndexAt(double timeSec, int extraOffsetMs) const
{
    if (lines.isEmpty()) {
        return -1;
    }

    const double t = timeSec + (offsetMs + extraOffsetMs) / 1000.0;
    int best = -1;
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].positionSec <= t + 1e-9) {
            best = i;
        } else {
            break;
        }
    }
    return best;
}

} // namespace lyricsqt
