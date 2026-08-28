#include "PreferencesDialog.h"

#include <lyricsqt/AppSettings.h>
#include <lyricsqt/LyricsFilter.h>

#include <functional>

#include <QAbstractItemView>
#include <QCheckBox>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QShowEvent>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {

void setCheckSilent(QCheckBox *box, bool checked)
{
    const QSignalBlocker blocker(box);
    box->setChecked(checked);
}

} // namespace

PreferencesDialog::PreferencesDialog(lyricsqt::AppSettings *settings, QWidget *parent)
    : QDialog(parent)
    , m_settings(settings)
{
    Q_ASSERT(m_settings);

    setWindowTitle(QStringLiteral("Preferences"));
    resize(520, 420);

    auto *tabs = new QTabWidget(this);
    tabs->addTab(buildGeneralTab(), QStringLiteral("General"));
    tabs->addTab(buildDisplayTab(), QStringLiteral("Display"));
    tabs->addTab(buildFilterTab(), QStringLiteral("Filter"));
    tabs->addTab(buildAdvancedTab(), QStringLiteral("Advanced"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(buttons);

    loadFromSettings();
}

void PreferencesDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    loadFromSettings();
}

QWidget *PreferencesDialog::buildGeneralTab()
{
    auto *page = new QWidget(this);
    auto *form = new QFormLayout(page);

    m_preferredPlayerEdit = new QLineEdit(page);
    m_preferredPlayerEdit->setPlaceholderText(QStringLiteral("empty = auto (e.g. spotify)"));
    connect(m_preferredPlayerEdit, &QLineEdit::editingFinished,
            this, &PreferencesDialog::applyPreferredPlayer);

    m_autostartCheck = new QCheckBox(QStringLiteral("Launch at login"), page);
    connect(m_autostartCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading) {
            m_settings->setAutostartEnabled(checked);
        }
    });

    m_quitWithPlayerCheck = new QCheckBox(QStringLiteral("Quit when preferred player exits"), page);
    connect(m_quitWithPlayerCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading) {
            m_settings->setQuitWithPlayer(checked);
        }
    });

    m_loadBesideTrackCheck = new QCheckBox(QStringLiteral("Load lyrics beside track"), page);
    connect(m_loadBesideTrackCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading) {
            m_settings->setLoadLyricsBesideTrack(checked);
        }
    });

    m_savingPathEdit = new QLineEdit(page);
    auto *browse = new QPushButton(QStringLiteral("Browse…"), page);
    connect(browse, &QPushButton::clicked, this, &PreferencesDialog::browseSavingPath);
    connect(m_savingPathEdit, &QLineEdit::editingFinished,
            this, &PreferencesDialog::applySavingPath);

    auto *pathRow = new QWidget(page);
    auto *pathLayout = new QHBoxLayout(pathRow);
    pathLayout->setContentsMargins(0, 0, 0, 0);
    pathLayout->addWidget(m_savingPathEdit, 1);
    pathLayout->addWidget(browse);

    m_strictSearchCheck = new QCheckBox(QStringLiteral("Strict search"), page);
    connect(m_strictSearchCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading) {
            m_settings->setStrictSearchEnabled(checked);
        }
    });

    form->addRow(QStringLiteral("Preferred player id"), m_preferredPlayerEdit);
    form->addRow(QString(), m_autostartCheck);
    form->addRow(QString(), m_quitWithPlayerCheck);
    form->addRow(QString(), m_loadBesideTrackCheck);
    form->addRow(QStringLiteral("Lyrics saving path"), pathRow);
    form->addRow(QString(), m_strictSearchCheck);
    return page;
}

