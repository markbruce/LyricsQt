#include <QtTest>

#include <lyricsqt/LyricsDocument.h>
#include <lyricsqt/QualityScorer.h>
#include <lyricsqt/TrackInfo.h>

using lyricsqt::LyricsDocument;
using lyricsqt::LyricsLine;
using lyricsqt::QualityScorer;
using lyricsqt::TrackInfo;
using lyricsqt::WordTag;

class TestQualityScorer : public QObject
{
    Q_OBJECT

private:
    TrackInfo baseTrack() const
    {
        TrackInfo track;
        track.title = QStringLiteral("Hello World");
        track.artist = QStringLiteral("Test Artist");
        track.lengthUs = 180'000'000; // 180s
        return track;
    }

    LyricsDocument baseDoc() const
    {
        LyricsDocument doc;
        doc.title = QStringLiteral("Hello World");
        doc.artist = QStringLiteral("Test Artist");
        LyricsLine line;
        line.positionSec = 10.0;
        line.content = QStringLiteral("line");
        doc.lines.append(line);
        LyricsLine end;
        end.positionSec = 175.0;
        end.content = QStringLiteral("end");
        doc.lines.append(end);
        return doc;
    }

private slots:
    void perfect_match_scores_highest()
    {
        const double score = QualityScorer::score(baseTrack(), baseDoc());
        QVERIFY(score > 80.0);
    }

    void title_mismatch_lowers_score()
    {
        auto doc = baseDoc();
        doc.title = QStringLiteral("Completely Different");
        const double good = QualityScorer::score(baseTrack(), baseDoc());
        const double bad = QualityScorer::score(baseTrack(), doc);
        QVERIFY(good > bad);
    }

    void artist_mismatch_lowers_score()
    {
        auto doc = baseDoc();
        doc.artist = QStringLiteral("Other Band");
        const double good = QualityScorer::score(baseTrack(), baseDoc());
        const double bad = QualityScorer::score(baseTrack(), doc);
        QVERIFY(good > bad);
    }

    void duration_proximity_matters()
    {
        auto close = baseDoc();
        close.lines.last().positionSec = 178.0;

        auto far = baseDoc();
        far.lines.last().positionSec = 60.0;

        const double closeScore = QualityScorer::score(baseTrack(), close);
        const double farScore = QualityScorer::score(baseTrack(), far);
        QVERIFY(closeScore > farScore);
    }

    void word_tags_bonus()
    {
        auto plain = baseDoc();
        auto withWords = baseDoc();
        WordTag tag;
        tag.timeSec = 10.0;
        tag.index = 0;
        withWords.lines[0].words.append(tag);

        const double plainScore = QualityScorer::score(baseTrack(), plain);
        const double wordScore = QualityScorer::score(baseTrack(), withWords);
        QVERIFY(wordScore > plainScore);
    }

    void translation_bonus()
    {
        auto plain = baseDoc();
        auto bilingual = baseDoc();
        bilingual.lines[0].translation = QStringLiteral("翻译");

        const double plainScore = QualityScorer::score(baseTrack(), plain);
        const double biScore = QualityScorer::score(baseTrack(), bilingual);
        QVERIFY(biScore > plainScore);
    }

    void case_insensitive_title_artist_match()
    {
        auto doc = baseDoc();
        doc.title = QStringLiteral("hello world");
        doc.artist = QStringLiteral("TEST ARTIST");
        const double score = QualityScorer::score(baseTrack(), doc);
        const double exact = QualityScorer::score(baseTrack(), baseDoc());
        QCOMPARE(score, exact);
    }
};

QTEST_MAIN(TestQualityScorer)
#include "test_quality_scorer.moc"
