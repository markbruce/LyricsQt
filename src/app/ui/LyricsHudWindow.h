#pragma once

#include <QWidget>

class QListWidget;
class QListWidgetItem;

namespace lyricsqt {
class AppSettings;
class LyricsSession;
class LyricsStore;
class PlayerService;
class ProviderHub;
}

class LyricsHudWindow : public QWidget
{
    Q_OBJECT
public:
    LyricsHudWindow(lyricsqt::LyricsSession *session,
                    lyricsqt::PlayerService *player,
                    lyricsqt::LyricsStore *store,
                    lyricsqt::ProviderHub *providers = nullptr,
                    lyricsqt::AppSettings *settings = nullptr,
                    QWidget *parent = nullptr);

signals:
    void searchLyricsRequested();
    void wrongLyricsRequested();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void onLyricsChanged();
    void onCurrentLineChanged(int index);
    void onItemDoubleClicked(QListWidgetItem *item);
    void rebuildList();
    void highlightCurrentLine(int index);
    bool importLyricsFile(const QString &path);

    lyricsqt::LyricsSession *m_session = nullptr;
    lyricsqt::PlayerService *m_player = nullptr;
    lyricsqt::LyricsStore *m_store = nullptr;
    lyricsqt::ProviderHub *m_providers = nullptr;
    lyricsqt::AppSettings *m_settings = nullptr;
    QListWidget *m_list = nullptr;
};
