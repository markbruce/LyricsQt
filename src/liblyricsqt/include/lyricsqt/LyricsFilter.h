#pragma once

#include <lyricsqt/LyricsDocument.h>

#include <QStringList>

namespace lyricsqt {

class AppSettings;

struct LyricsFilterOptions {
    bool keywordEnabled = true;
    bool smartEnabled = true;
    QStringList keywords;
};

class LyricsFilter
{
public:
    static QStringList defaultKeywords();
    static LyricsFilterOptions optionsFromSettings(const AppSettings *settings);

    /// Returns a copy of doc with matching lines removed.
    static LyricsDocument apply(const LyricsDocument &doc, const LyricsFilterOptions &options);
    static LyricsDocument apply(const LyricsDocument &doc, const AppSettings *settings);

    static bool matchesKeyword(const QString &content, const QString &key);
    static bool isEmptyOrWhitespace(const QString &content);
    static bool isPunctuationOnly(const QString &content);
    static bool isMetadataLike(const QString &content);
};

} // namespace lyricsqt
