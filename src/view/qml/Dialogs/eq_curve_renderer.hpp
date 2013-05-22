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

#include <cstdint>
#include <utility>
#include <vector>

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
    //! What of the live spectrum is drawn behind the curve, one of SpectrumMode.
    Q_PROPERTY(int spectrumMode READ spectrumMode WRITE setSpectrumMode NOTIFY spectrumModeChanged)
    //! How many dB below full scale the spectrum reaches at the bottom of the plot.
    Q_PROPERTY(int spectrumRange READ spectrumRange WRITE setSpectrumRange NOTIFY spectrumRangeChanged)
    //! The output's spectrum, filled, and the input's, outlined and in the difference modes.
    Q_PROPERTY(QColor spectrumPostColor READ spectrumPostColor WRITE setSpectrumPostColor NOTIFY spectrumColorsChanged)
    Q_PROPERTY(QColor spectrumPreColor READ spectrumPreColor WRITE setSpectrumPreColor NOTIFY spectrumColorsChanged)

public:
    //! Appended to only: the dialogs list the modes in this order.
    enum SpectrumMode
    {
        SpectrumOff,
        //! What goes into the equalizer.
        SpectrumPre,
        //! What comes out of it.
        SpectrumPost,
        //! Both, the output filled and the input outlined over it.
        SpectrumBoth,
        //! What the equalizer took away, on the curve's own scale.
        SpectrumPreMinusPost,
        //! What the equalizer did to the signal, on the curve's own scale. Where it leaves the curve,
        //! something the curve cannot show is at work.
        SpectrumPostMinusPre
    };
    Q_ENUM(SpectrumMode)

    explicit EqCurveRenderer(QQuickItem * parent = nullptr);

    QVariantList response() const;
    void setResponse(const QVariantList & response);

    QVariantList secondaryResponse() const;
    void setSecondaryResponse(const QVariantList & response);

    int dbRange() const;
    void setDbRange(int dbRange);

    QColor accentColor() const;
    void setAccentColor(const QColor & color);

    int spectrumMode() const;
    void setSpectrumMode(int mode);

    int spectrumRange() const;
    void setSpectrumRange(int range);

    QColor spectrumPostColor() const;
    void setSpectrumPostColor(const QColor & color);
    QColor spectrumPreColor() const;
    void setSpectrumPreColor(const QColor & color);

    //! Whether the mode needs the input's spectrum, and the output's.
    bool spectrumUsesPre() const;
    bool spectrumUsesPost() const;

    //! The levels of the input and the output, in dBFS, one per band, or empty for a tap not shown.
    //!
    //! Taken as vectors straight from the analyzer rather than through a QML list, for the same
    //! reason RtaRenderer takes them that way: this is called thirty times a second.
    void setSpectrumLevels(std::vector<float> pre, std::vector<float> post);

    //! Where the bands sit, as normalised log-frequency spans, and which layout they came from.
    void setSpectrumLayout(const std::vector<std::pair<float, float>> & positions, uint32_t generation);
    uint32_t spectrumLayoutGeneration() const;

    void paint(QPainter * painter) override;

signals:
    void responseChanged();
    void secondaryResponseChanged();
    void dbRangeChanged();
    void accentColorChanged();
    void spectrumModeChanged();
    void spectrumRangeChanged();
    void spectrumColorsChanged();

private:
    //! Where a frequency falls across the plot, 0 at 20 Hz and 1 at 20 kHz.
    static double frequencyPosition(double hz);

    //! @p response as a path across the plot, with the dB values clamped to the drawn range.
    QPainterPath curvePath(const QVariantList & response, const QRectF & plot) const;

    //! @p levels across the plot on the spectrum's own dBFS scale, at the band centres.
    QPainterPath spectrumPath(const std::vector<float> & levels, const QRectF & plot) const;
    void paintSpectrum(QPainter & painter, const QRectF & plot) const;
    //! Post minus Pre, or Pre minus Post when @p inverted, on the curve's scale. Bands whose
    //! reference is too quiet to compare are left as gaps rather than drawn as noise.
    void paintSpectrumDifference(QPainter & painter, const QRectF & plot, bool inverted) const;
    qreal bandCentreX(size_t band, const QRectF & plot) const;

    QVariantList m_response;
    QVariantList m_secondaryResponse;
    int m_dbRange = 18;
    QColor m_accentColor { 0, 180, 255 };

    int m_spectrumMode = SpectrumOff;
    int m_spectrumRange = 90;
    QColor m_spectrumPostColor { 250, 220, 60 };
    QColor m_spectrumPreColor { 250, 140, 60 };
    std::vector<float> m_preLevels;
    std::vector<float> m_postLevels;
    std::vector<std::pair<float, float>> m_spectrumPositions;
    uint32_t m_spectrumLayoutGeneration = 0;
};

} // namespace noteahead

#endif // EQ_CURVE_RENDERER_HPP