QWidget *PreferencesDialog::buildDisplayTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    m_desktopLyricsCheck = new QCheckBox(QStringLiteral("Enable desktop lyrics"), page);
    connect(m_desktopLyricsCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading) {
            m_settings->setDesktopLyricsEnabled(checked);
        }
    });

    m_trayLyricsCheck = new QCheckBox(QStringLiteral("Show current line in tray tooltip"), page);
    connect(m_trayLyricsCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading) {
            m_settings->setMenuBarLyricsEnabled(checked);
        }
    });

    m_bilingualCheck = new QCheckBox(QStringLiteral("Prefer bilingual lyrics"), page);
    connect(m_bilingualCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading) {
            m_settings->setPreferBilingualLyrics(checked);
        }
    });

    m_disableWhenPausedCheck = new QCheckBox(QStringLiteral("Hide desktop lyrics when paused"), page);
    connect(m_disableWhenPausedCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading) {
            m_settings->setDisableLyricsWhenPaused(checked);
        }
    });

    m_desktopFontSpin = new QSpinBox(page);
    m_desktopFontSpin->setRange(14, 72);
    m_desktopFontSpin->setSuffix(QStringLiteral(" pt"));
    connect(m_desktopFontSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (!m_loading) {
            m_settings->setDesktopLyricsFontPt(value);
        }
    });

    auto *fontRow = new QWidget(page);
    auto *fontLayout = new QHBoxLayout(fontRow);
    fontLayout->setContentsMargins(0, 0, 0, 0);
    fontLayout->addWidget(new QLabel(QStringLiteral("Desktop lyrics font size"), page));
    fontLayout->addStretch(1);
    fontLayout->addWidget(m_desktopFontSpin);

    auto makeColorRow = [this, page](const QString &label, QPushButton *&button) {
        auto *row = new QWidget(page);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->addWidget(new QLabel(label, page));
        rowLayout->addStretch(1);
        button = new QPushButton(page);
        button->setFixedSize(88, 28);
        button->setCursor(Qt::PointingHandCursor);
        rowLayout->addWidget(button);
        return row;
    };

    auto *unplayedRow = makeColorRow(QStringLiteral("Unplayed color"), m_unplayedColorButton);
    auto *playedRow = makeColorRow(QStringLiteral("Played (karaoke) color"), m_playedColorButton);
    auto *outlineRow = makeColorRow(QStringLiteral("Outline / border color"), m_outlineColorButton);
    auto *outlineNoneButton = new QPushButton(QStringLiteral("None"), page);
    outlineNoneButton->setToolTip(QStringLiteral("No outline / border"));
    outlineNoneButton->setFixedHeight(28);
    if (auto *outlineLayout = qobject_cast<QHBoxLayout *>(outlineRow->layout())) {
        outlineLayout->addWidget(outlineNoneButton);
    }

    connect(m_unplayedColorButton, &QPushButton::clicked, this, [this]() {
        pickDesktopColor(m_unplayedColorButton,
                         QStringLiteral("Unplayed color"),
                         [this]() { return m_settings->desktopLyricsUnplayedColor(); },
                         [this](const QString &c) { m_settings->setDesktopLyricsUnplayedColor(c); });
    });
    connect(m_playedColorButton, &QPushButton::clicked, this, [this]() {
        pickDesktopColor(m_playedColorButton,
                         QStringLiteral("Played color"),
                         [this]() { return m_settings->desktopLyricsPlayedColor(); },
                         [this](const QString &c) { m_settings->setDesktopLyricsPlayedColor(c); });
    });
    connect(m_outlineColorButton, &QPushButton::clicked, this, [this]() {
        pickDesktopColor(m_outlineColorButton,
                         QStringLiteral("Outline color"),
                         [this]() { return m_settings->desktopLyricsOutlineColor(); },
                         [this](const QString &c) { m_settings->setDesktopLyricsOutlineColor(c); });
    });
    connect(outlineNoneButton, &QPushButton::clicked, this, [this]() {
        if (m_loading) {
            return;
        }
        m_settings->setDesktopLyricsOutlineColor(QStringLiteral("none"));
        syncColorButton(m_outlineColorButton, m_settings->desktopLyricsOutlineColor());
    });

    m_textOpacitySpin = new QSpinBox(page);
    m_textOpacitySpin->setRange(10, 100);
    m_textOpacitySpin->setSuffix(QStringLiteral(" %"));
    connect(m_textOpacitySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (!m_loading) {
            m_settings->setDesktopLyricsTextOpacity(value);
        }
    });

    auto *opacityRow = new QWidget(page);
    auto *opacityLayout = new QHBoxLayout(opacityRow);
    opacityLayout->setContentsMargins(0, 0, 0, 0);
    opacityLayout->addWidget(new QLabel(QStringLiteral("Text opacity"), page));
    opacityLayout->addStretch(1);
    opacityLayout->addWidget(m_textOpacitySpin);

    auto *positionNote = new QLabel(
        QStringLiteral("Unlocked: hover for A- / lock / A+ (centered). "
                       "Drag to move, drag edges to resize, scroll or A± for font size. "
                       "Lock makes lyrics click-through; unlock from the tray menu "
                       "(\"Lock Desktop Lyrics\"). Gray background only while unlocked and hovering."),
        page);
    positionNote->setWordWrap(true);
    positionNote->setStyleSheet(QStringLiteral("color: palette(mid);"));

    layout->addWidget(m_desktopLyricsCheck);
    layout->addWidget(m_trayLyricsCheck);
    layout->addWidget(m_bilingualCheck);
    layout->addWidget(m_disableWhenPausedCheck);
    layout->addWidget(fontRow);
    layout->addWidget(unplayedRow);
    layout->addWidget(playedRow);
    layout->addWidget(outlineRow);
    layout->addWidget(opacityRow);
    layout->addSpacing(12);
    layout->addWidget(positionNote);
    layout->addStretch(1);
    return page;
}

