#pragma once

#include <QColor>
#include <QFont>
#include <QString>
#include <QWidget>

// QQ Music–style karaoke line: unplayed blue → played yellow L→R, with dark outline.
class KaraokeLyricLabel : public QWidget
{
    Q_OBJECT
public:
    explicit KaraokeLyricLabel(QWidget *parent = nullptr);

    void setText(const QString &text);
    QString text() const;

    // 0 = none played (all unplayedColor), 1 = fully played (all playedColor)
    void setProgress(qreal progress);

    void setUnplayedColor(const QColor &color);
    void setPlayedColor(const QColor &color);
    void setOutlineColor(const QColor &color);
    void setLyricFont(const QFont &font);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateMetrics();

    QString m_text;
    qreal m_progress = 0.0;
    QColor m_unplayed{0x3D, 0x9E, 0xFF}; // QQ-like blue
    QColor m_played{0xFF, 0xE0, 0x3D};   // QQ-like yellow
    QColor m_outline{0, 0, 0, 230};
    QFont m_font;
    QSize m_textSize;
};
