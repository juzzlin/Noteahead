// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <QString>

namespace noteahead::Constants {

QString applicationName();
QString applicationVersion();

QString copyright();

QString license();

QString fileFormatVersion();
QString fileFormatExtension();
QString deviceSettingsExtension();
QString effectRackSettingsExtension();
QString midiFileExtension();

QString qSettingsCompanyName();
QString qSettingSoftwareName();
QString webSiteUrl();

size_t defaultPatternLineCount();
size_t defaultTrackCount();

size_t deviceRackSize();
size_t effectRackSize();

int transposeMin();
int transposeMax();

QString samplerDeviceName();
QString synthDeviceName();
QString wavetableSynthDeviceName();
QString bassSynthDeviceName();
QString drumSynthDeviceName();

QString internalDevicePortPrefix();
double defaultSampleRate();

constexpr float minEffectLevel()
{
    return 0.001f;
}

constexpr float uiInternalScaling()
{
    return 1000.0f;
}

namespace RackEffectType {
QString reverb();
QString compressor();
QString delay();
QString chorus();
QString clipper();
QString panner();
QString autoPanner();
QString eq8BandParametric();
QString eq8BandParametricLegacy();
} // namespace RackEffectType

namespace NahdXml {

QString xmlKeyFileFormatVersion();

QString xmlKeyApplicationName();
QString xmlKeyApplicationVersion();

QString xmlKeyCreatedDate();

QString xmlKeyTypeName();

QString xmlKeyBankEnabled();
QString xmlKeyBankLsb();
QString xmlKeyBankMsb();
QString xmlKeyBankByteOrderSwapped();

QString xmlKeyBeatsPerMinute();

QString xmlKeyChannel();
QString xmlKeyGroup();

QString xmlKeyColumn();
QString xmlKeyColumns();
QString xmlKeyColumnCount();
QString xmlKeyColumnSettings();

QString xmlKeyChordNote1Offset();
QString xmlKeyChordNote1Velocity();
QString xmlKeyChordNote1Delay();
QString xmlKeyChordNote2Offset();
QString xmlKeyChordNote2Velocity();
QString xmlKeyChordNote2Delay();
QString xmlKeyChordNote3Offset();
QString xmlKeyChordNote3Velocity();
QString xmlKeyChordNote3Delay();
QString xmlKeyArpeggiatorEnabled();
QString xmlKeyArpeggiatorPattern();
QString xmlKeyArpeggiatorEventsPerBeat();

QString xmlKeyController();
QString xmlKeyEnabled();

QString xmlKeyCutoff();
QString xmlKeyHpfCutoff();

QString xmlKeyDelay();
QString xmlKeyMidiDelayEnabled();
QString xmlKeyMidiDelayLines();
QString xmlKeyMidiDelayFeedback();
QString xmlKeyMidiDelayMaxRepetitions();
QString xmlKeyTranspose();
QString xmlKeyDrumTrack();

QString xmlKeyVelocityJitter();
QString xmlKeyVelocityKeyTrack();
QString xmlKeyVelocityKeyTrackOffset();
QString xmlKeyAutoNoteOffOffset();
QString xmlKeyIndex();

QString xmlKeyInstrument();
QString xmlKeyInstrumentSettings();

QString xmlKeySendMidiClock();
QString xmlKeySendTransport();

QString xmlKeyPlayOrder();
QString xmlKeyPatternAttr();
QString xmlKeyPosition();
QString xmlKeySkipped();
QString xmlKeyLength();

QString xmlKeyLine();
QString xmlKeyLineEvent();
QString xmlKeyLines();
QString xmlKeyLineCount();
QString xmlKeyLinesPerBeat();

QString xmlKeyLookahead();
QString xmlKeyParameters();
QString xmlKeyMidiCcSetting();
QString xmlKeyMidiSideChain();

QString xmlKeyMixer();
QString xmlKeyMasterEffects();
QString xmlKeyInsertEffects();
QString xmlKeySendEffects();
QString xmlKeyEffect();
QString xmlKeySend();
QString xmlKeyDeviceSlot();
QString xmlKeyEffectSlot();

QString xmlKeyAudioRecorder();
QString xmlKeyLatestRecordingFilePath();
QString xmlKeyLatestRecordingStartTick();
QString xmlKeyLatestRecordingEndTick();

QString xmlKeyAutomation();

QString xmlKeyMidiCcAutomation();
QString xmlKeyPitchBendAutomation();
QString xmlKeyInterpolation();
QString xmlKeyLocation();
QString xmlKeyId();
QString xmlKeyLine0();
QString xmlKeyLine1();
QString xmlKeyValue0();
QString xmlKeyValue1();
QString xmlKeyComment();

QString xmlKeyModulation();
QString xmlKeyCycles();
QString xmlKeyAmplitude();

QString xmlKeyInverted();
QString xmlKeyOffset();

QString xmlKeyColumnAttr();
QString xmlKeyColumnMuted();
QString xmlKeyColumnSoloed();
QString xmlKeyColumnVelocityScale();
QString xmlKeyTrackAttr();
QString xmlKeyTrackMuted();
QString xmlKeyTrackSoloed();
QString xmlKeyTrackVelocityScale();

QString xmlKeyName();

QString xmlKeyNote();
QString xmlKeyNoteOn();
QString xmlKeyNoteOff();
QString xmlKeyNoteData();

QString xmlKeySlot();
QString xmlKeyPan();
QString xmlKeyReverbSend();
QString xmlKeyReverbSend1();
QString xmlKeyReverbSend2();
QString xmlKeyReverbSend3();
QString xmlKeyReverbSend4();

QString xmlKeyPatch();

QString xmlKeyPattern();
QString xmlKeyPatterns();

QString xmlKeyPortName();

QString xmlKeyRelease();
QString xmlKeyReleaseValue();
QString xmlKeyEventsPerBeat();
QString xmlKeyLineOffset();

QString xmlKeyTrack();
QString xmlKeyTracks();

QString xmlKeyTrackCount();

QString xmlKeyTargetValue();

QString xmlKeyType();
QString xmlKeyValue();

QString xmlKeyVelocity();
QString xmlKeyVolume();
QString xmlKeyGain();

QString xmlKeyProject();
QString xmlKeySettings();
QString xmlKeySong();

QString xmlKeySourceColumn();
QString xmlKeySourceTrack();
QString xmlKeySideChain();
QString xmlKeySideChainTarget();
QString xmlKeySideChainSettings();

QString xmlKeyDevices();
QString xmlKeyDevice();
QString xmlKeyCategory();
QString xmlKeyParameter();
QString xmlKeyParameterValueType();
QString xmlKeyMin();
QString xmlKeyMax();
QString xmlKeyDefault();
QString xmlKeyScale();
QString xmlKeySampler();
QString xmlKeySynth();
QString xmlKeyOscillator();
QString xmlKeyWaveform();
QString xmlKeyLevel();
QString xmlKeyShape();
QString xmlKeyPitch();
QString xmlKeySync();
QString xmlKeyOctave();
QString xmlKeyMultiType();
QString xmlKeyMultiKeyTrack();
QString xmlKeyVoiceMode();
QString xmlKeyMode();
QString xmlKeyBaseRate();
QString xmlKeyRate();
QString xmlKeyDepth();
QString xmlKeyWidth();
QString xmlKeyMix();
QString xmlKeyThreshold();
QString xmlKeyRatio();
QString xmlKeyKnee();
QString xmlKeyMakeup();
QString xmlKeySize();
QString xmlKeyDamping();
QString xmlKeyPreDelay();
QString xmlKeyVoiceDepth();
QString xmlKeyPortamento();
QString xmlKeyPanSpread();
QString xmlKeyPitchBendRange();
QString xmlKeyResonance();
QString xmlKeyKeyTrack();
QString xmlKeyAttack();
QString xmlKeyDecay();
QString xmlKeySustain();
QString xmlKeyReleaseTime();
QString xmlKeyIntensity();
QString xmlKeyTarget();
QString xmlKeyDelayType();
QString xmlKeyDelayTime();
QString xmlKeyDelayFeedback();
QString xmlKeyDelayDepth();
QString xmlKeyDelayMix();
QString xmlKeyDelaySync();
QString xmlKeyDelaySyncDivision();
QString xmlKeyDelayFeedbackLpf();
QString xmlKeyDelayFeedbackHpf();
QString xmlKeyOscillatorDrift();

QString xmlKeyBandType(size_t bandIndex);
QString xmlKeyBandFreq(size_t bandIndex);
QString xmlKeyBandGain(size_t bandIndex);
QString xmlKeyBandQ(size_t bandIndex);

QString xmlKeyVco1Waveform();
QString xmlKeyVco1Octave();
QString xmlKeyVco1Pitch();
QString xmlKeyVco1Shape();
QString xmlKeyVco1Sync();
QString xmlKeySideChainSourceDevice();
QString xmlKeySideChainLpf();

QString xmlKeyVco2Waveform();
QString xmlKeyVco2Octave();
QString xmlKeyVco2Pitch();
QString xmlKeyVco2Shape();
QString xmlKeyVco2Sync();

QString xmlKeyVco3Waveform();
QString xmlKeyVco3Octave();
QString xmlKeyVco3Pitch();
QString xmlKeyVco3Shape();
QString xmlKeyVco3Sync();

QString xmlKeyOsc1Pos();
QString xmlKeyOsc1Octave();
QString xmlKeyOsc1Pitch();
QString xmlKeyOsc1Level();

QString xmlKeyOsc2Pos();
QString xmlKeyOsc2Octave();
QString xmlKeyOsc2Pitch();
QString xmlKeyOsc2Level();

QString xmlKeyNoiseLevel();

QString xmlKeyLpfCutoff();
QString xmlKeyLpfResonance();
QString xmlKeyHpfCutoff();

QString xmlKeyAmpAttack();
QString xmlKeyAmpDecay();
QString xmlKeyAmpSustain();
QString xmlKeyAmpRelease();
QString xmlKeyAmpVelocitySensitivity();

QString xmlKeyModAttack();
QString xmlKeyModDecay();
QString xmlKeyModIntensity();
QString xmlKeyModTarget();

QString xmlKeyLfoWaveform();
QString xmlKeyLfoMode();
QString xmlKeyLfoRate();
QString xmlKeyLfoIntensity();
QString xmlKeyLfoTarget();
QString xmlKeyLfo2Waveform();
QString xmlKeyLfo2Mode();
QString xmlKeyLfo2Rate();
QString xmlKeyLfo2Intensity();
QString xmlKeyLfo2Target();
QString xmlKeyWavetableIndex();

QString xmlKeyMultiMode();
QString xmlKeyMultiShape();
QString xmlKeyMultiLevel();

QString xmlKeyMixLevel1();
QString xmlKeyMixLevel2();
QString xmlKeyMixLevel3();

QString xmlKeyUserPresets();
QString xmlKeyPreset();
QString xmlKeyTypeId();

QString xmlKeyBassSynth();
QString xmlKeySubLevel();
QString xmlKeySubOctave();
QString xmlKeyEnvMod();
QString xmlKeyAccent();
QString xmlKeySlide();
QString xmlKeyDistDrive();
QString xmlKeyDistTone();
QString xmlKeyDistLevel();

QString xmlKeySample();
QString xmlKeySamples();
QString xmlKeySamplePath();
QString xmlKeyChannelMode();
QString xmlKeyEmbedWaveData();
QString xmlKeyStartOffset();

QString xmlKeyData();

QString xmlKeyDrumSynth();
QString xmlKeyPad();
QString xmlKeyTune();
QString xmlKeyClickTune();
QString xmlKeySnappy();
QString xmlKeyTone();
QString xmlKeyPitchDepth();
QString xmlKeyPitchDecay();

QString embeddedDataPathPrefix();

QString xmlValueFalse();
QString xmlValueTrue();
QString xmlValueInt();
QString xmlValueBool();
QString xmlValueFloat();
QString xmlValueSineWave();
QString xmlValueRandom();
QString xmlValueSamplers();
QString xmlValueSynths();

} // namespace NahdXml
} // namespace noteahead::Constants

#endif // CONSTANTS_HPP
