#include <lyricsqt/LyricsStore.h>

#include <lyricsqt/AppSettings.h>
#include <lyricsqt/LrcParser.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace lyricsqt {

namespace {

QString sanitizePathComponent(QString value)
{
    return value.replace(QLatin1Char('/'), QLatin1Char(':'));
}

QString formatLrcTimestamp(double positionSec)
{
    if (positionSec < 0.0) {
        positionSec = 0.0;
    }
    const int totalCs = static_cast<int>(positionSec * 100.0 + 0.5);
    const int minutes = totalCs / 6000;
    const int rem = totalCs % 6000;
    const int seconds = rem / 100;
    const int centiseconds = rem % 100;
    return QStringLiteral("%1:%2.%3")
        .arg(minutes, 2, 10, QChar(QLatin1Char('0')))
        .arg(seconds, 2, 10, QChar(QLatin1Char('0')))
        .arg(centiseconds, 2, 10, QChar(QLatin1Char('0')));
}

} // namespace

LyricsStore::LyricsStore(AppSettings *settings)
    : m_settings(settings)
{
}

QString LyricsStore::cacheFileName(const TrackInfo &track) const
{
    return QStringLiteral("%1 - %2.lrcx")
        .arg(sanitizePathComponent(track.title), sanitizePathComponent(track.artist));
}

std::optional<LyricsDocument> LyricsStore::tryLoadFile(const QString &path) const
{
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return std::nullopt;
    }
    LyricsDocument doc = LrcParser::parseFile(path);
    if (doc.lines.isEmpty()) {
        qDebug().noquote() << QStringLiteral("[LyricsStore] empty or unreadable: %1").arg(path);
        return std::nullopt;
    }
    qDebug().noquote()
        << QStringLiteral("[LyricsStore] loaded %1 lines from %2").arg(doc.lines.size()).arg(path);
    return doc;
}

std::optional<LyricsDocument> LyricsStore::loadLocal(const TrackInfo &track)
{
    if (!m_settings || track.isEmpty()) {
        return std::nullopt;
    }

    QStringList candidates;

    if (m_settings->loadLyricsBesideTrack() && track.fileUrl.isLocalFile()) {
        const QFileInfo media(track.fileUrl.toLocalFile());
        if (media.exists()) {
            const QString base = media.absolutePath() + QLatin1Char('/') + media.completeBaseName();
            candidates << (base + QStringLiteral(".lrcx"))
                       << (base + QStringLiteral(".lrc"));
        }
    }

    const QString saveDir = m_settings->lyricsSavingPath();
    const QString cacheBase = saveDir + QLatin1Char('/')
        + sanitizePathComponent(track.title) + QStringLiteral(" - ")
        + sanitizePathComponent(track.artist);
    candidates << (cacheBase + QStringLiteral(".lrcx"))
               << (cacheBase + QStringLiteral(".lrc"));

    for (const QString &path : candidates) {
        if (auto doc = tryLoadFile(path)) {
            return doc;
        }
    }

    qDebug().noquote()
        << QStringLiteral("[LyricsStore] no local lyrics for title=%1 artist=%2")
               .arg(track.title, track.artist);
    return std::nullopt;
}

QString LyricsStore::serializeLrc(const LyricsDocument &doc) const
{
    QString out;
    QTextStream stream(&out);
    if (!doc.title.isEmpty()) {
        stream << QStringLiteral("[ti:%1]\n").arg(doc.title);
    }
    if (!doc.artist.isEmpty()) {
        stream << QStringLiteral("[ar:%1]\n").arg(doc.artist);
    }
    if (!doc.album.isEmpty()) {
        stream << QStringLiteral("[al:%1]\n").arg(doc.album);
    }
    if (doc.offsetMs != 0) {
        stream << QStringLiteral("[offset:%1]\n").arg(doc.offsetMs);
    }
    for (const LyricsLine &line : doc.lines) {
        QString content = line.content;
        if (!line.translation.isEmpty()) {
            content += QLatin1Char('|') + line.translation;
        }
        stream << QStringLiteral("[%1]%2\n")
                      .arg(formatLrcTimestamp(line.positionSec), content);
    }
    stream.flush();
    return out;
}

QUrl LyricsStore::save(const TrackInfo &track, const LyricsDocument &doc)
{
    if (!m_settings) {
        return {};
    }

    const QString dirPath = m_settings->lyricsSavingPath();
    QDir dir(dirPath);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        qDebug().noquote() << QStringLiteral("[LyricsStore] failed to create dir %1").arg(dirPath);
        return {};
    }

    LyricsDocument toWrite = doc;
    if (toWrite.title.isEmpty()) {
        toWrite.title = track.title;
    }
    if (toWrite.artist.isEmpty()) {
        toWrite.artist = track.artist;
    }
    if (toWrite.album.isEmpty()) {
        toWrite.album = track.album;
    }

    const QString fileName = cacheFileName(track);
    const QString path = dir.filePath(fileName);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qDebug().noquote() << QStringLiteral("[LyricsStore] failed to write %1").arg(path);
        return {};
    }
    QTextStream out(&file);
    out << serializeLrc(toWrite);
    file.close();

    toWrite.localPath = path;
    qDebug().noquote() << QStringLiteral("[LyricsStore] saved %1").arg(path);
    return QUrl::fromLocalFile(path);
}

bool LyricsStore::removeLocal(const TrackInfo &track)
{
    if (!m_settings || track.isEmpty()) {
        return false;
    }

    const QString saveDir = m_settings->lyricsSavingPath();
    const QString cacheBase = saveDir + QLatin1Char('/')
        + sanitizePathComponent(track.title) + QStringLiteral(" - ")
        + sanitizePathComponent(track.artist);

    bool removedAny = false;
    for (const QString &ext : {QStringLiteral(".lrcx"), QStringLiteral(".lrc")}) {
        const QString path = cacheBase + ext;
        if (!QFileInfo::exists(path)) {
            continue;
        }
        if (QFile::remove(path)) {
            removedAny = true;
            qDebug().noquote() << QStringLiteral("[LyricsStore] removed %1").arg(path);
        } else {
            qDebug().noquote() << QStringLiteral("[LyricsStore] failed to remove %1").arg(path);
        }
    }
    return removedAny;
}

} // namespace lyricsqt
