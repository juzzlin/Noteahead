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

#ifndef WAVEFORM_RENDERER_HPP
#define WAVEFORM_RENDERER_HPP

#include <QColor>
#include <QPointF>
#include <QQuickPaintedItem>
#include <QVariantList>

#include <vector>

namespace noteahead {

//! Draws a sound as one peak bar per pixel, the trimmed-off ends shaded, and the amplitude envelope
//! it is played through laid over it, mirrored about the centre line.
//!
//! The peaks are handed in already reduced: whoever has the audio knows how to read it, and this
//! only knows how to draw.
class WaveformRenderer : public QQuickPaintedItem
{
    Q_OBJECT

    //! Peak levels from left to right. See normalized for the scale.
    Q_PROPERTY(QVariantList waveformData READ waveformData WRITE setWaveformData NOTIFY waveformDataChanged)
    //! Whether the loudest peak is drawn at full height, which suits a sound whose level is not the
    //! point. Off, the peaks are drawn on a fixed 0..1 scale, so that what turns a sound down, such
    //! as a filter, shows as a smaller picture rather than being scaled back up.
    Q_PROPERTY(bool normalized READ normalized WRITE setNormalized NOTIFY normalizedChanged)
    //! Where playback starts and stops, as fractions of the file. What lies outside is shaded.
    Q_PROPERTY(double startOffset READ startOffset WRITE setStartOffset NOTIFY startOffsetChanged)
    Q_PROPERTY(double endOffset READ endOffset WRITE setEndOffset NOTIFY endOffsetChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)

    Q_PROPERTY(bool showEnvelope READ showEnvelope WRITE setShowEnvelope NOTIFY envelopeChanged)
    //! What the picture spans, in seconds: what puts the envelope's seconds on the waveform's axis.
    Q_PROPERTY(double duration READ duration WRITE setDuration NOTIFY envelopeChanged)
    Q_PROPERTY(double envelopeAttack READ envelopeAttack WRITE setEnvelopeAttack NOTIFY envelopeChanged)
    Q_PROPERTY(double envelopeHold READ envelopeHold WRITE setEnvelopeHold NOTIFY envelopeChanged)
    Q_PROPERTY(double envelopeDecay READ envelopeDecay WRITE setEnvelopeDecay NOTIFY envelopeChanged)
    Q_PROPERTY(double envelopeSustain READ envelopeSustain WRITE setEnvelopeSustain NOTIFY envelopeChanged)
    Q_PROPERTY(double envelopeRelease READ envelopeRelease WRITE setEnvelopeRelease NOTIFY envelopeChanged)
    //! Bend of every segment, 0..1.
    Q_PROPERTY(double envelopeCurve READ envelopeCurve WRITE setEnvelopeCurve NOTIFY envelopeChanged)
    //! How far a full curve bends, which differs between envelopes: the drum pads' DrumAmpEnvelope,
    //! the default, and the Sampler's AdsrEnvelope do not bend alike.
    Q_PROPERTY(double envelopeMaxCurvature READ envelopeMaxCurvature WRITE setEnvelopeMaxCurvature NOTIFY envelopeChanged)

public:
    //! An envelope's times in seconds, its sustain level 0..1 and its bend.
    struct Envelope
    {
        double attack = 0.0;
        double hold = 0.0;
        double decay = 0.0;
        double sustain = 1.0;
        double release = 0.0;
        double curve = 0.0;
        //! Zero draws straight segments whatever the curve.
        double maxCurvature = 0.0;
    };

    explicit WaveformRenderer(QQuickItem * parent = nullptr);

    QVariantList waveformData() const;
    void setWaveformData(const QVariantList & data);

    bool normalized() const;
    void setNormalized(bool normalized);

    double startOffset() const;
    void setStartOffset(double offset);

    double endOffset() const;
    void setEndOffset(double offset);

    QColor accentColor() const;
    void setAccentColor(const QColor & color);

    bool showEnvelope() const;
    void setShowEnvelope(bool show);

    double duration() const;
    void setDuration(double seconds);

    double envelopeAttack() const;
    void setEnvelopeAttack(double seconds);

    double envelopeHold() const;
    void setEnvelopeHold(double seconds);

    double envelopeDecay() const;
    void setEnvelopeDecay(double seconds);

    double envelopeSustain() const;
    void setEnvelopeSustain(double level);

    double envelopeRelease() const;
    void setEnvelopeRelease(double seconds);

    double envelopeCurve() const;
    void setEnvelopeCurve(double curve);

    double envelopeMaxCurvature() const;
    void setEnvelopeMaxCurvature(double curvature);

    void paint(QPainter * painter) override;

    //! The bend an envelope applies across a segment, 0..1 in and out. Matches AdsrEnvelope::shape()
    //! and DrumAmpEnvelope::shape(), so the picture is the shape the sound is played through.
    static double shape(double phase, double curve, double maxCurvature);

    //! @p envelope as points across a range from @p x0 to @p x1, x in pixels and y the level 0..1.
    //!
    //! Attack, hold and decay run from where the range starts, the sustain holds to where it ends and
    //! the release falls from there, which is where a sound held to the end of its range would be let
    //! go. A segment that would run past the end of the range is cut there, at the level it had
    //! reached, so a long attack on a short sound shows how little of the envelope is ever heard.
    static std::vector<QPointF> envelopePoints(const Envelope & envelope, double x0, double x1, double pixelsPerSecond);

    //! @p peaks scaled so that the loudest is one. All silent stays all zero.
    static std::vector<double> normalisedPeaks(const std::vector<double> & peaks);

    //! @p peaks as drawn on a fixed scale: anything past full scale is drawn at full height.
    static std::vector<double> clampedPeaks(const std::vector<double> & peaks);

signals:
    void waveformDataChanged();
    void normalizedChanged();
    void startOffsetChanged();
    void endOffsetChanged();
    void accentColorChanged();
    void envelopeChanged();

private:
    void setEnvelopeValue(double & member, double value);
    void updatePeaks();
    void paintPeaks(QPainter & painter) const;
    void paintTrims(QPainter & painter) const;
    void paintEnvelope(QPainter & painter) const;

    QVariantList m_waveformData;
    bool m_normalized = true;
    //! The same peaks as numbers, already scaled, so that a repaint does no conversion.
    std::vector<double> m_peaks;
    double m_startOffset = 0.0;
    double m_endOffset = 1.0;
    QColor m_accentColor { "#4fc3f7" };

    bool m_showEnvelope = false;
    double m_duration = 0.0;
    Envelope m_envelope;
};

} // namespace noteahead

#endif // WAVEFORM_RENDERER_HPP
