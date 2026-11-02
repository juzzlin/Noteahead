// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#ifndef WAVE_SHAPE_RENDERER_HPP
#define WAVE_SHAPE_RENDERER_HPP

#include <QColor>
#include <QList>
#include <QQuickPaintedItem>

namespace noteahead {

//! Draws a few cycles of a waveform, edge to edge around a centre line: a preview one line of text
//! tall, such as an oscillator's beside its heading.
class WaveShapeRenderer : public QQuickPaintedItem
{
    Q_OBJECT

    //! The waveform from left to right, -1 at the bottom and 1 at the top.
    Q_PROPERTY(QList<qreal> samples READ samples WRITE setSamples NOTIFY samplesChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    //! Drawn faintly, for a source that is shaped but not heard.
    Q_PROPERTY(bool dimmed READ dimmed WRITE setDimmed NOTIFY dimmedChanged)

public:
    explicit WaveShapeRenderer(QQuickItem * parent = nullptr);

    QList<qreal> samples() const;
    void setSamples(const QList<qreal> & samples);

    QColor accentColor() const;
    void setAccentColor(const QColor & color);

    bool dimmed() const;
    void setDimmed(bool dimmed);

    void paint(QPainter * painter) override;

signals:
    void samplesChanged();
    void accentColorChanged();
    void dimmedChanged();

private:
    QList<qreal> m_samples;
    QColor m_accentColor { "#4fc3f7" };
    bool m_dimmed { false };
};

} // namespace noteahead

#endif // WAVE_SHAPE_RENDERER_HPP
