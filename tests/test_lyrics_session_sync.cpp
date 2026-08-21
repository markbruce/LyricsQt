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
};

QTEST_MAIN(TestLyricsSessionSync)
#include "test_lyrics_session_sync.moc"
