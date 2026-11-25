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

#ifndef ANALOG_BUS_TEST_HPP
#define ANALOG_BUS_TEST_HPP

#include <QObject>

namespace noteahead {

class AnalogBusTest : public QObject
{
    Q_OBJECT

private slots:
    void test_parameterDefaultsAndConversions();
    void test_tightHpf_attenuatesSubBassAndDifferentiatesModes();
    void test_consoleRouting_stagesAndOrderingDifferentiateHarmonics();
    void test_colorModules_produceDistinctColorationAndRespectColorMix();
    void test_toneEq_baxandallShelvesAirAndBypass();
    void test_toneEq_preVsPostAffectsSaturation();
    void test_vintage_rollsOffHighFrequencies();
    void test_aspect_widensStereoAndPreservesMono();
    void test_analog_introducesDeterministicLrDifference();
    void test_oversampling_mixSendModeAndMetering();
    void test_presets_loadAndRenderCleanly();
    void test_autoGain_drive_keepsPinkNoiseLevel();
    void test_autoGain_off_leavesOutputAsItWas();
    void test_autoGain_reset_measuresAtOnce();
    void test_reset_restoresExactReplay();
    void test_processBlock_matchesProcessSample();
};

} // namespace noteahead

#endif // ANALOG_BUS_TEST_HPP
