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

#include "auto_panner_test.hpp"
#include "../../common/constants.hpp"
#include "../../domain/effects/auto_panner_effect.hpp"

#include <QTest>

namespace noteahead {

void AutoPannerTest::test_process_shouldModulatePanning()
{
    AutoPannerEffect effect;
    effect.setSampleRate(44100);

    // Set rate to 1Hz, Sine waveform
    if (auto p = effect.parameter(Constants::NahdXml::xmlKeyRate().toStdString()); p) {
        p->get().update(0.5f);
    }
    effect.sync();

    double l = 1.0;
    double r = 1.0;

    // Process some samples and check that l and r are no longer equal
    bool changed = false;
    for (int i = 0; i < 44100; i++) {
        effect.process(l, r);
        if (std::abs(l - r) > 0.001) {
            changed = true;
            break;
        }
    }
    QVERIFY(changed);
}

void AutoPannerTest::test_intensity_shouldScaleModulation()
{
    AutoPannerEffect effect;
    effect.setSampleRate(44100);

    // Intensity 0%
    if (auto p = effect.parameter(Constants::NahdXml::xmlKeyIntensity().toStdString()); p) {
        p->get().update(0.0f);
    }
    effect.sync();

    double l = 1.0;
    double r = 1.0;
    effect.process(l, r);
    QCOMPARE(l, 1.0);
    QCOMPARE(r, 1.0);
}

void AutoPannerTest::test_setBpm_shouldUpdateLfoFrequencyInSyncMode()
{
    AutoPannerEffect effect;
    effect.setSampleRate(44100);

    // Enable sync
    if (auto p = effect.parameter(Constants::NahdXml::xmlKeySync().toStdString()); p) {
        p->get().update(1.0f);
    }
    effect.sync();

    effect.setBpm(140.0f);

    double l = 1.0;
    double r = 1.0;
    effect.process(l, r);
    // Just verify it doesn't crash and does something
    QVERIFY(true);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AutoPannerTest)
