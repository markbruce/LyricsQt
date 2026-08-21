#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

namespace lyricsqt {

class AppSettings : public QObject
{
    Q_OBJECT
public:
    explicit AppSettings(QObject *parent = nullptr);
    explicit AppSettings(const QString &organization, const QString &application, QObject *parent = nullptr);

    bool desktopLyricsEnabled() const;
    void setDesktopLyricsEnabled(bool enabled);

    bool menuBarLyricsEnabled() const;
    void setMenuBarLyricsEnabled(bool enabled);

    bool exportEnabled() const;
    void setExportEnabled(bool enabled);

    QString preferredPlayerId() const;
    void setPreferredPlayerId(const QString &id);

    int globalOffsetMs() const;
    void setGlobalOffsetMs(int offsetMs);

signals:
    void changed(const QString &key);

private:
    QSettings m_settings;
};

} // namespace lyricsqt
