#include <lyricsqt/LyricsFilter.h>

#include <lyricsqt/AppSettings.h>

#include <QRegularExpression>

namespace lyricsqt {

namespace {

bool hasLetterOrNumber(const QString &content)
{
    for (const QChar ch : content) {
        if (ch.isLetterOrNumber()) {
            return true;
        }
    }
    return false;
}

const QRegularExpression &metadataRegex()
{
    static const QRegularExpression re(
        QStringLiteral(
            R"((?i)^\s*(by|title|song|album|artist|singer|lyrics|作词|作詞|作曲|编曲|編曲|演唱|歌手)\s*[:：∶])"),
        QRegularExpression::UseUnicodePropertiesOption);
    return re;
}

const QRegularExpression &domainLikeRegex()
{
    static const QRegularExpression re(QStringLiteral(R"(\w+(\.\w+){2})"));
    return re;
}

} // namespace

QStringList LyricsFilter::defaultKeywords()
{
    return {
        QStringLiteral("/(by|title|song|album|artist|singer|lyrics)\\s*[:：∶]"),
        QStringLiteral("/\\w+(\\.\\w+){2}"),
        QStringLiteral("/^\\s*$"),
        QStringLiteral("/\\d{8}"),
        QStringLiteral("/^\\.$"),
        QStringLiteral("作詞"),
        QStringLiteral("作词"),
        QStringLiteral("作曲"),
        QStringLiteral("編曲"),
        QStringLiteral("编曲"),
        QStringLiteral("収録"),
        QStringLiteral("收录"),
        QStringLiteral("演唱"),
        QStringLiteral("歌手"),
        QStringLiteral("歌曲"),
        QStringLiteral("制作"),
        QStringLiteral("製作"),
        QStringLiteral("歌词"),
        QStringLiteral("歌詞"),
        QStringLiteral("翻譯"),
        QStringLiteral("翻译"),
        QStringLiteral("插曲"),
        QStringLiteral("插入歌"),
        QStringLiteral("主题歌"),
        QStringLiteral("主題歌"),
        QStringLiteral("片頭曲"),
        QStringLiteral("片头曲"),
        QStringLiteral("片尾曲"),
        QStringLiteral("SoundTrack"),
        QStringLiteral("アニメ"),
    };
}

bool LyricsFilter::matchesKeyword(const QString &content, const QString &key)
{
    if (key.isEmpty()) {
        return false;
    }

    if (key.startsWith(QLatin1Char('/'))) {
        const QString pattern = key.mid(1);
        if (pattern.isEmpty()) {
            return false;
        }
        const QRegularExpression re(pattern, QRegularExpression::UseUnicodePropertiesOption);
        if (!re.isValid()) {
            return false;
        }
        return re.match(content).hasMatch();
    }

    return content.contains(key);
}

bool LyricsFilter::isEmptyOrWhitespace(const QString &content)
{
    return content.trimmed().isEmpty();
}

bool LyricsFilter::isPunctuationOnly(const QString &content)
{
    const QString trimmed = content.trimmed();
    if (trimmed.isEmpty()) {
        return false; // handled by empty check
    }
    return !hasLetterOrNumber(trimmed);
}

bool LyricsFilter::isMetadataLike(const QString &content)
{
    const QString trimmed = content.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    if (trimmed == QLatin1String(".")) {
        return true;
    }
    if (metadataRegex().match(trimmed).hasMatch()) {
        return true;
    }
    if (domainLikeRegex().match(trimmed).hasMatch()) {
        return true;
    }
    return false;
}

LyricsFilterOptions LyricsFilter::optionsFromSettings(const AppSettings *settings)
{
    LyricsFilterOptions opts;
    if (!settings) {
        opts.keywordEnabled = true;
        opts.smartEnabled = true;
        opts.keywords = defaultKeywords();
        return opts;
    }
    opts.keywordEnabled = settings->lyricsFilterEnabled();
    opts.smartEnabled = settings->lyricsSmartFilterEnabled();
    opts.keywords = settings->lyricsFilterKeys();
    return opts;
}

LyricsDocument LyricsFilter::apply(const LyricsDocument &doc, const LyricsFilterOptions &options)
{
    LyricsDocument out = doc;
    out.lines.clear();
    out.lines.reserve(doc.lines.size());

    for (const LyricsLine &line : doc.lines) {
        bool drop = false;

        if (options.smartEnabled) {
            if (isEmptyOrWhitespace(line.content)
                || isPunctuationOnly(line.content)
                || isMetadataLike(line.content)) {
                drop = true;
            }
        }

        if (!drop && options.keywordEnabled) {
            for (const QString &key : options.keywords) {
                if (matchesKeyword(line.content, key)) {
                    drop = true;
                    break;
                }
            }
        }

        if (!drop) {
            out.lines.append(line);
        }
    }

    return out;
}

LyricsDocument LyricsFilter::apply(const LyricsDocument &doc, const AppSettings *settings)
{
    return apply(doc, optionsFromSettings(settings));
}

} // namespace lyricsqt
