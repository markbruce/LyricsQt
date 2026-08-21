#include "ExportAdaptor.h"

#include <lyricsqt/ExportServer.h>

namespace lyricsqt {

ExportAdaptor::ExportAdaptor(ExportServer *parent)
    : QDBusAbstractAdaptor(parent)
{
    setAutoRelaySignals(true);
}

QString ExportAdaptor::currentLine() const
{
    return server()->currentLine();
}

QString ExportAdaptor::title() const
{
    return server()->title();
}

QString ExportAdaptor::artist() const
{
    return server()->artist();
}

bool ExportAdaptor::playing() const
{
    return server()->playing();
}

ExportServer *ExportAdaptor::server() const
{
    return qobject_cast<ExportServer *>(parent());
}

} // namespace lyricsqt
