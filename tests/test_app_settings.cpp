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
        QCOMPARE(fresh.desktopPositionXFactor(), 0.5);
        QCOMPARE(fresh.desktopPositionYFactor(), 0.5);
        QCOMPARE(fresh.disableLyricsWhenPaused(), false);
        QCOMPARE(fresh.enabledProviderIds(),
                 (QStringList{QStringLiteral("lrclib"), QStringLiteral("netease"),
                              QStringLiteral("qq"), QStringLiteral("kugou")}));
        QCOMPARE(fresh.noSearchingTrackIds(), QStringList());
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

    void roundtrip_position_factors()
    {
        lyricsqt::AppSettings settings(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsPos"));
        QSettings raw(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsPos"));
        raw.clear();
        raw.sync();

        lyricsqt::AppSettings fresh(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsPos"));
        fresh.setDesktopPositionXFactor(0.25);
        fresh.setDesktopPositionYFactor(0.75);
        QCOMPARE(fresh.desktopPositionXFactor(), 0.25);
        QCOMPARE(fresh.desktopPositionYFactor(), 0.75);
    }

    void roundtrip_enabled_providers()
    {
        lyricsqt::AppSettings settings(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsProviders"));
        QSettings raw(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsProviders"));
        raw.clear();
        raw.sync();

        lyricsqt::AppSettings fresh(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsProviders"));
        const QStringList ids{QStringLiteral("qq"), QStringLiteral("lrclib")};
        fresh.setEnabledProviderIds(ids);
        QCOMPARE(fresh.enabledProviderIds(), ids);
    }

    void no_searching_track_ids_helpers()
    {
        lyricsqt::AppSettings settings(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsNoSearch"));
        QSettings raw(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsNoSearch"));
        raw.clear();
        raw.sync();

        lyricsqt::AppSettings fresh(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsNoSearch"));
        QCOMPARE(fresh.isNoSearchingTrackId(QStringLiteral("track-1")), false);
        fresh.addNoSearchingTrackId(QStringLiteral("track-1"));
        fresh.addNoSearchingTrackId(QStringLiteral("track-1")); // no duplicate
        fresh.addNoSearchingTrackId(QString()); // ignored
        QVERIFY(fresh.isNoSearchingTrackId(QStringLiteral("track-1")));
        QCOMPARE(fresh.noSearchingTrackIds(), QStringList{QStringLiteral("track-1")});
        fresh.addNoSearchingTrackId(QStringLiteral("track-2"));
        QCOMPARE(fresh.noSearchingTrackIds(),
                 (QStringList{QStringLiteral("track-1"), QStringLiteral("track-2")}));
    }
};

QTEST_MAIN(TestAppSettings)
#include "test_app_settings.moc"
