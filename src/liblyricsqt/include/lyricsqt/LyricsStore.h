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
    QString cacheFileName(const TrackInfo &track) const;

private:
    std::optional<LyricsDocument> tryLoadFile(const QString &path) const;
    QString serializeLrc(const LyricsDocument &doc) const;

    AppSettings *m_settings = nullptr;
};

} // namespace lyricsqt
