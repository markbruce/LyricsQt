#pragma once

#include <lyricsqt/LyricsDocument.h>
#include <lyricsqt/TrackInfo.h>

#include <QObject>
#include <QString>
#include <QVector>

namespace lyricsqt {

struct LyricsSearchRequest
{
    QString title;
    QString artist;
    QString album;
    double durationSec = 0.0;
    /// Opaque generation token; providers should ignore stale replies.
    quint64 generation = 0;

    static LyricsSearchRequest fromTrack(const TrackInfo &track, quint64 generation = 0)
    {
        LyricsSearchRequest req;
        req.title = track.title;
        req.artist = track.artist;
        req.album = track.album;
        if (track.lengthUs > 0) {
            req.durationSec = static_cast<double>(track.lengthUs) / 1'000'000.0;
        }
        req.generation = generation;
        return req;
    }
};

class ILyricsProvider : public QObject
{
    Q_OBJECT
public:
    explicit ILyricsProvider(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~ILyricsProvider() override = default;

    virtual QString id() const = 0;
    virtual void search(const LyricsSearchRequest &req) = 0;
    virtual void cancel() = 0;

signals:
    void resultsReady(const QString &providerId, quint64 generation, const QVector<LyricsDocument> &docs);
    void searchFailed(const QString &providerId, quint64 generation, const QString &error);
};

} // namespace lyricsqt