QWidget *PreferencesDialog::buildFilterTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    m_filterEnabledCheck = new QCheckBox(QStringLiteral("Enable lyrics filter"), page);
    connect(m_filterEnabledCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading) {
            m_settings->setLyricsFilterEnabled(checked);
        }
    });

    m_smartFilterCheck = new QCheckBox(QStringLiteral("Enable smart filter"), page);
    connect(m_smartFilterCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading) {
            m_settings->setLyricsSmartFilterEnabled(checked);
        }
    });

    m_filterKeysList = new QListWidget(page);
    m_filterKeysList->setSelectionMode(QAbstractItemView::ExtendedSelection);

    m_filterKeywordEdit = new QLineEdit(page);
    m_filterKeywordEdit->setPlaceholderText(QStringLiteral("Keyword or /^regex$/"));
    connect(m_filterKeywordEdit, &QLineEdit::returnPressed,
            this, &PreferencesDialog::addFilterKeyword);

    auto *addButton = new QPushButton(QStringLiteral("Add"), page);
    connect(addButton, &QPushButton::clicked, this, &PreferencesDialog::addFilterKeyword);

    auto *removeButton = new QPushButton(QStringLiteral("Remove"), page);
    connect(removeButton, &QPushButton::clicked, this, &PreferencesDialog::removeSelectedFilterKeywords);

    auto *resetButton = new QPushButton(QStringLiteral("Reset defaults"), page);
    connect(resetButton, &QPushButton::clicked, this, &PreferencesDialog::resetFilterKeywords);

    auto *editRow = new QHBoxLayout;
    editRow->addWidget(m_filterKeywordEdit, 1);
    editRow->addWidget(addButton);
    editRow->addWidget(removeButton);
    editRow->addWidget(resetButton);

    layout->addWidget(m_filterEnabledCheck);
    layout->addWidget(m_smartFilterCheck);
    layout->addWidget(new QLabel(QStringLiteral("Filter keywords"), page));
    layout->addWidget(m_filterKeysList, 1);
    layout->addLayout(editRow);
    return page;
}

QWidget *PreferencesDialog::buildAdvancedTab()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    layout->addWidget(new QLabel(QStringLiteral("Enabled lyrics providers (order = checklist order)"), page));

    m_providersList = new QListWidget(page);
    for (const QString &id : lyricsqt::AppSettings::defaultProviderIds()) {
        auto *item = new QListWidgetItem(id, m_providersList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
    connect(m_providersList, &QListWidget::itemChanged, this, [this](QListWidgetItem *) {
        if (!m_loading) {
            applyEnabledProviders();
        }
    });

    m_exportEnabledCheck = new QCheckBox(QStringLiteral("Enable panel export server"), page);
    connect(m_exportEnabledCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (!m_loading) {
            m_settings->setExportEnabled(checked);
        }
    });

    layout->addWidget(m_providersList, 1);
    layout->addWidget(m_exportEnabledCheck);
    return page;
}

