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

signals:
    void lyricsChanged();
    void currentLineChanged(int index);

private:
    void recomputeCurrentLine();
    void scheduleNextLineCheck();
    double effectivePositionSec() const;

    std::optional<LyricsDocument> m_lyrics;
    int m_currentLine = -1;
    bool m_playing = false;
    double m_positionSec = 0.0;
    qint64 m_positionStampMs = 0;
    QTimer m_lineCheckTimer;
};

} // namespace lyricsqt
