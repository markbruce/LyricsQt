#include "LyricsHudWindow.h"

#include <lyricsqt/LrcParser.h>
#include <lyricsqt/LyricsSession.h>
#include <lyricsqt/LyricsStore.h>
#include <lyricsqt/PlayerService.h>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QListWidget>
#include <QMimeData>
#include <QUrl>
#include <QVBoxLayout>

namespace {

bool isLyricsFile(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == QLatin1String("lrc") || suffix == QLatin1String("lrcx");
}

} // namespace

LyricsHudWindow::LyricsHudWindow(lyricsqt::LyricsSession *session,
                                 lyricsqt::PlayerService *player,
                                 lyricsqt::LyricsStore *store,
                                 QWidget *parent)
    : QWidget(parent)
    , m_session(session)
    , m_player(player)
    , m_store(store)
{
    Q_ASSERT(m_session);
    Q_ASSERT(m_player);
    Q_ASSERT(m_store);

    setWindowTitle(QStringLiteral("LyricsQt HUD"));
    setAcceptDrops(true);
    resize(480, 640);

    m_list = new QListWidget(this);
    m_list->setUniformItemSizes(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setStyleSheet(QStringLiteral(
        "QListWidget::item:selected {"
        "  background: #2a6bb5;"
        "  color: white;"
        "}"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(m_list);

    connect(m_session, &lyricsqt::LyricsSession::lyricsChanged,
            this, &LyricsHudWindow::onLyricsChanged);
    connect(m_session, &lyricsqt::LyricsSession::currentLineChanged,
            this, &LyricsHudWindow::onCurrentLineChanged);
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &LyricsHudWindow::onItemDoubleClicked);

    rebuildList();
    highlightCurrentLine(m_session->currentLineIndex());
}

void LyricsHudWindow::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mime = event->mimeData();
    if (!mime || !mime->hasUrls()) {
        event->ignore();
        return;
    }

    for (const QUrl &url : mime->urls()) {
        if (url.isLocalFile() && isLyricsFile(url.toLocalFile())) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void LyricsHudWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mime = event->mimeData();
    if (!mime || !mime->hasUrls()) {
        event->ignore();
        return;
    }

    for (const QUrl &url : mime->urls()) {
        if (!url.isLocalFile()) {
            continue;
        }
        const QString path = url.toLocalFile();
        if (isLyricsFile(path) && importLyricsFile(path)) {
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void LyricsHudWindow::onLyricsChanged()
{
    rebuildList();
    highlightCurrentLine(m_session->currentLineIndex());
}

void LyricsHudWindow::onCurrentLineChanged(int index)
{
    highlightCurrentLine(index);
}

void LyricsHudWindow::onItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) {
        return;
    }
    m_player->seekTo(item->data(Qt::UserRole).toDouble());
}

void LyricsHudWindow::rebuildList()
{
    m_list->clear();

    const auto *lyrics = m_session->lyrics();
    if (!lyrics) {
        return;
    }

    for (const auto &line : lyrics->lines) {
        auto *item = new QListWidgetItem(line.content, m_list);
        item->setData(Qt::UserRole, line.positionSec);
        if (!line.translation.isEmpty()) {
            item->setToolTip(line.translation);
        }
    }
}

void LyricsHudWindow::highlightCurrentLine(int index)
{
    if (index < 0 || index >= m_list->count()) {
        m_list->clearSelection();
        m_list->setCurrentRow(-1);
        return;
    }

    m_list->setCurrentRow(index);
    if (QListWidgetItem *item = m_list->item(index)) {
        m_list->scrollToItem(item, QAbstractItemView::PositionAtCenter);
    }
}

bool LyricsHudWindow::importLyricsFile(const QString &path)
{
    const lyricsqt::LyricsDocument doc = lyricsqt::LrcParser::parseFile(path);
    if (doc.lines.isEmpty()) {
        return false;
    }

    m_session->setLyrics(doc);

    const lyricsqt::TrackInfo track = m_player->currentTrack();
    if (!track.isEmpty()) {
        m_store->save(track, doc);
    }
    return true;
}
