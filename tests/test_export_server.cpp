#include <lyricsqt/AppSettings.h>
#include <lyricsqt/ExportServer.h>
#include <lyricsqt/LyricsDocument.h>
#include <lyricsqt/LyricsSession.h>
#include <lyricsqt/PlayerService.h>

#include <QLocalSocket>
#include <QTemporaryDir>
#include <QTest>

class ExportServerTest : public QObject
{
    Q_OBJECT
private slots:
    void writesUtf8LinePlusNewlineToSocketClients();
};

void ExportServerTest::writesUtf8LinePlusNewlineToSocketClients()
{
    lyricsqt::AppSettings settings(QStringLiteral("lyricsqt-test"), QStringLiteral("ExportServer"));
    lyricsqt::PlayerService player(&settings);
    lyricsqt::LyricsSession session;

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString sock = tmp.path() + QStringLiteral("/lyricsqt-test.sock");

    lyricsqt::ExportServer server(&session, &player);
    server.setSocketPath(sock);
    QVERIFY2(server.start(), qPrintable(sock));

    lyricsqt::LyricsDocument doc;
    lyricsqt::LyricsLine line;
    line.positionSec = 0.0;
    line.content = QStringLiteral("你好 lyrics");
    doc.lines.append(line);
    session.setLyrics(doc);
    session.setPlayback(true, 0.0);
    QCOMPARE(server.currentLine(), QStringLiteral("你好 lyrics"));

    QLocalSocket client;
    client.connectToServer(sock);
    QVERIFY(client.waitForConnected(1000));
    QCOMPARE(client.state(), QLocalSocket::ConnectedState);

    // Allow the server-side newConnection handler to write the snapshot.
    QTRY_VERIFY_WITH_TIMEOUT(client.bytesAvailable() > 0, 1000);
    QCOMPARE(client.readAll(), QStringLiteral("你好 lyrics\n").toUtf8());

    session.clearLyrics();
    QTRY_VERIFY_WITH_TIMEOUT(client.bytesAvailable() > 0, 1000);
    QCOMPARE(client.readAll(), QByteArray("\n"));

    server.stop();
}

QTEST_MAIN(ExportServerTest)
#include "test_export_server.moc"
