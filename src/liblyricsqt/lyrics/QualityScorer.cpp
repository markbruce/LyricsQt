#include <lyricsqt/QualityScorer.h>

#include <QtMath>

namespace lyricsqt {

namespace {

QString normalize(QString value)
{
    return value.trimmed().toLower();
}

bool hasWordTags(const LyricsDocument &doc)
{
    for (const LyricsLine &line : doc.lines) {
        if (!line.words.isEmpty()) {
            return true;
        }
    }
    return false;
}

bool hasTranslation(const LyricsDocument &doc)
{
    for (const LyricsLine &line : doc.lines) {
        if (!line.translation.isEmpty()) {
            return true;
        }
    }
    return false;
}

double estimatedDurationSec(const LyricsDocument &doc)
{
    if (doc.lines.isEmpty()) {
        return 0.0;
    }
    return doc.lines.last().positionSec;
}

double titleArtistMatchScore(const QString &trackValue, const QString &docValue, double exact, double partial)
{
    const QString a = normalize(trackValue);
    const QString b = normalize(docValue);
    if (a.isEmpty() || b.isEmpty()) {
        return 0.0;
    }
    if (a == b) {
        return exact;
    }
    if (a.contains(b) || b.contains(a)) {
        return partial;
    }
    return 0.0;
}

double durationProximityScore(const TrackInfo &track, const LyricsDocument &doc)
{
    if (track.lengthUs <= 0) {
        return 0.0;
    }
    const double trackSec = static_cast<double>(track.lengthUs) / 1'000'000.0;
    const double docSec = estimatedDurationSec(doc);
    if (docSec <= 0.0) {
        return 0.0;
    }
    const double delta = qAbs(trackSec - docSec);
    constexpr double fullMatchWindow = 2.0;
    constexpr double zeroAt = 30.0;
    constexpr double maxPoints = 20.0;
    if (delta <= fullMatchWindow) {
        return maxPoints;
    }
    if (delta >= zeroAt) {
        return 0.0;
    }
    return maxPoints * (1.0 - (delta - fullMatchWindow) / (zeroAt - fullMatchWindow));
}

} // namespace

double QualityScorer::score(const TrackInfo &track, const LyricsDocument &doc)
{
    double total = 0.0;
    total += titleArtistMatchScore(track.title, doc.title, 40.0, 20.0);
    total += titleArtistMatchScore(track.artist, doc.artist, 30.0, 15.0);
    total += durationProximityScore(track, doc);
    if (hasWordTags(doc)) {
        total += 10.0;
    }
    if (hasTranslation(doc)) {
        total += 5.0;
    }
    return total;
}

} // namespace lyricsqt
