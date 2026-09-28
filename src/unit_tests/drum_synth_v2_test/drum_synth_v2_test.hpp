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

#ifndef DRUM_SYNTH_V2_TEST_HPP
#define DRUM_SYNTH_V2_TEST_HPP

#include <QObject>

namespace noteahead {

class DrumSynthV2Test : public QObject
{
    Q_OBJECT

private slots:
    void test_drumSynthV2Device_typeId_shouldDifferFromV1();
    void test_voiceSend_shouldReachTheBusOnItsOwn();
    void test_voiceSend_unrouted_shouldSendNothing();
    void test_drumSynthV2Device_everyVoice_shouldStayCloseToV1();
    void test_drumSynthV2Device_midiNoteOn_shouldTriggerVoice();
    void test_ampEnvelope_shortHold_shouldTightenTheVoice();
    void test_ampEnvelope_fullSustain_shouldMatchV1Exactly();
    void test_ampEnvelope_closed_shouldStopTheVoiceRendering();
    void test_ampEnvelope_curve_shouldBendTheVoicesDecay();
    void test_renderVoiceAlone_shouldStopWhenTheVoiceDoes();
    void test_renderVoiceAlone_shouldBeDeterministic();
    void test_renderVoiceAlone_shouldRenderOnlyThatVoice();
    void test_voicePreview_shouldPictureTheVoiceAndMeasureWhatIsHeard();
    void test_voicePreview_shouldNotDisturbTheDeviceItPictures();
    void test_voicePreview_shortEnvelope_shouldShortenOnlyWhatIsHeard();
    void test_voiceElapsedSeconds_shouldFollowTheVoice();
    void test_toms_shouldBeStruckNotJustPitched();
    void test_toms_shouldBePitchedLikeTheRecordings();
    void test_snare_shouldBeADrumRatherThanASizzle();
    void test_reverseCrash_shouldBeTheCrashRunBackwards();
    void test_cymbals_v1_shouldNotTakeTheFit();
    void test_cymbals_ride_shouldBeAsNoisyAsRealMetal();
    void test_cymbals_tune_shouldOnlyEverBrighten();
    void test_cymbals_crash_shouldBloom();
    void test_cymbals_crash_shouldPeakInTheSplashBand();
    void test_cymbals_shouldHaveABody();
    void test_rim_shouldBeAShortPitchedClick();
    void test_rim_shouldNotDisturbAProjectSavedWithoutIt();
    void test_drumSynthV2Device_xmlSerialization_shouldRestoreParameters();
};

} // namespace noteahead

#endif // DRUM_SYNTH_V2_TEST_HPP
