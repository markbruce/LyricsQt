#include <lyricsqt/ProviderHub.h>

#include <lyricsqt/QualityScorer.h>

#include <QDebug>

#include <algorithm>

namespace lyricsqt {

ProviderHub::ProviderHub(QObject *parent)
    : QObject(parent)
{
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, [this]() {
        if (m_finished) {
            return;
        }
        qDebug().noquote()
            << QStringLiteral("[ProviderHub] timeout after %1 ms candidates=%2")
                   .arg(m_timeoutMs)
                   .arg(m_candidates.size());
        emitBestAndFinish();
    });
}

void ProviderHub::addProvider(ILyricsProvider *provider)
{
    if (!provider) {
        return;
    }
    provider->setParent(this);
    m_providers.append(provider);
    connect(provider, &ILyricsProvider::resultsReady, this, &ProviderHub::onProviderResults);
    connect(provider, &ILyricsProvider::searchFailed, this, &ProviderHub::onProviderFailed);
}

void ProviderHub::setTimeoutMs(int ms)
{
    m_timeoutMs = qMax(0, ms);
}

void ProviderHub::setEnabledProviderIds(const QStringList &ids)
{
    m_enabledProviderIds = ids;
}

QVector<ILyricsProvider *> ProviderHub::activeProviders() const
{
    if (m_enabledProviderIds.isEmpty()) {
        return m_providers;
    }

    QVector<ILyricsProvider *> ordered;
    ordered.reserve(m_enabledProviderIds.size());
    for (const QString &id : m_enabledProviderIds) {
        for (ILyricsProvider *provider : m_providers) {
            if (provider && provider->id() == id) {
                ordered.append(provider);
                break;
            }
        }
    }
    return ordered;
}

void ProviderHub::cancel()
{
    m_timeout.stop();
    ++m_generation;
    m_finished = true;
    m_collecting = false;
    m_pendingProviders = 0;
    m_candidates.clear();
    for (ILyricsProvider *provider : m_providers) {
        provider->cancel();
    }
}

void ProviderHub::search(const TrackInfo &track)
{
    startSearch(track, false);
}

void ProviderHub::searchCollecting(const TrackInfo &track)
{
    startSearch(track, true);
}

void ProviderHub::startSearch(const TrackInfo &track, bool collecting)
{
    cancel();
    m_finished = false;
    m_collecting = collecting;
    m_track = track;
    m_candidates.clear();
    m_pendingProviders = 0;

    const QVector<ILyricsProvider *> providers = activeProviders();
    if (track.isEmpty() || providers.isEmpty()) {
        m_finished = true;
        m_collecting = false;
        if (collecting) {
            emit candidatesReady(track, {});
        }
        emit searchFinished(track, false);
        return;
    }

    const quint64 gen = m_generation;
    const LyricsSearchRequest req = LyricsSearchRequest::fromTrack(track, gen);

    for (ILyricsProvider *provider : providers) {
        ++m_pendingProviders;
        provider->search(req);
    }

    if (m_timeoutMs > 0) {
        m_timeout.start(m_timeoutMs);
    }
}

void ProviderHub::onProviderResults(const QString &providerId, quint64 generation,
                                    const QVector<LyricsDocument> &docs)
{
    if (generation != m_generation || m_finished) {
        return;
    }

    for (LyricsDocument doc : docs) {
        if (doc.sourceId.isEmpty()) {
            doc.sourceId = providerId;
        }
        doc.quality = QualityScorer::score(m_track, doc);
        m_candidates.append(doc);
        qDebug().noquote()
            << QStringLiteral("[ProviderHub] candidate source=%1 quality=%2 title=%3")
                   .arg(doc.sourceId)
                   .arg(doc.quality)
                   .arg(doc.title);
    }

    onProviderSettled();
}

void ProviderHub::onProviderFailed(const QString &providerId, quint64 generation,
                                   const QString &error)
{
    if (generation != m_generation || m_finished) {
        return;
    }
    qDebug().noquote()
        << QStringLiteral("[ProviderHub] provider failed id=%1 error=%2")
               .arg(providerId, error);
    onProviderSettled();
}

void ProviderHub::onProviderSettled()
{
    if (m_pendingProviders > 0) {
        --m_pendingProviders;
    }
    finishIfReady();
}

void ProviderHub::finishIfReady()
{
    if (m_finished) {
        return;
    }
    if (m_pendingProviders > 0) {
        return;
    }
    emitBestAndFinish();
}

void ProviderHub::emitBestAndFinish()
{
    if (m_finished) {
        return;
    }
    m_finished = true;
    m_timeout.stop();
    m_pendingProviders = 0;
    const bool collecting = m_collecting;
    m_collecting = false;

    // Cancel stragglers so late replies are ignored via generation bump... keep gen
    // so we don't invalidate the result we're about to emit; just abort network.
    for (ILyricsProvider *provider : m_providers) {
        provider->cancel();
    }

    if (collecting) {
        QVector<LyricsDocument> sorted = m_candidates;
        std::sort(sorted.begin(), sorted.end(), [](const LyricsDocument &a, const LyricsDocument &b) {
            return a.quality > b.quality;
        });
        emit candidatesReady(m_track, sorted);
        emit searchFinished(m_track, !sorted.isEmpty());
        return;
    }

    if (m_candidates.isEmpty()) {
        emit searchFinished(m_track, false);
        return;
    }

    LyricsDocument best = m_candidates.first();
    for (const LyricsDocument &doc : m_candidates) {
        if (doc.quality > best.quality) {
            best = doc;
        }
    }

    emit lyricsFound(m_track, best);
    emit searchFinished(m_track, true);
}

} // namespace lyricsqt
