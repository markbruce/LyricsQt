#include <QApplication>
#include <QLabel>

#include <lyricsqt/Version.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("lyricsqt"));
    QApplication::setApplicationName(QStringLiteral("LyricsQt"));

    QLabel label(QStringLiteral("LyricsQt %1").arg(QLatin1String(lyricsqt::version())));
    label.resize(320, 80);
    label.show();

    return app.exec();
}