void PreferencesDialog::loadFromSettings()
{
    m_loading = true;

    m_preferredPlayerEdit->setText(m_settings->preferredPlayerId());
    setCheckSilent(m_autostartCheck, m_settings->autostartEnabled());
    setCheckSilent(m_quitWithPlayerCheck, m_settings->quitWithPlayer());
    setCheckSilent(m_loadBesideTrackCheck, m_settings->loadLyricsBesideTrack());
    m_savingPathEdit->setText(m_settings->lyricsSavingPath());
    setCheckSilent(m_strictSearchCheck, m_settings->strictSearchEnabled());

    setCheckSilent(m_desktopLyricsCheck, m_settings->desktopLyricsEnabled());
    setCheckSilent(m_trayLyricsCheck, m_settings->menuBarLyricsEnabled());
    setCheckSilent(m_bilingualCheck, m_settings->preferBilingualLyrics());
    setCheckSilent(m_disableWhenPausedCheck, m_settings->disableLyricsWhenPaused());
    if (m_desktopFontSpin) {
        const QSignalBlocker blocker(m_desktopFontSpin);
        m_desktopFontSpin->setValue(m_settings->desktopLyricsFontPt());
    }
    syncColorButton(m_unplayedColorButton, m_settings->desktopLyricsUnplayedColor());
    syncColorButton(m_playedColorButton, m_settings->desktopLyricsPlayedColor());
    syncColorButton(m_outlineColorButton, m_settings->desktopLyricsOutlineColor());
    if (m_textOpacitySpin) {
        const QSignalBlocker blocker(m_textOpacitySpin);
        m_textOpacitySpin->setValue(m_settings->desktopLyricsTextOpacity());
    }

    setCheckSilent(m_filterEnabledCheck, m_settings->lyricsFilterEnabled());
    setCheckSilent(m_smartFilterCheck, m_settings->lyricsSmartFilterEnabled());
    m_filterKeysList->clear();
    for (const QString &key : m_settings->lyricsFilterKeys()) {
        m_filterKeysList->addItem(key);
    }

    const QStringList enabled = m_settings->enabledProviderIds();
    {
        const QSignalBlocker blocker(m_providersList);
        for (int i = 0; i < m_providersList->count(); ++i) {
            QListWidgetItem *item = m_providersList->item(i);
            item->setCheckState(enabled.contains(item->text()) ? Qt::Checked : Qt::Unchecked);
        }
        // Reorder checked providers to match enabled order, then unchecked.
        QList<QListWidgetItem *> ordered;
        for (const QString &id : enabled) {
            for (int i = 0; i < m_providersList->count(); ++i) {
                if (m_providersList->item(i)->text() == id) {
                    ordered.append(m_providersList->takeItem(i));
                    break;
                }
            }
        }
        while (m_providersList->count() > 0) {
            ordered.append(m_providersList->takeItem(0));
        }
        for (QListWidgetItem *item : ordered) {
            m_providersList->addItem(item);
        }
    }

    setCheckSilent(m_exportEnabledCheck, m_settings->exportEnabled());

    m_loading = false;
}

void PreferencesDialog::applyPreferredPlayer()
{
    if (m_loading) {
        return;
    }
    m_settings->setPreferredPlayerId(m_preferredPlayerEdit->text().trimmed());
}

void PreferencesDialog::applySavingPath()
{
    if (m_loading) {
        return;
    }
    m_settings->setLyricsSavingPath(m_savingPathEdit->text().trimmed());
}

void PreferencesDialog::applyFilterKeys()
{
    QStringList keys;
    keys.reserve(m_filterKeysList->count());
    for (int i = 0; i < m_filterKeysList->count(); ++i) {
        const QString text = m_filterKeysList->item(i)->text().trimmed();
        if (!text.isEmpty()) {
            keys.append(text);
        }
    }
    m_settings->setLyricsFilterKeys(keys);
}

