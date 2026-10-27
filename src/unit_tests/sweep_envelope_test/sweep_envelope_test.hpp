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

#ifndef SWEEP_ENVELOPE_TEST_HPP
#define SWEEP_ENVELOPE_TEST_HPP

#include <QObject>

namespace noteahead {

class SweepEnvelopeTest : public QObject
{
    Q_OBJECT

private slots:
    void test_sweepEnvelope_lengthBeats_shouldEndOnTheBeat_data();
    void test_sweepEnvelope_lengthBeats_shouldEndOnTheBeat();
    void test_sweepEnvelope_lengthBeats_tempoChange_shouldApplyToRemainder();
    void test_sweepEnvelope_lengthSeconds_shouldEndOnTime();
    void test_sweepEnvelope_curve_shouldShapeMidpoint_data();
    void test_sweepEnvelope_curve_shouldShapeMidpoint();
    void test_sweepEnvelope_modeHold_shouldStayAtEnd();
    void test_sweepEnvelope_modeLoop_shouldRestartFromStart();
    void test_sweepEnvelope_modePingPong_shouldReturnToStart();
    void test_sweepEnvelope_untriggered_shouldStayAtStart();
    void test_sweepEnvelope_lengthZero_shouldJumpToEnd();
};

} // namespace noteahead

#endif // SWEEP_ENVELOPE_TEST_HPP
