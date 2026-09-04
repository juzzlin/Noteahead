// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
#ifndef SAMPLER_CONTROLLER_TEST_HPP
#define SAMPLER_CONTROLLER_TEST_HPP

#include <QObject>

namespace noteahead {

class SamplerControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_sampleRateChange_shouldUpdateHzValues();
    void test_properties_shouldUpdateDeviceAndEmitSignals();
    void test_selectedPadLoopStart_secondsAndMilliseconds_shouldCombineIntoOneOffset();
    void test_selectedPadStartOffset_wholeSecond_shouldReadBackWhole();
    void test_selectedPadLoop_enabled_shouldDropTheLoopPointInTheMiddleOfTheRange();
    void test_selectedPadLoop_enabled_shouldKeepALoopPointThePadAlreadyHas();
    void test_reset_shouldRestoreDefaultValues();
    void test_setSampler_shouldRefreshGlobalSwitchesToReflectNewInstance();
    void test_loadedPads_shouldListOnlyLoadedPads();
    void test_copyPad_shouldCopyPadToTarget();
    void test_copyPad_samePad_shouldDoNothing();
};

} // namespace noteahead

#endif // SAMPLER_CONTROLLER_TEST_HPP
