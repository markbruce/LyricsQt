#pragma once

#include <QString>
#include <QVector>

namespace lyricsqt {

struct WordTag {
    double timeSec = 0.0;
    int index = 0;
};

struct LyricsLine {
    double positionSec = 0.0;
    QString content;
    QString translation;
    QVector<WordTag> words;
};

struct LyricsDocument {
    QVector<LyricsLine> lines;
    int offsetMs = 0;
    QString sourceId;
    double quality = 0.0;
    QString title;
    QString artist;
    QString album;
    QString language;
    QString localPath;

    int lineIndexAt(double timeSec, int extraOffsetMs = 0) const;
};

} // namespace lyricsqt
