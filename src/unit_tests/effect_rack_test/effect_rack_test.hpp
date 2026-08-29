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

#ifndef EFFECT_RACK_TEST_HPP
#define EFFECT_RACK_TEST_HPP

#include <QObject>

namespace noteahead {

class EffectRackTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void test_addRemove_shouldAddAndRemoveEffects();
    void test_version_shouldChangeOnlyWhenEffectsChange();
    void test_rackEnabled_shouldBypassWholeRack();
    void test_rackEnabled_serialization_shouldRoundTrip();
    void test_process_shouldProcessAudio();
    void test_setEffect_install_shouldSyncParameters();
    void test_processInPlace_shouldApplyEffectToBuffer();
    void test_serialization_shouldSerializeAndDeserializeEffects();
    void test_enabled_flag_shouldControlProcessing();
    void test_reverb_parameters_shouldGetAndSetParameters();
    void test_reverb_presets_shouldApplyPresets();
    void test_exportImportEffectSettings_shouldWorkForSingleEffect();
    void test_importEffectSettings_backwardsCompatibility();
    void test_exportImportEffectSettings_drive_shouldRoundTrip();
    void test_copyEffect_shouldDuplicateIntoTargetSlot();
    void test_copyEffect_emptySource_shouldFail();
    void test_copyEffect_sameSlot_shouldFail();
    void test_copyFrom_shouldCloneEffectsIndependently();
    void test_copyFrom_emptySource_shouldClearTarget();
    void test_swapEffects_shouldSwapTwoSlots();
    void test_swapEffects_outOfBounds_shouldDoNothing();
    void test_moveEffect_upwards_shouldShiftSlotsInBetweenDown();
    void test_moveEffect_downwards_shouldShiftSlotsInBetweenUp();
    void test_moveEffect_sameSlot_shouldDoNothing();
    void test_moveEffect_outOfBounds_shouldDoNothing();
};

} // namespace noteahead

#endif // EFFECT_RACK_TEST_HPP
