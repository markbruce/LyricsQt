#include "LyricsHudWindow.h"

#include <lyricsqt/AppSettings.h>
#include <lyricsqt/LrcParser.h>
#include <lyricsqt/LyricsFilter.h>
#include <lyricsqt/LyricsSession.h>
#include <lyricsqt/LyricsStore.h>
#include <lyricsqt/PlayerService.h>
#include <lyricsqt/ProviderHub.h>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
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
                                 lyricsqt::ProviderHub *providers,
                                 lyricsqt::AppSettings *settings,
                                 QWidget *parent)
    : QWidget(parent)
    , m_session(session)
    , m_player(player)
    , m_store(store)
    , m_providers(providers)
    , m_settings(settings)
{
    Q_ASSERT(m_session);
    Q_ASSERT(m_player);
    Q_ASSERT(m_store);

    setWindowTitle(QStringLiteral("LyricsQt HUD"));
    setAcceptDrops(true);
    resize(480, 640);

    auto *menuBar = new QMenuBar(this);
    auto *lyricsMenu = menuBar->addMenu(QStringLiteral("Lyrics"));
    lyricsMenu->addAction(QStringLiteral("Search lyrics…"), this, &LyricsHudWindow::searchLyricsRequested);
    lyricsMenu->addAction(QStringLiteral("Wrong lyrics"), this, &LyricsHudWindow::wrongLyricsRequested);

    m_list = new QListWidget(this);
    m_list->setUniformItemSizes(false);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setStyleSheet(QStringLiteral(
        "QListWidget::item:selected {"
        "  background: #2a6bb5;"
        "  color: white;"
        "}"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setMenuBar(menuBar);
    layout->addWidget(m_list);

    connect(m_session, &lyricsqt::LyricsSession::lyricsChanged,
            this, &LyricsHudWindow::onLyricsChanged);
    connect(m_session, &lyricsqt::LyricsSession::currentLineChanged,
            this, &LyricsHudWindow::onCurrentLineChanged);
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &LyricsHudWindow::onItemDoubleClicked);
    if (m_settings) {
        connect(m_settings, &lyricsqt::AppSettings::changed, this, [this](const QString &key) {
            if (key == QLatin1String("PreferBilingualLyrics")) {
                rebuildList();
                highlightCurrentLine(m_session->currentLineIndex());
            }
        });
    }

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

    const bool bilingual = !m_settings || m_settings->preferBilingualLyrics();
    for (const auto &line : lyrics->lines) {
        QString text = line.content;
        if (bilingual && !line.translation.isEmpty()) {
            text += QLatin1Char('\n') + line.translation;
        }
        auto *item = new QListWidgetItem(text, m_list);
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
    lyricsqt::LyricsDocument doc = lyricsqt::LrcParser::parseFile(path);
    if (doc.lines.isEmpty()) {
        return false;
    }

    doc = lyricsqt::LyricsFilter::apply(doc, m_settings);
    if (doc.lines.isEmpty()) {
        return false;
    }

    // Cancel in-flight remote search so stale results cannot overwrite the import.
    if (m_providers) {
        m_providers->cancel();
    }

    m_session->setLyrics(doc);

    const lyricsqt::TrackInfo track = m_player->currentTrack();
    if (!track.isEmpty()) {
        m_store->save(track, doc);
    }
    return true;
}
