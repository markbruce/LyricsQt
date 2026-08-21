#include "SearchLyricsDialog.h"

#include <lyricsqt/AppSettings.h>
#include <lyricsqt/LyricsFilter.h>
#include <lyricsqt/LyricsSession.h>
#include <lyricsqt/LyricsStore.h>
#include <lyricsqt/PlayerService.h>
#include <lyricsqt/ProviderHub.h>

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

SearchLyricsDialog::SearchLyricsDialog(lyricsqt::PlayerService *player,
                                       lyricsqt::LyricsSession *session,
                                       lyricsqt::LyricsStore *store,
                                       lyricsqt::ProviderHub *providers,
                                       lyricsqt::AppSettings *settings,
                                       QWidget *parent)
    : QDialog(parent)
    , m_player(player)
    , m_session(session)
    , m_store(store)
    , m_providers(providers)
    , m_settings(settings)
{
    Q_ASSERT(m_player);
    Q_ASSERT(m_session);
    Q_ASSERT(m_store);
    Q_ASSERT(m_providers);
    Q_ASSERT(m_settings);

    setWindowTitle(QStringLiteral("Search Lyrics"));
    resize(720, 480);

    m_titleEdit = new QLineEdit(this);
    m_artistEdit = new QLineEdit(this);

    m_searchButton = new QPushButton(QStringLiteral("Search"), this);
    m_applyButton = new QPushButton(QStringLiteral("Apply"), this);
    m_applyButton->setEnabled(false);

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("Title"), m_titleEdit);
    form->addRow(QStringLiteral("Artist"), m_artistEdit);

    auto *queryRow = new QHBoxLayout;
    queryRow->addLayout(form, 1);
    queryRow->addWidget(m_searchButton, 0, Qt::AlignBottom);

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Title"),
        QStringLiteral("Artist"),
        QStringLiteral("Source"),
        QStringLiteral("Quality"),
    });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(m_applyButton, QDialogButtonBox::AcceptRole);
    buttons->addButton(QDialogButtonBox::Cancel);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(queryRow);
    layout->addWidget(new QLabel(QStringLiteral("Results"), this));
    layout->addWidget(m_table, 1);
    layout->addWidget(buttons);

    connect(m_searchButton, &QPushButton::clicked, this, &SearchLyricsDialog::onSearchClicked);
    connect(m_titleEdit, &QLineEdit::returnPressed, this, &SearchLyricsDialog::onSearchClicked);
    connect(m_artistEdit, &QLineEdit::returnPressed, this, &SearchLyricsDialog::onSearchClicked);
    connect(m_applyButton, &QPushButton::clicked, this, &SearchLyricsDialog::onApplyClicked);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() {
        m_applyButton->setEnabled(selectedRow() >= 0);
    });
    connect(m_table, &QTableWidget::itemDoubleClicked, this, [this]() {
        if (selectedRow() >= 0) {
            onApplyClicked();
        }
    });

    connect(m_providers, &lyricsqt::ProviderHub::candidatesReady,
            this, &SearchLyricsDialog::onCandidatesReady);
    connect(m_providers, &lyricsqt::ProviderHub::searchFinished,
            this, &SearchLyricsDialog::onSearchFinished);

    const lyricsqt::TrackInfo track = m_player->currentTrack();
    m_titleEdit->setText(track.title);
    m_artistEdit->setText(track.artist);
}

void SearchLyricsDialog::reloadFromCurrentTrack()
{
    const lyricsqt::TrackInfo track = m_player->currentTrack();
    m_titleEdit->setText(track.title);
    m_artistEdit->setText(track.artist);
    if (!track.isEmpty()) {
        onSearchClicked();
    }
}

void SearchLyricsDialog::onSearchClicked()
{
    m_results.clear();
    populateResults({});
    m_applyButton->setEnabled(false);

    m_searchTrack = queryTrack();
    if (m_searchTrack.title.trimmed().isEmpty()) {
        return;
    }

    m_searching = true;
    m_searchButton->setEnabled(false);
    m_providers->searchCollecting(m_searchTrack);
}

void SearchLyricsDialog::onApplyClicked()
{
    const int row = selectedRow();
    if (row < 0 || row >= m_results.size()) {
        return;
    }

    const lyricsqt::TrackInfo current = m_player->currentTrack();
    if (current.isEmpty()) {
        return;
    }

    // Stop in-flight search so a late auto-result cannot overwrite the choice.
    m_providers->cancel();
    m_searching = false;
    m_searchButton->setEnabled(true);

    if (!current.id.isEmpty()) {
        QStringList ids = m_settings->noSearchingTrackIds();
        ids.removeAll(current.id);
        m_settings->setNoSearchingTrackIds(ids);
    }

    const lyricsqt::LyricsDocument filtered =
        lyricsqt::LyricsFilter::apply(m_results.at(row), m_settings);
    m_session->setLyrics(filtered);
    m_store->save(current, filtered);
    accept();
}

void SearchLyricsDialog::onCandidatesReady(const lyricsqt::TrackInfo &track,
                                          const QVector<lyricsqt::LyricsDocument> &docs)
{
    if (!m_searching) {
        return;
    }
    // Match by query fields; id may differ when user edited title/artist.
    if (track.title != m_searchTrack.title || track.artist != m_searchTrack.artist) {
        return;
    }
    m_results = docs;
    populateResults(m_results);
}

void SearchLyricsDialog::onSearchFinished(const lyricsqt::TrackInfo &track, bool)
{
    if (!m_searching) {
        return;
    }
    if (track.title != m_searchTrack.title || track.artist != m_searchTrack.artist) {
        return;
    }
    m_searching = false;
    m_searchButton->setEnabled(true);
}

void SearchLyricsDialog::populateResults(const QVector<lyricsqt::LyricsDocument> &docs)
{
    m_table->setRowCount(docs.size());
    for (int row = 0; row < docs.size(); ++row) {
        const lyricsqt::LyricsDocument &doc = docs.at(row);
        m_table->setItem(row, 0, new QTableWidgetItem(
            doc.title.isEmpty() ? QStringLiteral("[lacking]") : doc.title));
        m_table->setItem(row, 1, new QTableWidgetItem(
            doc.artist.isEmpty() ? QStringLiteral("[lacking]") : doc.artist));
        m_table->setItem(row, 2, new QTableWidgetItem(
            doc.sourceId.isEmpty() ? QStringLiteral("[lacking]") : doc.sourceId));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(doc.quality, 'f', 2)));
    }
    if (!docs.isEmpty()) {
        m_table->selectRow(0);
    }
}

lyricsqt::TrackInfo SearchLyricsDialog::queryTrack() const
{
    lyricsqt::TrackInfo track = m_player->currentTrack();
    track.title = m_titleEdit->text().trimmed();
    track.artist = m_artistEdit->text().trimmed();
    return track;
}

int SearchLyricsDialog::selectedRow() const
{
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return -1;
    }
    return rows.first().row();
}
