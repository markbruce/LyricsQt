#include <QtTest>

#include <lyricsqt/AppSettings.h>

class TestAppSettings : public QObject
{
    Q_OBJECT
private slots:
    void defaults_desktopLyricsEnabled()
    {
        lyricsqt::AppSettings settings(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsDefaults"));
        QSettings raw(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsDefaults"));
        raw.clear();
        raw.sync();

        lyricsqt::AppSettings fresh(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsDefaults"));
        QVERIFY(fresh.desktopLyricsEnabled());
        QCOMPARE(fresh.menuBarLyricsEnabled(), false);
        QCOMPARE(fresh.exportEnabled(), false);
        QCOMPARE(fresh.preferredPlayerId(), QString());
        QCOMPARE(fresh.globalOffsetMs(), 0);
        QVERIFY(fresh.loadLyricsBesideTrack());
        QVERIFY(!fresh.lyricsSavingPath().isEmpty());
    }

    void roundtrip_offset()
    {
        lyricsqt::AppSettings settings(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsRoundtrip"));
        QSettings raw(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsRoundtrip"));
        raw.clear();
        raw.sync();

        lyricsqt::AppSettings fresh(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsRoundtrip"));
        fresh.setGlobalOffsetMs(250);
        QCOMPARE(fresh.globalOffsetMs(), 250);
    }
};

QTEST_MAIN(TestAppSettings)
#include "test_app_settings.moc"
