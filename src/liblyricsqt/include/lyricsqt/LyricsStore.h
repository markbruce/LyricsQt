#pragma once

#include <lyricsqt/LyricsDocument.h>
#include <lyricsqt/TrackInfo.h>

#include <QString>
#include <QUrl>

#include <optional>

namespace lyricsqt {

class AppSettings;

class LyricsStore
{
public:
    explicit LyricsStore(AppSettings *settings);

    std::optional<LyricsDocument> loadLocal(const TrackInfo &track);
    QUrl save(const TrackInfo &track, const LyricsDocument &doc);
    /// Deletes cached .lrcx/.lrc under lyricsSavingPath for this title/artist.
    /// Does not touch beside-track files. Returns true if at least one file was removed.
    bool removeLocal(const TrackInfo &track);
    QString cacheFileName(const TrackInfo &track) const;

private:
    std::optional<LyricsDocument> tryLoadFile(const QString &path) const;
    QString serializeLrc(const LyricsDocument &doc) const;

    AppSettings *m_settings = nullptr;
};

} // namespace lyricsqt
