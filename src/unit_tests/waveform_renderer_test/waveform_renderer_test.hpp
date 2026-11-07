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

#ifndef WAVEFORM_RENDERER_TEST_HPP
#define WAVEFORM_RENDERER_TEST_HPP

#include <QObject>

namespace noteahead {

class WaveformRendererTest : public QObject
{
    Q_OBJECT

private slots:
    void test_shape_zeroCurve_shouldBeLinear();
    void test_shape_fullCurve_shouldMatchTheEnvelopes();

    void test_envelopePoints_defaultEnvelope_shouldDrawABox();
    void test_envelopePoints_longAttack_shouldBeCutAtTheRangeEnd();
    void test_envelopePoints_decay_shouldHoldTheSustainToTheRangeEnd();
    void test_envelopePoints_release_shouldFallPastTheRangeEnd();

    void test_normalisedPeaks_loudest_shouldBeOne();
    void test_normalisedPeaks_silence_shouldStayZero();
    void test_clampedPeaks_pastFullScale_shouldBeFullHeight();

    void test_paint_peaks_shouldDrawOneBarPerPeak();
    void test_paint_peaksNotNormalized_shouldDrawAtTheirOwnLevel();
    void test_paint_trims_shouldShadeOutsideTheRange();
    void test_paint_envelope_shouldDrawOnlyWhenShown();
};

} // namespace noteahead

#endif // WAVEFORM_RENDERER_TEST_HPP
