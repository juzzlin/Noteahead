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

#ifndef SAMPLER_TEST_HPP
#define SAMPLER_TEST_HPP

#include <QObject>

namespace noteahead {

class SamplerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void test_initialState_shouldBeCorrect();

    void test_loadAndClearSample_shouldUpdateModel();

    void test_midiNoteOn_shouldPlaySample();
    void test_midiAllNotesOff_shouldStopAllVoices();

    void test_pan_shouldAdjustPanning();
    void test_volume_shouldAdjustVolume();
    void test_cutoff_shouldAdjustCutoff();

    void test_channelMode_shouldToggleCorrectMode();
    void test_midiCcReset_shouldResetInternalValues();

    void test_startOffset_shouldShiftPlaybackStart();
    void test_reset_shouldResetParametersAndPads();
    void test_processAudio_shouldProduceOutput();
    void test_serialization_shouldSaveAndLoadGain();
    void test_midiCcResetGlobalPanAndVolume_shouldRestoreManualValues();
    void test_projectLoadMidiCcResetGlobal_shouldRestoreLoadedValues();
};

} // namespace noteahead

#endif // SAMPLER_TEST_HPP
