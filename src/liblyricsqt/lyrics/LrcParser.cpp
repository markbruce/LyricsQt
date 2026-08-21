#include <lyricsqt/LrcParser.h>

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include <algorithm>

namespace lyricsqt {

namespace {

double parseTimestamp(const QString &token)
{
    // mm:ss.xx or mm:ss.xxx or mm:ss
    const QStringList parts = token.split(QLatin1Char(':'));
    if (parts.size() != 2) {
        return -1.0;
    }
    bool okMin = false;
    bool okSec = false;
    const int minutes = parts[0].toInt(&okMin);
    const double seconds = parts[1].toDouble(&okSec);
    if (!okMin || !okSec || minutes < 0 || seconds < 0.0) {
        return -1.0;
    }
    return minutes * 60.0 + seconds;
}

} // namespace

LyricsDocument LrcParser::parse(const QString &text)
{
    LyricsDocument doc;
    QRegularExpression metaRe(QStringLiteral(R"(^\[(ti|ar|al|offset|by|re|ve|length):(.*)\]\s*$)"));
    metaRe.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression timeRe(QStringLiteral(R"(\[(\d{1,3}:\d{1,2}(?:\.\d{1,3})?)\])"));

    const QStringList rawLines = text.split(QRegularExpression(QStringLiteral(R"(\r\n|\n|\r)")));
    for (QString line : rawLines) {
        line = line.trimmed();
        if (line.isEmpty()) {
            continue;
        }

        const auto metaMatch = metaRe.match(line);
        if (metaMatch.hasMatch()) {
            const QString key = metaMatch.captured(1).toLower();
            const QString value = metaMatch.captured(2).trimmed();
            if (key == QLatin1String("ti")) {
                doc.title = value;
            } else if (key == QLatin1String("ar")) {
                doc.artist = value;
            } else if (key == QLatin1String("al")) {
                doc.album = value;
            } else if (key == QLatin1String("offset")) {
                bool ok = false;
                const int offset = value.toInt(&ok);
                if (ok) {
                    doc.offsetMs = offset;
                }
            }
            continue;
        }

        QVector<double> times;
        qsizetype last = 0;
        auto it = timeRe.globalMatch(line);
        while (it.hasNext()) {
            const auto m = it.next();
            if (m.capturedStart() != last) {
                // Non-leading junk before first tag — treat whole line as non-timed if no times yet
                if (times.isEmpty() && m.capturedStart() > 0) {
                    break;
                }
            }
            const double t = parseTimestamp(m.captured(1));
            if (t >= 0.0) {
                times.push_back(t);
            }
            last = m.capturedEnd();
        }

        if (times.isEmpty()) {
            continue;
        }

        QString content = line.mid(last).trimmed();
        // LRCX simple translation: content | translation
        QString translation;
        const int bar = content.indexOf(QLatin1Char('|'));
        if (bar >= 0) {
            translation = content.mid(bar + 1).trimmed();
            content = content.left(bar).trimmed();
        }

        for (double t : times) {
            LyricsLine l;
            l.positionSec = t;
            l.content = content;
            l.translation = translation;
            doc.lines.push_back(l);
        }
    }

    std::sort(doc.lines.begin(), doc.lines.end(), [](const LyricsLine &a, const LyricsLine &b) {
        return a.positionSec < b.positionSec;
    });
    return doc;
}

LyricsDocument LrcParser::parseFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    QTextStream in(&file);
    const QString text = in.readAll();
    LyricsDocument doc = parse(text);
    doc.localPath = path;
    return doc;
}

} // namespace lyricsqt
