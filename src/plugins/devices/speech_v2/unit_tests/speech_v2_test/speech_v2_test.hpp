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

#ifndef SPEECH_V2_TEST_HPP
#define SPEECH_V2_TEST_HPP

#include <QObject>

namespace noteahead {

class SpeechV2Test : public QObject
{
    Q_OBJECT

private slots:
    void test_noteOn_shouldSoundAndGoSilentAfterPhrase();
    void test_noteOn_shouldFollowNotePitch_data();
    void test_noteOn_shouldFollowNotePitch();
    void test_velocitySensitivity_shouldScaleSoftNote();
    void test_voiceTypes_allNineShouldBeBoundedAndDistinct();
    void test_loudness_helloWorld_shouldMatchSpeechV1();
    void test_midiCc_shouldReachParametersAndResetCleanly();
    void test_triggerAndSyncModes_shouldBehaveAndSeek();
    void test_dialogModel_roundTripAndRequestSettings();
};

} // namespace noteahead

#endif // SPEECH_V2_TEST_HPP
