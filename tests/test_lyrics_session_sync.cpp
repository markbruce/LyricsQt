#include <QtTest>

#include <lyricsqt/LrcParser.h>
#include <lyricsqt/LyricsSession.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

class TestLyricsSessionSync : public QObject
{
    Q_OBJECT
private:
    QString fixturePath(const QString &name) const
    {
        const QString fromSource = QFileInfo(QStringLiteral(SOURCE_DIR "/fixtures/") + name).absoluteFilePath();
        if (QFileInfo::exists(fromSource)) {
            return fromSource;
        }
        return QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("fixtures/") + name);
    }

    lyricsqt::LyricsDocument docFromFixture() const
    {
        return lyricsqt::LrcParser::parseFile(fixturePath(QStringLiteral("basic.lrc")));
    }

private slots:
    void updates_line_when_time_advances()
    {
        lyricsqt::LyricsSession session;
        session.setLyrics(docFromFixture());
        session.setPlayback(true, /*positionSec*/ 1.0);
        QCOMPARE(session.currentLineIndex(), 0);
        session.setPlayback(true, 6.0);
        QCOMPARE(session.currentLineIndex(), 1);
    }

    void offset_affects_current_line_index()
    {
        auto doc = docFromFixture();
        doc.offsetMs = 1000;
        lyricsqt::LyricsSession session;
        session.setLyrics(doc);
        session.setPlayback(true, 0.5);
        QCOMPARE(session.currentLineIndex(), 0);
    }

    void schedules_next_line_using_offset()
    {
        lyricsqt::LyricsDocument doc;
        doc.offsetMs = 500;
        doc.lines = {
            {1.0, QStringLiteral("a")},
            {2.0, QStringLiteral("b")},
        };
        lyricsqt::LyricsSession session;
        session.setLyrics(doc);
        session.setPlayback(true, 1.0);
        QCOMPARE(session.currentLineIndex(), 0);
        // With offset: dt = 2.0 - (1.0 + 0.5) = 0.5s; without: 1.0s
        QTest::qWait(600);
        QCOMPARE(session.currentLineIndex(), 1);
    }
};

QTEST_MAIN(TestLyricsSessionSync)
#include "test_lyrics_session_sync.moc"
