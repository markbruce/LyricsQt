#include <QtTest>

#include <lyricsqt/AppSettings.h>
#include <lyricsqt/LyricsStore.h>
#include <lyricsqt/TrackInfo.h>

#include <QFile>
#include <QFileInfo>
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

    void removeLocal_deletesCacheFiles()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        QSettings raw(QStringLiteral("lyricsqt-test"), QStringLiteral("LyricsStoreRemove"));
        raw.clear();
        raw.sync();

        lyricsqt::AppSettings fresh(QStringLiteral("lyricsqt-test"), QStringLiteral("LyricsStoreRemove"));
        fresh.setLyricsSavingPath(tmp.path());
        fresh.setLoadLyricsBesideTrack(false);

        lyricsqt::TrackInfo track;
        track.title = QStringLiteral("Wrong");
        track.artist = QStringLiteral("Song");

        const QString lrcx = tmp.filePath(QStringLiteral("Wrong - Song.lrcx"));
        const QString lrc = tmp.filePath(QStringLiteral("Wrong - Song.lrc"));
        {
            QFile a(lrcx);
            QVERIFY(a.open(QIODevice::WriteOnly | QIODevice::Text));
            a.write("[00:01.00]bad\n");
        }
        {
            QFile b(lrc);
            QVERIFY(b.open(QIODevice::WriteOnly | QIODevice::Text));
            b.write("[00:01.00]also bad\n");
        }

        lyricsqt::LyricsStore store(&fresh);
        QVERIFY(store.loadLocal(track).has_value());
        QVERIFY(store.removeLocal(track));
        QVERIFY(!QFileInfo::exists(lrcx));
        QVERIFY(!QFileInfo::exists(lrc));
        QVERIFY(!store.loadLocal(track).has_value());
        QVERIFY(!store.removeLocal(track)); // already gone
    }
};

QTEST_MAIN(TestLyricsStore)
#include "test_lyrics_store.moc"
