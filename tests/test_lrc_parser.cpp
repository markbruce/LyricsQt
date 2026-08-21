#include <QtTest>

#include <lyricsqt/LrcParser.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

class TestLrcParser : public QObject
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

private slots:
    void parses_three_lines()
    {
        const auto doc = lyricsqt::LrcParser::parseFile(fixturePath(QStringLiteral("basic.lrc")));
        QCOMPARE(doc.lines.size(), 3);
        QCOMPARE(doc.title, QStringLiteral("Test Song"));
        QCOMPARE(doc.artist, QStringLiteral("Test Artist"));
        QCOMPARE(doc.lines[0].content, QStringLiteral("First line"));
        QCOMPARE(doc.lines[0].positionSec, 1.0);
        QCOMPARE(doc.lines[1].positionSec, 5.5);
        QCOMPARE(doc.lines[2].positionSec, 10.0);
    }

    void lineIndex_at_time()
    {
        const auto doc = lyricsqt::LrcParser::parseFile(fixturePath(QStringLiteral("basic.lrc")));
        QCOMPARE(doc.lineIndexAt(0.5, 0), -1);
        QCOMPARE(doc.lineIndexAt(1.0, 0), 0);
        QCOMPARE(doc.lineIndexAt(6.0, 0), 1);
        QCOMPARE(doc.lineIndexAt(10.0, 0), 2);
    }

    void multiple_timestamps_on_one_line()
    {
        const auto doc = lyricsqt::LrcParser::parse(QStringLiteral("[00:01.00][00:02.00]Hello"));
        QCOMPARE(doc.lines.size(), 2);
        QCOMPARE(doc.lines[0].content, QStringLiteral("Hello"));
        QCOMPARE(doc.lines[1].content, QStringLiteral("Hello"));
        QCOMPARE(doc.lines[0].positionSec, 1.0);
        QCOMPARE(doc.lines[1].positionSec, 2.0);
    }
};

QTEST_MAIN(TestLrcParser)
#include "test_lrc_parser.moc"
