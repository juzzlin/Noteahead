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

#ifndef SPECTRUM_COMPARE_RENDERER_HPP
#define SPECTRUM_COMPARE_RENDERER_HPP

#include <QColor>
#include <QQuickPaintedItem>

#include <utility>
#include <vector>

namespace noteahead {

//! Draws two files' third-octave balances and the difference between them.
//!
//! The two curves say what each mix is; the bars between them say what to do about it. Both sides
//! are measured against their own midrange, so their difference is a balance difference whatever
//! either was mastered to, and a bar reads directly as an equalizer move.
class SpectrumCompareRenderer : public QQuickPaintedItem
{
    Q_OBJECT

    //! How far the drawing reaches either side of the centre line, in dB.
    Q_PROPERTY(int dbRange READ dbRange WRITE setDbRange NOTIFY dbRangeChanged)
    Q_PROPERTY(QColor leftColor READ leftColor WRITE setLeftColor NOTIFY leftColorChanged)
    Q_PROPERTY(QColor rightColor READ rightColor WRITE setRightColor NOTIFY rightColorChanged)

public:
    //! One file's bands, as centre frequency and level relative to that file's own midrange.
    using Bands = std::vector<std::pair<double, float>>;

    explicit SpectrumCompareRenderer(QQuickItem * parent = nullptr);

    //! What to draw. Either side may be empty, which draws that curve and the difference not at all.
    //!
    //! Taken as vectors rather than through QML lists because the controller has them as vectors
    //! already, and a band count in the dozens is not worth boxing into QVariants.
    void setBands(const Bands & left, const Bands & right);

    void setNames(const QString & left, const QString & right);

    int dbRange() const;
    void setDbRange(int dbRange);

    QColor leftColor() const;
    void setLeftColor(const QColor & color);

    QColor rightColor() const;
    void setRightColor(const QColor & color);

    void paint(QPainter * painter) override;

signals:
    void dbRangeChanged();
    void leftColorChanged();
    void rightColorChanged();

private:
    //! Where @p frequency sits across the width, as a fraction: logarithmic, as an octave is the
    //! same distance wherever it is.
    static double position(double frequency);

    void paintGrid(QPainter * painter);
    void paintCurve(QPainter * painter, const Bands & bands, const QColor & color);
    void paintDifference(QPainter * painter);
    void paintLegend(QPainter * painter);

    Bands m_left;
    Bands m_right;
    QString m_leftName;
    QString m_rightName;

    int m_dbRange { 18 };
    QColor m_leftColor { "#4CAF50" };
    QColor m_rightColor { "#2196F3" };
};

} // namespace noteahead

#endif // SPECTRUM_COMPARE_RENDERER_HPP
