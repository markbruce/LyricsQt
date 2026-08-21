#include "KaraokeLyricLabel.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

KaraokeLyricLabel::KaraokeLyricLabel(QWidget *parent)
    : QWidget(parent)
{
    m_font = QFont(QStringLiteral("Sans Serif"), 28, QFont::Bold);
    setAttribute(Qt::WA_TranslucentBackground);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
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
    return m_textSize.expandedTo(QSize(40, 36));
}

QSize KaraokeLyricLabel::minimumSizeHint() const
{
    return sizeHint();
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
    const QRect textRect = rect().adjusted(2, 2, -2, -2);
    const int flags = Qt::AlignCenter | Qt::TextSingleLine;

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
            p.drawText(textRect.translated(dx, dy), flags, m_text);
        }
    }

    // Unplayed (blue) full text, then clip left→right with played (yellow).
    p.setPen(m_unplayed);
    p.drawText(textRect, flags, m_text);

    if (m_progress > 0.001) {
        const int splitX = textRect.left()
            + static_cast<int>(qRound(textRect.width() * m_progress));
        p.save();
        p.setClipRect(QRect(textRect.left(), textRect.top(),
                            qMax(0, splitX - textRect.left()), textRect.height()));
        p.setPen(m_played);
        p.drawText(textRect, flags, m_text);
        p.restore();
    }
}
