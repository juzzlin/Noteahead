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

#ifndef OSCILLOSCOPE_RENDERER_HPP
#define OSCILLOSCOPE_RENDERER_HPP

#include <QQuickPaintedItem>
#include <QVariantList>

namespace noteahead {

//! Reusable single-channel oscilloscope trace. Fed a decimated waveform (floats in output
//! amplitude units) via the samples property; two of these side by side make a stereo scope.
class OscilloscopeRenderer : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(QVariantList samples READ samples WRITE setSamples NOTIFY samplesChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(qreal gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY showGridChanged)
    //! Largest absolute sample in the current trace, before gain. Read by the host to work out one
    //! vertical scale for both channels: scaling them separately would move the two traces by
    //! different amounts and the stereo picture would be a lie.
    Q_PROPERTY(qreal peak READ peak NOTIFY samplesChanged)

public:
    explicit OscilloscopeRenderer(QQuickItem * parent = nullptr);

    QVariantList samples() const;
    void setSamples(const QVariantList & samples);

    QColor accentColor() const;
    void setAccentColor(const QColor & color);

    qreal gain() const;
    void setGain(qreal gain);

    bool showGrid() const;
    void setShowGrid(bool show);

    qreal peak() const;

    void paint(QPainter * painter) override;

signals:
    void samplesChanged();
    void accentColorChanged();
    void gainChanged();
    void showGridChanged();

private:
    QVariantList m_samples;
    QColor m_accentColor { 0, 180, 255 };
    qreal m_gain = 1.0;
    bool m_showGrid = true;
    qreal m_peak = 0.0;
};

} // namespace noteahead

#endif // OSCILLOSCOPE_RENDERER_HPP
