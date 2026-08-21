#pragma once

#include <lyricsqt/LyricsDocument.h>
#include <lyricsqt/LrcParser.h>

#include <QByteArray>
#include <QRegularExpression>
#include <QString>
#include <QtMath>

namespace lyricsqt {
namespace provider_helpers {

inline QString stripJsonp(QByteArray data)
{
    data = data.trimmed();
    const int open = data.indexOf('(');
    const int close = data.lastIndexOf(')');
    if (open >= 0 && close > open) {
        return QString::fromUtf8(data.mid(open + 1, close - open - 1));
    }
    return QString::fromUtf8(data);
}

inline QString decodeBase64Utf8(const QString &b64)
{
    const QByteArray decoded = QByteArray::fromBase64(b64.toUtf8());
    if (decoded.isEmpty() && !b64.isEmpty()) {
        return {};
    }
    return QString::fromUtf8(decoded);
}

/// NetEase sometimes uses [mm:ss:xx] instead of [mm:ss.xx].
inline QString fixNetEaseTimeTags(QString text)
{
    static const QRegularExpression re(QStringLiteral(R"((\[\d+:\d+):(\d+\])"));
    return text.replace(re, QStringLiteral("\\1.\\2"));
}

inline void mergeTranslation(LyricsDocument &doc, const LyricsDocument &trans)
{
    if (trans.lines.isEmpty()) {
        return;
    }
    for (LyricsLine &line : doc.lines) {
        if (!line.translation.isEmpty()) {
            continue;
        }
        double bestDelta = 1e9;
        QString best;
        for (const LyricsLine &t : trans.lines) {
            const double delta = qAbs(t.positionSec - line.positionSec);
            if (delta < bestDelta) {
                bestDelta = delta;
                best = t.content;
            }
        }
        if (bestDelta <= 0.8 && !best.isEmpty()) {
            line.translation = best;
        }
    }
}

inline LyricsDocument parseLrc(const QString &text, const QString &sourceId)
{
    LyricsDocument doc = LrcParser::parse(text);
    doc.sourceId = sourceId;
    return doc;
}

} // namespace provider_helpers
} // namespace lyricsqt
