#include <lyricsqt/Version.h>

namespace lyricsqt {

// Ensures the shared/static library has a translation unit.
const char* versionString()
{
    return version();
}

} // namespace lyricsqt
