#pragma once

#include <QString>
#include <QUrl>

namespace lyricsqt {

struct TrackInfo
{
    QString id;
    QString title;
    QString artist;
    QString album;
    qint64 lengthUs = 0;
    QUrl fileUrl;
    QUrl artUrl;

    bool isEmpty() const
    {
        return title.isEmpty() && artist.isEmpty() && id.isEmpty();
    }

    bool operator==(const TrackInfo &other) const
    {
        return id == other.id
            && title == other.title
            && artist == other.artist
            && album == other.album
            && lengthUs == other.lengthUs
            && fileUrl == other.fileUrl
            && artUrl == other.artUrl;
    }

    bool operator!=(const TrackInfo &other) const
    {
        return !(*this == other);
    }
};

} // namespace lyricsqt
