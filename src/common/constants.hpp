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
QString clipper();
QString panner();
QString eq8BandParametric();
}

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

QString xmlKeyReverbSize();
QString xmlKeyReverbDecay();
QString xmlKeyReverbDamping();
QString xmlKeyReverbPreDelay();
QString xmlKeyReverbMix();
QString xmlKeyReverbWidth();
QString xmlKeyReverbLpfCutoff();
QString xmlKeyReverbHpfCutoff();

QString xmlKeyCompressorThreshold();
QString xmlKeyCompressorRatio();
QString xmlKeyCompressorKnee();
QString xmlKeyCompressorMakeup();

QString xmlKeyClipperMode();
QString xmlKeyClipperThreshold();
QString xmlKeyClipperGain();

QString xmlKeyEq8BandParametricType(int bandIndex);
QString xmlKeyEq8BandParametricFreq(int bandIndex);
QString xmlKeyEq8BandParametricGain(int bandIndex);
QString xmlKeyEq8BandParametricQ(int bandIndex);

QString xmlKeySynthVco1Waveform();
QString xmlKeySynthVco1Octave();
QString xmlKeySynthVco1Pitch();
QString xmlKeySynthVco1Shape();
QString xmlKeySynthVco1Sync();

QString xmlKeySynthVco2Waveform();
QString xmlKeySynthVco2Octave();
QString xmlKeySynthVco2Pitch();
QString xmlKeySynthVco2Shape();
QString xmlKeySynthVco2Sync();

QString xmlKeySynthVco3Waveform();
QString xmlKeySynthVco3Octave();
QString xmlKeySynthVco3Pitch();
QString xmlKeySynthVco3Shape();
QString xmlKeySynthVco3Sync();

QString xmlKeySynthMultiMode();
QString xmlKeySynthMultiShape();
QString xmlKeySynthMultiLevel();

QString xmlKeySynthMixLevel1();
QString xmlKeySynthMixLevel2();
QString xmlKeySynthMixLevel3();

QString xmlKeySynthLpfCutoff();
QString xmlKeySynthLpfResonance();
QString xmlKeySynthHpfCutoff();

QString xmlKeySynthAmpAttack();
QString xmlKeySynthAmpDecay();
QString xmlKeySynthAmpSustain();
QString xmlKeySynthAmpRelease();

QString xmlKeySynthModAttack();
QString xmlKeySynthModDecay();
QString xmlKeySynthModIntensity();
QString xmlKeySynthModTarget();

QString xmlKeyUserPresets();
QString xmlKeyPreset();
QString xmlKeyTypeId();

QString xmlKeySynthLfoWaveform();
QString xmlKeySynthLfoMode();
QString xmlKeySynthLfoRate();
QString xmlKeySynthLfoIntensity();
QString xmlKeySynthLfoTarget();

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
QString xmlKeyStartOffset();

QString xmlKeyDrumSynth();
QString xmlKeyPad();
QString xmlKeyTune();
QString xmlKeyClickTune();
QString xmlKeySnappy();
QString xmlKeyTone();
QString xmlKeyPitchDepth();
QString xmlKeyPitchDecay();

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
