#pragma once

#include <lyricsqt/LyricsDocument.h>

#include <QString>

namespace lyricsqt {

class LrcParser
{
public:
    static LyricsDocument parse(const QString &text);
    static LyricsDocument parseFile(const QString &path);
};

} // namespace lyricsqt
