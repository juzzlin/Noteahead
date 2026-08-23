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

#ifndef XML_SERIALIZATION_TEST_HPP
#define XML_SERIALIZATION_TEST_HPP

#include <QObject>

namespace noteahead {

class XmlSerializationTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_toXmlFromXml_addTrack_shouldLoadSong();

    void test_toXmlFromXml_columnName_shouldLoadColumnName();
    void test_toXmlFromXml_columnSettings_shouldSaveAndLoad();
    void test_toXmlFromXml_columnDeleted_shouldLoadColumnIndices();
    void test_toXmlFromXml_columnMoved_shouldLoadColumnOrder();
    void test_toXmlFromXml_columnInserted_shouldLoadColumnOrder();
    void test_toXmlFromXml_trackMoved_shouldLoadTrackOrder();
    void test_toXml_noColumnsDeleted_shouldNotWriteColumnIndices();
    void test_fromXml_legacyNoColumnIndices_shouldLoadConsecutiveIndices();

    void test_toXmlFromXml_instrumentSettings_shouldParseInstrumentSettings();
    void test_toXmlFromXml_instrumentSettings_syncedAutoNoteOff_shouldRoundTrip();
    void test_toXmlFromXml_instrument_shouldParseInstrument();

    void test_toXmlFromXml_sideChainService_shouldLoadSideChainService();

    void test_toXmlFromXml_automationService_midiCc_curve_shouldLoadAutomationService();
    void test_toXmlFromXml_automationService_midiCc_linearCurve_shouldNotWriteCurveAttribute();
    void test_toXmlFromXml_automationService_midiCc_noModulation_shouldLoadAutomationService();
    void test_toXmlFromXml_automationService_midiCc_shouldLoadAutomationService();
    void test_toXmlFromXml_automationService_midiCc_withModulation_shouldLoadAutomationService();
    void test_toXmlFromXml_automationService_pitchBend_curve_shouldLoadAutomationService();
    void test_toXmlFromXml_automationService_pitchBend_shouldLoadAutomationService();
    void test_toXmlFromXml_automationService_pitchBend_withModulation_shouldLoadAutomationService();

    void test_toXmlFromXml_mixerService_shouldLoadMixerService();

    void test_toXmlFromXml_noteData_noteOff_shouldBeCorrect();
    void test_toXmlFromXml_noteData_noteOn_shouldBeCorrect();
    void test_toXmlFromXml_noteData_delay_shouldSaveAndLoadDelay();
    void test_toXmlFromXml_noteData_pan_shouldSaveAndLoadPan();
    void test_toXmlFromXml_noteData_pan_absent_shouldDefaultToNullopt();
    void test_toXmlFromXml_noteData_pan_panOnly_shouldSaveAndLoad();

    void test_toXmlFromXml_playOrder_shouldBeCorrect();
    void test_toXmlFromXml_removeTrack_shouldLoadSong();
    void test_toXmlFromXml_songProperties_shouldBeCorrect();
    void test_toXmlFromXml_songMetadata_shouldRoundTrip();
    void test_toXmlFromXml_songMetadata_empty_shouldRoundTrip();
    void test_toXmlFromXml_songSettings_milliseconds_shouldRoundTrip();
    void test_toXmlFromXml_songSettings_sync_shouldRoundTrip();
    void test_fromXml_songSettingsMissing_shouldSeedFromApplicationDefault();
    void test_fromXml_legacyAutoNoteOffOffset_shouldLoadAsMilliseconds();
    void test_toXmlFromXml_trackName_shouldLoadTrackName();
    void test_toXmlFromXml_trackDrumTrack_shouldLoadTrackDrumTrack();
    void test_toXmlFromXml_samplerDevice_shouldLoadSamplerDevice();
    void test_toXmlFromXml_samplerDevice_padEffectRack_shouldRoundTrip();
    void test_toXmlFromXml_drumSynthDevice_voiceEffectRack_shouldRoundTrip();
    void test_toXmlFromXml_samplerDevice_relativePath_shouldLoadCorrectly();
    void test_toXmlFromXml_samplerDevice_saveAs_shouldPreserveEmbeddedData();
    void test_toXml_whileAutomated_shouldSaveAuthoredValues();
    void test_toXmlFromXml_synthDevice_shouldPreserveValuesAndDiscreteFlags();
    void test_toXmlFromXml_synthUserPresets_shouldSaveAndLoad();
    void test_toXmlFromXml_synthUserPresets_discreteValues_shouldSaveAndLoad();
    void test_toXmlFromXml_masterSendEffects_shouldLoadCorrectly();
    void test_toXmlFromXml_chorusEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_endlessReverbEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_vintagePassiveEqEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_airBandEqEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_subMixerDevice_shouldLoadCorrectly();
    void test_toXmlFromXml_stringEnsembleDevice_shouldLoadCorrectly();
    void test_toXmlFromXml_pianoSynthV2Device_shouldLoadCorrectly();
    void test_toXmlFromXml_pianoSynthV3Device_shouldLoadCorrectly();
    void test_toXmlFromXml_kick808Device_shouldLoadCorrectly();
    void test_toXmlFromXml_tubeStage_shouldLoadCorrectly();
    void test_toXmlFromXml_bassGrinder_shouldLoadCorrectly();
    void test_toXmlFromXml_simpleEqEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_stereoExciter_shouldLoadCorrectly();
    void test_toXmlFromXml_stereoEnhancer_shouldLoadCorrectly();
    void test_toXmlFromXml_waveDesigner_shouldLoadCorrectly();
    void test_toXmlFromXml_reverbGate_shouldLoadCorrectly();
    void test_toXmlFromXml_limiterEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_autoDuckerEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_multibandCompressorEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_stereoWidenerEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_stereoFieldMeterEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_analogFuzzEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_gainEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_monitorEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_dimensionEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_earlyReflectionsEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_autoFilterEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_phaserEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_lufsMeterEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_dbtpMeterEffect_shouldLoadCorrectly();
    void test_toXmlFromXml_delayEffectRack_shouldLoadCorrectly();
    void test_fromXml_samplerDevice_missingId_shouldNotThrow();

    void test_toXmlFromXml_differentSongs_shouldLoadSongs();
    void test_toXmlFromXml_template_shouldLoadTemplate();
    void test_toXmlFromXml_audioRecorder_shouldLoadAudioRecorder();

    void test_fromXml_missingPatterns_shouldRemoveThemFromPlayOrder();
    void test_fromXml_legacyLength_shouldBeSupported();

    void test_stringVoice_legacyFemale8_shouldLoadAsUpperMale8();
    void test_wavetableSynth_legacyNames_shouldLoadCorrectly();
    void test_wavetableSynth_legacyWavetableRange_shouldPreserveSelection();
    void test_eq8BandParametric_legacyNames_shouldLoadCorrectly();
    void test_drive_legacyDrive_shouldKeepItsGain();
    void test_saturator_legacyDrive_shouldKeepItsGain();
    void test_tubeStage_legacyDrive_shouldKeepItsGain();
    void test_chorus_legacyNames_shouldLoadCorrectly();
    void test_clipper_legacyNames_shouldLoadCorrectly();
    void test_reverb_legacyNames_shouldLoadCorrectly();
    void test_allPassFilter_legacyNames_shouldLoadCorrectly();
};

} // namespace noteahead

#endif // XML_SERIALIZATION_TEST_HPP
