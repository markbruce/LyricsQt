#pragma once

#include <lyricsqt/LyricsDocument.h>
#include <lyricsqt/TrackInfo.h>

#include <QDialog>
#include <QVector>

class QLineEdit;
class QPushButton;
class QTableWidget;

namespace lyricsqt {
class AppSettings;
class LyricsSession;
class LyricsStore;
class PlayerService;
class ProviderHub;
}

class SearchLyricsDialog : public QDialog
{
    Q_OBJECT
public:
    SearchLyricsDialog(lyricsqt::PlayerService *player,
                       lyricsqt::LyricsSession *session,
                       lyricsqt::LyricsStore *store,
                       lyricsqt::ProviderHub *providers,
                       lyricsqt::AppSettings *settings,
                       QWidget *parent = nullptr);

    void reloadFromCurrentTrack();

private:
    void onSearchClicked();
    void onApplyClicked();
    void onCandidatesReady(const lyricsqt::TrackInfo &track,
                           const QVector<lyricsqt::LyricsDocument> &docs);
    void onSearchFinished(const lyricsqt::TrackInfo &track, bool found);
    void populateResults(const QVector<lyricsqt::LyricsDocument> &docs);
    lyricsqt::TrackInfo queryTrack() const;
    int selectedRow() const;

    lyricsqt::PlayerService *m_player = nullptr;
    lyricsqt::LyricsSession *m_session = nullptr;
    lyricsqt::LyricsStore *m_store = nullptr;
    lyricsqt::ProviderHub *m_providers = nullptr;
    lyricsqt::AppSettings *m_settings = nullptr;

    QLineEdit *m_titleEdit = nullptr;
    QLineEdit *m_artistEdit = nullptr;
    QPushButton *m_searchButton = nullptr;
    QPushButton *m_applyButton = nullptr;
    QTableWidget *m_table = nullptr;
    QVector<lyricsqt::LyricsDocument> m_results;
    lyricsqt::TrackInfo m_searchTrack;
    bool m_searching = false;
};
