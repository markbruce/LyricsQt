#include <lyricsqt/LyricsSession.h>

#include <QDateTime>

namespace lyricsqt {

LyricsSession::LyricsSession(QObject *parent)
    : QObject(parent)
{
    m_lineCheckTimer.setSingleShot(true);
    connect(&m_lineCheckTimer, &QTimer::timeout, this, [this]() {
        recomputeCurrentLine();
        scheduleNextLineCheck();
    });
}

void LyricsSession::setLyrics(const LyricsDocument &doc)
{
    m_lyrics = doc;
    emit lyricsChanged();
    recomputeCurrentLine();
    scheduleNextLineCheck();
}

void LyricsSession::clearLyrics()
{
    m_lineCheckTimer.stop();
    m_lyrics.reset();
    if (m_currentLine != -1) {
        m_currentLine = -1;
        emit currentLineChanged(m_currentLine);
    }
    emit lyricsChanged();
}

bool LyricsSession::hasLyrics() const
{
    return m_lyrics.has_value();
}

const LyricsDocument *LyricsSession::lyrics() const
{
    return m_lyrics ? &*m_lyrics : nullptr;
}

void LyricsSession::setPlayback(bool playing, double positionSec)
{
    m_playing = playing;
    m_positionSec = positionSec;
    m_positionStampMs = QDateTime::currentMSecsSinceEpoch();
    recomputeCurrentLine();
    scheduleNextLineCheck();
}

int LyricsSession::currentLineIndex() const
{
    return m_currentLine;
}

void LyricsSession::setExtraOffsetMs(int offsetMs)
{
    if (m_extraOffsetMs == offsetMs) {
        return;
    }
    m_extraOffsetMs = offsetMs;
    recomputeCurrentLine();
    scheduleNextLineCheck();
}

int LyricsSession::extraOffsetMs() const
{
    return m_extraOffsetMs;
}

double LyricsSession::effectivePositionSec() const
{
    if (!m_playing) {
        return m_positionSec;
    }
    const qint64 elapsedMs = QDateTime::currentMSecsSinceEpoch() - m_positionStampMs;
    return m_positionSec + elapsedMs / 1000.0;
}

void LyricsSession::recomputeCurrentLine()
{
    const int index = m_lyrics ? m_lyrics->lineIndexAt(effectivePositionSec(), m_extraOffsetMs) : -1;
    if (index != m_currentLine) {
        m_currentLine = index;
        emit currentLineChanged(m_currentLine);
    }
}

void LyricsSession::scheduleNextLineCheck()
{
    m_lineCheckTimer.stop();
    if (!m_playing || !m_lyrics || m_lyrics->lines.isEmpty()) {
        return;
    }

    const double pos = effectivePositionSec();
    const int index = m_lyrics->lineIndexAt(pos, m_extraOffsetMs);
    const int next = index + 1;
    if (next < 0 || next >= m_lyrics->lines.size()) {
        return;
    }

    const double adjustedPos = pos + (m_lyrics->offsetMs + m_extraOffsetMs) / 1000.0;
    const double dt = m_lyrics->lines[next].positionSec - adjustedPos;
    if (dt <= 0.0) {
        return;
    }

    m_lineCheckTimer.start(static_cast<int>(dt * 1000.0));
}

} // namespace lyricsqt
