#pragma once

#include <QDBusAbstractAdaptor>
#include <QString>

namespace lyricsqt {

class ExportServer;

/// D-Bus adaptor for org.lyricsqt.Export (panel / DE extension clients).
class ExportAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.lyricsqt.Export")
    Q_PROPERTY(QString CurrentLine READ currentLine)
    Q_PROPERTY(QString Title READ title)
    Q_PROPERTY(QString Artist READ artist)
    Q_PROPERTY(bool Playing READ playing)

public:
    explicit ExportAdaptor(ExportServer *parent);

    QString currentLine() const;
    QString title() const;
    QString artist() const;
    bool playing() const;

private:
    ExportServer *server() const;
};

} // namespace lyricsqt