void PreferencesDialog::applyEnabledProviders()
{
    QStringList ids;
    for (int i = 0; i < m_providersList->count(); ++i) {
        QListWidgetItem *item = m_providersList->item(i);
        if (item->checkState() == Qt::Checked) {
            ids.append(item->text());
        }
    }
    if (ids.isEmpty()) {
        ids = lyricsqt::AppSettings::defaultProviderIds();
        const QSignalBlocker blocker(m_providersList);
        for (int i = 0; i < m_providersList->count(); ++i) {
            m_providersList->item(i)->setCheckState(Qt::Checked);
        }
    }
    m_settings->setEnabledProviderIds(ids);
}

void PreferencesDialog::browseSavingPath()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Choose lyrics saving path"),
        m_savingPathEdit->text());
    if (dir.isEmpty()) {
        return;
    }
    m_savingPathEdit->setText(dir);
    applySavingPath();
}

void PreferencesDialog::addFilterKeyword()
{
    const QString keyword = m_filterKeywordEdit->text().trimmed();
    if (keyword.isEmpty()) {
        return;
    }
    m_filterKeysList->addItem(keyword);
    m_filterKeywordEdit->clear();
    applyFilterKeys();
}

void PreferencesDialog::removeSelectedFilterKeywords()
{
    const auto selected = m_filterKeysList->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    for (QListWidgetItem *item : selected) {
        delete m_filterKeysList->takeItem(m_filterKeysList->row(item));
    }
    applyFilterKeys();
}

void PreferencesDialog::resetFilterKeywords()
{
    m_filterKeysList->clear();
    for (const QString &key : lyricsqt::LyricsFilter::defaultKeywords()) {
        m_filterKeysList->addItem(key);
    }
    applyFilterKeys();
}

void PreferencesDialog::syncColorButton(QPushButton *button, const QString &cssColor)
{
    if (!button) {
        return;
    }
    if (cssColor.compare(QLatin1String("none"), Qt::CaseInsensitive) == 0) {
        button->setText(QStringLiteral("None"));
        button->setStyleSheet(
            QStringLiteral("QPushButton { background-color: #f0f0f0; color: #333; border: 1px dashed #888; "
                           "border-radius: 4px; padding: 2px 6px; }"));
        return;
    }
    QColor color(cssColor);
    if (!color.isValid()) {
        color = QColor(Qt::gray);
    }
    const QString fg = color.lightness() > 140 ? QStringLiteral("#111111") : QStringLiteral("#ffffff");
    button->setText(cssColor);
    button->setStyleSheet(
        QStringLiteral("QPushButton { background-color: %1; color: %2; border: 1px solid #666; "
                       "border-radius: 4px; padding: 2px 6px; }")
            .arg(color.name(QColor::HexArgb), fg));
}

void PreferencesDialog::pickDesktopColor(QPushButton *button,
                                         const QString &title,
                                         const std::function<QString()> &getter,
                                         const std::function<void(const QString &)> &setter)
{
    if (m_loading || !button) {
        return;
    }
    QString current = getter();
    QColor initial(Qt::black);
    if (current.compare(QLatin1String("none"), Qt::CaseInsensitive) != 0) {
        initial = QColor(current);
    }
    if (!initial.isValid()) {
        initial = Qt::black;
    }
    const QColor chosen = QColorDialog::getColor(
        initial, this, title, QColorDialog::ShowAlphaChannel);
    if (!chosen.isValid()) {
        return;
    }
    // Fully transparent from the picker also means "no outline" for border color.
    if (chosen.alpha() == 0 && title.contains(QStringLiteral("Outline"), Qt::CaseInsensitive)) {
        setter(QStringLiteral("none"));
    } else {
        const QString css = chosen.alpha() == 255 ? chosen.name(QColor::HexRgb).toLower()
                                                  : chosen.name(QColor::HexArgb).toLower();
        setter(css);
    }
    syncColorButton(button, getter());
}

