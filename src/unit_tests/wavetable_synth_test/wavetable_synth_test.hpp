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

#ifndef WAVETABLE_SYNTH_TEST_HPP
#define WAVETABLE_SYNTH_TEST_HPP

#include <QObject>

namespace noteahead {

class WavetableSynthTest : public QObject
{
    Q_OBJECT

private slots:
    void test_name_shouldReturnCorrectName();
    void test_defaultValues_shouldBeCorrect();
    void test_parameterSetting_shouldUpdateValues();
    void test_polyphony_shouldActiveMultipleVoices();
    void test_midiCc_shouldUpdateParameters();
    void test_reset_shouldRestoreDefaults();
    void test_serialization_shouldPreserveState();
    void test_pitchBend_shouldUpdateFrequency();
    void test_audio_shouldProcessWhenActive();
    void test_hpf_shouldUpdateParameterAndFilterAudio();
    void test_wavetableSelection_shouldUpdateWavetable();
    void test_lfo2_defaultValues_shouldBeCorrect();
    void test_lfo2_parameterSetting_shouldUpdateValues();
    void test_lfo2_serialization_shouldPreserveState();
    void test_lfoWaveform_random_serialization_shouldPreserveState();
    void test_lfo2Waveform_random_serialization_shouldPreserveState();
};

} // namespace noteahead

#endif // WAVETABLE_SYNTH_TEST_HPP
