#include <QtTest>

#include <lyricsqt/LyricsDocument.h>
#include <lyricsqt/LyricsFilter.h>

using lyricsqt::LyricsDocument;
using lyricsqt::LyricsFilter;
using lyricsqt::LyricsFilterOptions;
using lyricsqt::LyricsLine;

class TestLyricsFilter : public QObject
{
    Q_OBJECT

private:
    static LyricsLine lineAt(double sec, const QString &content, const QString &translation = {})
    {
        LyricsLine line;
        line.positionSec = sec;
        line.content = content;
        line.translation = translation;
        return line;
    }

private slots:
    void keyword_filter_drops_matching_substring()
    {
        LyricsDocument doc;
        doc.lines = {
            lineAt(0.0, QStringLiteral("hello world")),
            lineAt(1.0, QStringLiteral("作词：某人")),
            lineAt(2.0, QStringLiteral("keep me")),
        };

        LyricsFilterOptions opts;
        opts.keywordEnabled = true;
        opts.smartEnabled = false;
        opts.keywords = {QStringLiteral("作词")};

        const LyricsDocument filtered = LyricsFilter::apply(doc, opts);
        QCOMPARE(filtered.lines.size(), 2);
        QCOMPARE(filtered.lines.at(0).content, QStringLiteral("hello world"));
        QCOMPARE(filtered.lines.at(1).content, QStringLiteral("keep me"));
    }

    void keyword_filter_supports_regex_prefix()
    {
        LyricsDocument doc;
        doc.lines = {
            lineAt(0.0, QStringLiteral("artist: Foo")),
            lineAt(1.0, QStringLiteral("real lyric")),
        };

        LyricsFilterOptions opts;
        opts.keywordEnabled = true;
        opts.smartEnabled = false;
        opts.keywords = {QStringLiteral("/(by|title|song|album|artist|singer|lyrics)\\s*[:：∶]")};

        const LyricsDocument filtered = LyricsFilter::apply(doc, opts);
        QCOMPARE(filtered.lines.size(), 1);
        QCOMPARE(filtered.lines.at(0).content, QStringLiteral("real lyric"));
    }

    void keyword_filter_disabled_keeps_all()
    {
        LyricsDocument doc;
        doc.lines = {
            lineAt(0.0, QStringLiteral("作词：某人")),
            lineAt(1.0, QStringLiteral("lyric")),
        };

        LyricsFilterOptions opts;
        opts.keywordEnabled = false;
        opts.smartEnabled = false;
        opts.keywords = {QStringLiteral("作词")};

        const LyricsDocument filtered = LyricsFilter::apply(doc, opts);
        QCOMPARE(filtered.lines.size(), 2);
    }

    void smart_filter_drops_empty_and_whitespace()
    {
        LyricsDocument doc;
        doc.lines = {
            lineAt(0.0, QStringLiteral("keep")),
            lineAt(1.0, QString()),
            lineAt(2.0, QStringLiteral("   \t  ")),
            lineAt(3.0, QStringLiteral("also keep")),
        };

        LyricsFilterOptions opts;
        opts.keywordEnabled = false;
        opts.smartEnabled = true;

        const LyricsDocument filtered = LyricsFilter::apply(doc, opts);
        QCOMPARE(filtered.lines.size(), 2);
        QCOMPARE(filtered.lines.at(0).content, QStringLiteral("keep"));
        QCOMPARE(filtered.lines.at(1).content, QStringLiteral("also keep"));
    }

    void smart_filter_drops_punctuation_only()
    {
        LyricsDocument doc;
        doc.lines = {
            lineAt(0.0, QStringLiteral("....")),
            lineAt(1.0, QStringLiteral("♪♪")),
            lineAt(2.0, QStringLiteral("— —")),
            lineAt(3.0, QStringLiteral("hello.")),
        };

        LyricsFilterOptions opts;
        opts.keywordEnabled = false;
        opts.smartEnabled = true;

        const LyricsDocument filtered = LyricsFilter::apply(doc, opts);
        QCOMPARE(filtered.lines.size(), 1);
        QCOMPARE(filtered.lines.at(0).content, QStringLiteral("hello."));
    }

    void smart_filter_drops_metadata_like_lines()
    {
        LyricsDocument doc;
        doc.lines = {
            lineAt(0.0, QStringLiteral("Album：Demo")),
            lineAt(1.0, QStringLiteral("www.example.com")),
            lineAt(2.0, QStringLiteral(".")),
            lineAt(3.0, QStringLiteral("真正的歌词")),
        };

        LyricsFilterOptions opts;
        opts.keywordEnabled = false;
        opts.smartEnabled = true;

        const LyricsDocument filtered = LyricsFilter::apply(doc, opts);
        QCOMPARE(filtered.lines.size(), 1);
        QCOMPARE(filtered.lines.at(0).content, QStringLiteral("真正的歌词"));
    }

    void apply_preserves_translation_and_timing()
    {
        LyricsDocument doc;
        doc.offsetMs = 100;
        doc.sourceId = QStringLiteral("test");
        doc.lines = {
            lineAt(1.5, QStringLiteral("keep"), QStringLiteral("保留")),
            lineAt(2.0, QString()),
        };

        LyricsFilterOptions opts;
        opts.keywordEnabled = false;
        opts.smartEnabled = true;

        const LyricsDocument filtered = LyricsFilter::apply(doc, opts);
        QCOMPARE(filtered.lines.size(), 1);
        QCOMPARE(filtered.offsetMs, 100);
        QCOMPARE(filtered.sourceId, QStringLiteral("test"));
        QCOMPARE(filtered.lines.at(0).positionSec, 1.5);
        QCOMPARE(filtered.lines.at(0).translation, QStringLiteral("保留"));
    }

    void default_keywords_are_non_empty()
    {
        QVERIFY(!LyricsFilter::defaultKeywords().isEmpty());
    }
};

QTEST_MAIN(TestLyricsFilter)
#include "test_lyrics_filter.moc"
