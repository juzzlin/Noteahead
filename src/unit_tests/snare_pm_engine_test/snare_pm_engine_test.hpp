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

#ifndef SNARE_PM_ENGINE_TEST_HPP
#define SNARE_PM_ENGINE_TEST_HPP

#include <QObject>

namespace noteahead {

class SnarePmEngineTest : public QObject
{
    Q_OBJECT

private slots:
    void test_trigger_shouldActivateEngine();
    void test_trigger_twice_shouldRenderDeterministically();
    void test_panSpread_zero_shouldBeStrictlyMono();
    void test_panSpread_full_shouldCreateStereoDifference();
    void test_snappy_zero_shouldBePureMembrane();
    void test_snareBuzz_shouldIncreaseSustainedWireRattleEnergy();
    void test_position_changesModalResponse();
    void test_click_changesInitialAttackEnergy();
    void test_hardness_changesModalBrightnessAndAttack();
    void test_body_changesFundamentalWeightAndResonance();
    void test_stop_shouldDeactivateAndSilenceEngine();
    void test_sampleRateStability();
};

} // namespace noteahead

#endif // SNARE_PM_ENGINE_TEST_HPP
