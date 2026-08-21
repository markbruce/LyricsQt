#pragma once

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QListWidget;

namespace lyricsqt {
class AppSettings;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PreferencesDialog(lyricsqt::AppSettings *settings, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;

private:
    QWidget *buildGeneralTab();
    QWidget *buildDisplayTab();
    QWidget *buildFilterTab();
    QWidget *buildAdvancedTab();

    void loadFromSettings();
    void applyPreferredPlayer();
    void applySavingPath();
    void applyFilterKeys();
    void applyEnabledProviders();
    void browseSavingPath();
    void addFilterKeyword();
    void removeSelectedFilterKeywords();
    void resetFilterKeywords();

    lyricsqt::AppSettings *m_settings = nullptr;

    // General
    QLineEdit *m_preferredPlayerEdit = nullptr;
    QCheckBox *m_autostartCheck = nullptr;
    QCheckBox *m_quitWithPlayerCheck = nullptr;
    QCheckBox *m_loadBesideTrackCheck = nullptr;
    QLineEdit *m_savingPathEdit = nullptr;
    QCheckBox *m_strictSearchCheck = nullptr;

    // Display
    QCheckBox *m_desktopLyricsCheck = nullptr;
    QCheckBox *m_trayLyricsCheck = nullptr;
    QCheckBox *m_bilingualCheck = nullptr;
    QCheckBox *m_disableWhenPausedCheck = nullptr;

    // Filter
    QCheckBox *m_filterEnabledCheck = nullptr;
    QCheckBox *m_smartFilterCheck = nullptr;
    QListWidget *m_filterKeysList = nullptr;
    QLineEdit *m_filterKeywordEdit = nullptr;

    // Advanced
    QListWidget *m_providersList = nullptr;
    QCheckBox *m_exportEnabledCheck = nullptr;

    bool m_loading = false;
};
