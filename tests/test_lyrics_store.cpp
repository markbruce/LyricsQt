#include <QtTest>

#include <lyricsqt/AppSettings.h>
#include <lyricsqt/LyricsStore.h>
#include <lyricsqt/TrackInfo.h>

#include <QFile>
#include <QSettings>
#include <QTemporaryDir>

class TestLyricsStore : public QObject
{
    Q_OBJECT
private slots:
    void cacheFileName_basic()
    {
        lyricsqt::AppSettings settings(QStringLiteral("lyricsqt-test"), QStringLiteral("LyricsStoreCache"));
        lyricsqt::LyricsStore store(&settings);

        lyricsqt::TrackInfo track;
        track.title = QStringLiteral("Hello");
        track.artist = QStringLiteral("World");
        QCOMPARE(store.cacheFileName(track), QStringLiteral("Hello - World.lrcx"));
    }

    void cacheFileName_sanitizesSlash()
    {
        lyricsqt::AppSettings settings(QStringLiteral("lyricsqt-test"), QStringLiteral("LyricsStoreSlash"));
        lyricsqt::LyricsStore store(&settings);

        lyricsqt::TrackInfo track;
        track.title = QStringLiteral("a/b");
        track.artist = QStringLiteral("c/d");
        QCOMPARE(store.cacheFileName(track), QStringLiteral("a:b - c:d.lrcx"));
    }

    void loadLocal_fromSavingPath()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        lyricsqt::AppSettings settings(QStringLiteral("lyricsqt-test"), QStringLiteral("LyricsStoreLoad"));
        QSettings raw(QStringLiteral("lyricsqt-test"), QStringLiteral("LyricsStoreLoad"));
        raw.clear();
        raw.sync();

        lyricsqt::AppSettings fresh(QStringLiteral("lyricsqt-test"), QStringLiteral("LyricsStoreLoad"));
        fresh.setLyricsSavingPath(tmp.path());
        fresh.setLoadLyricsBesideTrack(false);

        lyricsqt::TrackInfo track;
        track.title = QStringLiteral("Demo");
        track.artist = QStringLiteral("Artist");

        const QString path = tmp.filePath(QStringLiteral("Demo - Artist.lrc"));
        {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
            file.write("[00:01.00]hello\n[00:02.00]world\n");
        }

        lyricsqt::LyricsStore store(&fresh);
        const auto doc = store.loadLocal(track);
        QVERIFY(doc.has_value());
        QCOMPARE(doc->lines.size(), 2);
        QCOMPARE(doc->lines[0].content, QStringLiteral("hello"));
    }
};

QTEST_MAIN(TestLyricsStore)
#include "test_lyrics_store.moc"
