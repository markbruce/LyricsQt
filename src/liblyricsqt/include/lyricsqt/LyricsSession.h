#pragma once

#include <lyricsqt/LyricsDocument.h>

#include <QObject>
#include <QTimer>
#include <optional>

namespace lyricsqt {

class LyricsSession : public QObject
{
    Q_OBJECT
public:
    explicit LyricsSession(QObject *parent = nullptr);

    void setLyrics(const LyricsDocument &doc);
    void clearLyrics();
    bool hasLyrics() const;
    const LyricsDocument *lyrics() const;

    void setPlayback(bool playing, double positionSec);
    int currentLineIndex() const;
    // Wall-clock-adjusted playhead used for line index + karaoke progress.
    // Does not extrapolate until MPRIS Position has been observed to advance
    // (Chromium/Edge often report Position=0 forever).
    double effectivePositionSec() const;

    void setExtraOffsetMs(int offsetMs);
    int extraOffsetMs() const;

signals:
    void lyricsChanged();
    void currentLineChanged(int index);

private:
    void recomputeCurrentLine();
    void scheduleNextLineCheck();

    std::optional<LyricsDocument> m_lyrics;
    int m_currentLine = -1;
    bool m_playing = false;
    double m_positionSec = 0.0;
    qint64 m_positionStampMs = 0;
    int m_extraOffsetMs = 0;
    bool m_positionAdvances = false;
    QTimer m_lineCheckTimer;
};

} // namespace lyricsqt
