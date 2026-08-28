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
        QCOMPARE(fresh.desktopLyricsWidth(), 720);
        QCOMPARE(fresh.desktopLyricsFontPt(), 30);
        QCOMPARE(fresh.desktopLyricsLocked(), false);
        QCOMPARE(fresh.desktopLyricsUnplayedColor(),
                 lyricsqt::AppSettings::defaultDesktopLyricsUnplayedColor());
        QCOMPARE(fresh.desktopLyricsPlayedColor(),
                 lyricsqt::AppSettings::defaultDesktopLyricsPlayedColor());
        QCOMPARE(fresh.desktopLyricsOutlineColor(),
                 lyricsqt::AppSettings::defaultDesktopLyricsOutlineColor());
        QCOMPARE(fresh.desktopLyricsTextOpacity(), 100);
        QCOMPARE(fresh.disableLyricsWhenPaused(), false);
        QCOMPARE(fresh.enabledProviderIds(),
                 (QStringList{QStringLiteral("lrclib"), QStringLiteral("netease"),
                              QStringLiteral("qq"), QStringLiteral("kugou")}));
        QCOMPARE(fresh.noSearchingTrackIds(), QStringList());
        QVERIFY(fresh.lyricsFilterEnabled());
        QVERIFY(fresh.lyricsSmartFilterEnabled());
        QVERIFY(!fresh.lyricsFilterKeys().isEmpty());
        QVERIFY(fresh.preferBilingualLyrics());
        QCOMPARE(fresh.autostartEnabled(), false);
        QCOMPARE(fresh.quitWithPlayer(), false);
        QCOMPARE(fresh.strictSearchEnabled(), false);
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
        fresh.setDesktopLyricsWidth(900);
        QCOMPARE(fresh.desktopLyricsWidth(), 900);
        fresh.setDesktopLyricsWidth(100); // clamped to minimum
        QCOMPARE(fresh.desktopLyricsWidth(), 360);
        fresh.setDesktopLyricsFontPt(40);
        QCOMPARE(fresh.desktopLyricsFontPt(), 40);
        fresh.setDesktopLyricsLocked(true);
        QCOMPARE(fresh.desktopLyricsLocked(), true);
        fresh.setDesktopLyricsUnplayedColor(QStringLiteral("#112233"));
        fresh.setDesktopLyricsPlayedColor(QStringLiteral("#AABBCCDD"));
        fresh.setDesktopLyricsOutlineColor(QStringLiteral("#010203"));
        fresh.setDesktopLyricsTextOpacity(55);
        QCOMPARE(fresh.desktopLyricsUnplayedColor(), QStringLiteral("#112233"));
        QCOMPARE(fresh.desktopLyricsPlayedColor(), QStringLiteral("#aabbccdd"));
        QCOMPARE(fresh.desktopLyricsOutlineColor(), QStringLiteral("#010203"));
        QCOMPARE(fresh.desktopLyricsTextOpacity(), 55);
        fresh.setDesktopLyricsOutlineColor(QStringLiteral("none"));
        QCOMPARE(fresh.desktopLyricsOutlineColor(), QStringLiteral("none"));
        fresh.setDesktopLyricsTextOpacity(3); // clamp to minimum
        QCOMPARE(fresh.desktopLyricsTextOpacity(), 10);
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

    void roundtrip_filter_and_bilingual()
    {
        lyricsqt::AppSettings settings(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsFilter"));
        QSettings raw(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsFilter"));
        raw.clear();
        raw.sync();

        lyricsqt::AppSettings fresh(QStringLiteral("lyricsqt-test"), QStringLiteral("AppSettingsFilter"));
        fresh.setLyricsFilterEnabled(false);
        fresh.setLyricsSmartFilterEnabled(false);
        fresh.setPreferBilingualLyrics(false);
        const QStringList keys{QStringLiteral("作词"), QStringLiteral("/^\\.$")};
        fresh.setLyricsFilterKeys(keys);

        QCOMPARE(fresh.lyricsFilterEnabled(), false);
        QCOMPARE(fresh.lyricsSmartFilterEnabled(), false);
        QCOMPARE(fresh.preferBilingualLyrics(), false);
        QCOMPARE(fresh.lyricsFilterKeys(), keys);
    }
};

QTEST_MAIN(TestAppSettings)
#include "test_app_settings.moc"
