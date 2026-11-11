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

#ifndef EQ_8_BAND_PARAMETRIC_PRESETS_TEST_HPP
#define EQ_8_BAND_PARAMETRIC_PRESETS_TEST_HPP

#include <QObject>

namespace noteahead {

class Eq8BandParametricPresetsTest : public QObject
{
    Q_OBJECT

private slots:
    void test_presets_shouldNotBeEmpty();
    void test_presets_names_shouldBeUniqueAndNonEmpty();
    void test_presets_flat_shouldComeFirst();
    void test_presets_afterFlat_shouldBeAlphabetical();
    void test_presets_shouldNameOnlyKnownParameters();
    void test_presets_shouldFitTheAvailableBands();
    void test_presets_bandsInUse_shouldRunLowToHigh();
    void test_presets_gains_shouldStayWithinAFewDecibels();

    void test_flat_shouldBypassEveryBand();
    void test_leadVocal_shouldMatchTheSourceNotes();
    void test_masterBus_shouldMatchTheSourceNotes();
    void test_highPass_shouldBe12DbPerOctave();
    void test_highPass_shouldCostOneBand();
    void test_oneOctaveBell_shouldUseTheQThatSpansAnOctave();

    void test_applyFactoryPreset_shouldReachTheAudio();
    void test_applyFactoryPreset_shouldReplaceThePreviousPreset();
    void test_applyFactoryPreset_outOfRange_shouldFail();
    void test_factoryPresets_defaultEffect_shouldOfferNone();
};

} // namespace noteahead

#endif // EQ_8_BAND_PARAMETRIC_PRESETS_TEST_HPP
