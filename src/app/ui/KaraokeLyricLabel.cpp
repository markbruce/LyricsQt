#include "KaraokeLyricLabel.h"

#include <QPainter>
#include <QPainterPath>
#include <QSizePolicy>
#include <QtMath>

KaraokeLyricLabel::KaraokeLyricLabel(QWidget *parent)
    : QWidget(parent)
{
    m_font = QFont(QStringLiteral("Sans Serif"), 28, QFont::Bold);
    setAttribute(Qt::WA_TranslucentBackground);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
}

void KaraokeLyricLabel::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    updateMetrics();
    update();
    updateGeometry();
}

QString KaraokeLyricLabel::text() const
{
    return m_text;
}

void KaraokeLyricLabel::setProgress(qreal progress)
{
    progress = qBound(0.0, progress, 1.0);
    if (qFuzzyCompare(m_progress + 1.0, progress + 1.0)) {
        return;
    }
    m_progress = progress;
    update();
}

void KaraokeLyricLabel::setUnplayedColor(const QColor &color)
{
    m_unplayed = color;
    update();
}

void KaraokeLyricLabel::setPlayedColor(const QColor &color)
{
    m_played = color;
    update();
}

void KaraokeLyricLabel::setOutlineColor(const QColor &color)
{
    m_outline = color;
    update();
}

void KaraokeLyricLabel::setLyricFont(const QFont &font)
{
    m_font = font;
    updateMetrics();
    update();
    updateGeometry();
}

QSize KaraokeLyricLabel::sizeHint() const
{
    return QSize(0, qMax(36, m_textSize.height()));
}

QSize KaraokeLyricLabel::minimumSizeHint() const
{
    return QSize(0, qMax(28, m_textSize.height()));
}

void KaraokeLyricLabel::updateMetrics()
{
    QFontMetrics fm(m_font);
    // Padding for outline stroke.
    const int pad = qMax(4, fm.height() / 10);
    m_textSize = QSize(fm.horizontalAdvance(m_text) + pad * 2, fm.height() + pad * 2);
}

void KaraokeLyricLabel::paintEvent(QPaintEvent *)
{
    if (m_text.isEmpty()) {
        return;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.setFont(m_font);

    const QFontMetrics fm(m_font);
    const QRect area = rect().adjusted(2, 2, -2, -2);
    const int flags = Qt::AlignCenter | Qt::TextSingleLine;

    // Progress must follow the centered glyphs, not the full panel width —
    // otherwise the wipe spends time on empty left padding and feels delayed.
    const int textW = fm.horizontalAdvance(m_text);
    const int textH = fm.height();
    QRect glyphRect(
        area.x() + (area.width() - textW) / 2,
        area.y() + (area.height() - textH) / 2,
        qMax(1, textW),
        qMax(1, textH));
    glyphRect = glyphRect.intersected(area);
    if (glyphRect.width() <= 0) {
        glyphRect = area;
    }

    // Dark outline / stroke for readability (QQ Music style).
    const int outline = qMax(2, fm.height() / 14);
    p.setPen(m_outline);
    for (int dx = -outline; dx <= outline; ++dx) {
        for (int dy = -outline; dy <= outline; ++dy) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            if (dx * dx + dy * dy > outline * outline + 1) {
                continue;
            }
            p.drawText(area.translated(dx, dy), flags, m_text);
        }
    }

    // Unplayed (blue) full text, then clip left→right over the glyph box only.
    p.setPen(m_unplayed);
    p.drawText(area, flags, m_text);

    if (m_progress > 0.001) {
        const int playedW = static_cast<int>(qRound(glyphRect.width() * m_progress));
        p.save();
        p.setClipRect(QRect(glyphRect.left(), area.top(),
                            qMax(0, playedW), area.height()));
        p.setPen(m_played);
        p.drawText(area, flags, m_text);
        p.restore();
    }
}
