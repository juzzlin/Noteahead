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

#ifndef EQ_CURVE_RENDERER_HPP
#define EQ_CURVE_RENDERER_HPP

#include <QColor>
#include <QPainterPath>
#include <QQuickPaintedItem>
#include <QRectF>
#include <QVariantList>

namespace noteahead {

//! Draws what the equalizer is doing, as one curve over the audible range.
//!
//! The dB values are handed in rather than worked out here: they come from the equalizer itself,
//! which reads them off the very filters it processes with. A drawing that recomputed the response
//! from the band settings would be a second implementation of the same maths, and the two would
//! drift the first time either changed.
class EqCurveRenderer : public QQuickPaintedItem
{
    Q_OBJECT

    //! The response in dB, evenly spaced by octave from 20 Hz to 20 kHz.
    Q_PROPERTY(QVariantList response READ response WRITE setResponse NOTIFY responseChanged)
    //! A second response drawn behind the first, thin and unfilled, or empty for none.
    //!
    //! For the case where one curve is not the whole truth: an equalizer working on one half of the
    //! stereo image shapes that half and wires the other straight through, and a single curve says
    //! nothing about which it is doing.
    Q_PROPERTY(QVariantList secondaryResponse READ secondaryResponse WRITE setSecondaryResponse NOTIFY secondaryResponseChanged)
    //! How far the curve is drawn either side of flat, in dB.
    Q_PROPERTY(int dbRange READ dbRange WRITE setDbRange NOTIFY dbRangeChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)

public:
    explicit EqCurveRenderer(QQuickItem * parent = nullptr);

    QVariantList response() const;
    void setResponse(const QVariantList & response);

    QVariantList secondaryResponse() const;
    void setSecondaryResponse(const QVariantList & response);

    int dbRange() const;
    void setDbRange(int dbRange);

    QColor accentColor() const;
    void setAccentColor(const QColor & color);

    void paint(QPainter * painter) override;

signals:
    void responseChanged();
    void secondaryResponseChanged();
    void dbRangeChanged();
    void accentColorChanged();

private:
    //! Where a frequency falls across the plot, 0 at 20 Hz and 1 at 20 kHz.
    static double frequencyPosition(double hz);

    //! @p response as a path across the plot, with the dB values clamped to the drawn range.
    QPainterPath curvePath(const QVariantList & response, const QRectF & plot) const;

    QVariantList m_response;
    QVariantList m_secondaryResponse;
    int m_dbRange = 18;
    QColor m_accentColor { 0, 180, 255 };
};

} // namespace noteahead

#endif // EQ_CURVE_RENDERER_HPP
