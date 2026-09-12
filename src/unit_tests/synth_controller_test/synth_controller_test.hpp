// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
#ifndef SYNTH_CONTROLLER_TEST_HPP
#define SYNTH_CONTROLLER_TEST_HPP

#include <QObject>

namespace noteahead {

class SynthControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_loadPreset_shouldShowTheLoadedPreset();
    void test_presetNames_withUserPresets_shouldContinueTheFactoryNumbering();
    void test_loadPreset_userPreset_shouldApplyTheStoredPatch();
    void test_saveUserPreset_shouldSelectWhatWasJustSaved();
    void test_deleteCurrentUserPreset_factoryPreset_shouldDeleteNothing();
    void test_deleteCurrentUserPreset_shouldDropItFromTheList();
    void test_sampleRateChange_shouldUpdateHzValues();
    void test_properties_shouldUpdateDeviceAndEmitSignals();
    void test_reset_shouldRestoreDefaultValues();
    void test_octaveNames();
    void test_squareWaveformIndex_shouldMatchWaveformNames();
    void test_voiceModes();
    void test_lfoTargetNames();
    void test_lfoEngagementProperties_shouldRoundTripThroughTheDevice();
    void test_modTargetNames();
    void test_scopeActive_shouldFollowShownInstance();
};

} // namespace noteahead

#endif // SYNTH_CONTROLLER_TEST_HPP
