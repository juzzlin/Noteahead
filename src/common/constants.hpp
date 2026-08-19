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

#include <chrono>
#include <cstdint>

#include <QString>

namespace noteahead::Constants {

using namespace std::chrono_literals;

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

// How many note columns the editor shows side by side at most. Narrow editors show fewer,
// down to minVisibleUnitCount(), so that the track headers stay readable.
quint64 defaultVisibleUnitCount();
quint64 minVisibleUnitCount();

// A song always keeps at least this many tracks, regardless of how many of them fit on screen.
quint64 minTrackCount();

size_t deviceRackSize();
size_t effectRackSize();

int transposeMin();
int transposeMax();

QString samplerDeviceName();
QString synthDeviceName();
QString wavetableSynthDeviceName();
QString bassSynthDeviceName();
QString drumSynthDeviceName();
QString pianoSynthDeviceName();
QString pianoSynthV2DeviceName();
QString kick808DeviceName();
QString subMixerDeviceName();
QString stringVoiceDeviceName();
QString stringEnsembleDeviceName();

QString internalDevicePortPrefix();
double defaultSampleRate();

//! Real-time priority requested for the audio callback thread. The playback worker threads sit one
//! step below it, so they can never preempt the thread that is waiting for them.
int audioCallbackRealTimePriority();

constexpr float minEffectLevel()
{
    return 0.001f;
}

//! Level the Device Rack's meter marker sits at, in dBFS: where a gain staged device should be
//! reading. -18 is the EBU R128 reference; -20 (SMPTE), -14 and -12 are the other common choices,
//! which is why this is a setting rather than a fixed line.
constexpr int defaultGainStagingTargetDb()
{
    return -18;
}

//! Milliseconds a note-off precedes the next note-on on the same column, unless the song or the
//! channel says otherwise. Real MIDI hardware needs the gap to retrigger. This is both the fallback
//! for a song that stores no offset and the value new songs are seeded with, so the two cannot drift
//! apart. Long enough for hardware to notice, short enough to stay well inside a single line.
constexpr std::chrono::milliseconds defaultAutoNoteOffOffset()
{
    return 25ms;
}

constexpr float uiInternalScaling()
{
    return 1000.0f;
}

//! Where unity gain sits on a fader's 0..1 throw. Everything above it is boost, so the usable
//! range is -inf .. maxFaderBoostDb(). Chosen so the taper below unity is the linear-amplitude
//! one faders had before the boost range existed, just compressed into the lower part of the throw.
//! How automations are drawn over the tracker lines.
//!
//! Curve plots each automation as its own vertical trace across the column, value on the horizontal
//! axis; Tint is the older single blended background colour per line.
enum class AutomationDisplayMode
{
    Tint = 0,
    Curve = 1
};

constexpr int defaultAutomationDisplayMode()
{
    return static_cast<int>(AutomationDisplayMode::Curve);
}

//! Width of an automation curve in the tracker, in tenths of a pixel. Stored as tenths because the
//! useful range is finer than a whole pixel, matching how the render settings store their level.
constexpr int defaultAutomationCurveThicknessTenths()
{
    return 15;
}

constexpr float faderUnityPosition()
{
    return 0.75f;
}

//! How far a fader can be pushed past unity, in dB.
constexpr float maxFaderBoostDb()
{
    return 10.0f;
}

//! Highest MIDI CC value an internal device's fader accepts.
//!
//! 127 keeps meaning unity, so every stored automation value means exactly what it always did; the
//! values above it are what reaches into the boost range. External gear stays on the MIDI 1.0
//! range — PropertyService is what decides which of the two a port gets.
constexpr int faderMaxMidiCcValue()
{
    return static_cast<int>(127.0f / faderUnityPosition() + 1.0f);
}

static_assert(faderMaxMidiCcValue() > 127 && faderMaxMidiCcValue() <= 255, "The fader's CC range has to extend past MIDI 1.0 and still fit in a byte");

//! MIDI CC controller the tracker's pan column writes to.
constexpr uint8_t panMidiCcController()
{
    return 10;
}

namespace RackEffectType {
QString reverb();
QString compressor();
QString multibandCompressor();
QString delay();
QString dimension();
QString chorus();
QString clipper();
QString drive();
QString bassGrinder();
QString saturator();
QString tubeStage();
QString waveDesigner();
QString stereoEnhancer();
QString stereoExciter();
QString stereoWidener();
QString stereoFieldMeter();
QString limiter();
QString endless();
QString panner();
QString autoPanner();
QString autoDucker();
QString autoFilter();
QString phaser();
QString eq8BandParametric();
QString eq8BandParametricLegacy();
QString vintagePassiveEq();
QString airBandEq();
QString simpleEq();
QString allPassFilter();
QString lufsMeter();
QString dbtpMeter();
QString rta();
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
//! Live column indices in order. Written only when they are not the plain 0..columnCount-1.
QString xmlKeyColumnIndices();
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
QString xmlKeyAutoNoteOffSyncEnabled();
QString xmlKeyAutoNoteOffSyncDenominator();
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
QString xmlKeyCurve();
QString xmlKeyComment();
QString xmlKeyMetadata();
QString xmlKeyRenderSettings();
QString xmlKeySongSettings();
QString xmlKeyFormat();
QString xmlKeySampleRate();
QString xmlKeyBitDepth();
QString xmlKeyOversampleFactor();
QString xmlKeyNormalizeEnabled();
QString xmlKeyNormalizeLevelTenthsDb();
QString xmlKeyTrimEnabled();
QString xmlKeyTrimMinutes();
QString xmlKeyTrimSeconds();
QString xmlKeyFadeOutEnabled();
QString xmlKeyFadeOutSeconds();
QString xmlKeyFadeOutTenths();
QString xmlKeySilenceEnabled();
QString xmlKeySilenceSeconds();
QString xmlKeySilenceTenths();
QString xmlKeyAnalyzeEnabled();
QString xmlKeyTags();
QString xmlKeyTag();
QString xmlKeyTitle();
QString xmlKeyArtist();
QString xmlKeyAlbum();
QString xmlKeyDate();
QString xmlKeyGenre();
QString xmlKeyTrackNumber();

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

QString xmlKeyNone();
QString xmlKeyNote();
QString xmlKeyNoteOn();
QString xmlKeyNoteOff();
QString xmlKeyNoteData();

QString xmlKeySlot();
QString xmlKeyMembers();
QString xmlKeyMember();
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
QString xmlKeyFader();
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
QString xmlKeyStereoMode();
QString xmlKeyStereoPhase();
QString xmlKeyBaseRate();
QString xmlKeyRate();
QString xmlKeyDepth();
QString xmlKeyDetune();
QString xmlKeyFeedback();
QString xmlKeyRateDivider();
QString xmlKeyWidth();
QString xmlKeyMix();
QString xmlKeyThreshold();
QString xmlKeyRatio();
QString xmlKeyKnee();
QString xmlKeyMakeup();
QString xmlKeyCeiling();
QString xmlKeyBoost();
QString xmlKeyFreeze();
QString xmlKeyGated();
QString xmlKeyHold();
QString xmlKeyDrive();
QString xmlKeyBias();
QString xmlKeyBlend();
QString xmlKeySplitFreq();
QString xmlKeyColor();
QString xmlKeySize();
QString xmlKeyDamping();
QString xmlKeyPreDelay();
QString xmlKeyVoiceDepth();
QString xmlKeyPortamento();
QString xmlKeyPanSpread();
QString xmlKeyVoiceBalance();
QString xmlKeyVoiceUpperMale8();
QString xmlKeyPitchBendRange();
QString xmlKeyResonance();
QString xmlKeyFilterType();
QString xmlKeyFilterSlope();
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
QString xmlKeyCrossModDepth();

QString xmlKeyBandType(size_t bandIndex);
QString xmlKeyBandFreq(size_t bandIndex);
QString xmlKeyBandGain(size_t bandIndex);
QString xmlKeyBandQ(size_t bandIndex);
QString xmlKeyBandThreshold(size_t bandIndex);
QString xmlKeyBandRatio(size_t bandIndex);
QString xmlKeyBandKnee(size_t bandIndex);
QString xmlKeyBandAttack(size_t bandIndex);
QString xmlKeyBandRelease(size_t bandIndex);
QString xmlKeyBandMakeup(size_t bandIndex);
QString xmlKeyBandBypass(size_t bandIndex);
QString xmlKeyBandSolo(size_t bandIndex);
QString xmlKeyBandWidth(size_t bandIndex);
QString xmlKeyCrossoverFreq(size_t crossoverIndex);

QString xmlKeyAmount();
QString xmlKeyBassGain();
QString xmlKeyBassFreq();
QString xmlKeyMidGain();
QString xmlKeyMidFreq();
QString xmlKeyMidQ();
QString xmlKeyHighGain();
QString xmlKeyHighFreq();
QString xmlKeyPeak();
QString xmlKeyZeroFill();
QString xmlKeyTimbre();
QString xmlKeyHarmonics();
QString xmlKeySolo();
QString xmlKeySpread();
QString xmlKeyMonoBass();
QString xmlKeyMonoFreq();
QString xmlKeyLowFreq();
QString xmlKeyLowBoost();
QString xmlKeyLowAtten();
QString xmlKeyHighBoostFreq();
QString xmlKeyHighBoost();
QString xmlKeyBandwidth();
QString xmlKeyHighAttenFreq();
QString xmlKeyHighAtten();

QString xmlKeyAirFreq();
QString xmlKeyAirGain();

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
QString xmlKeyAmpCurve();

QString xmlKeyModAttack();
QString xmlKeyModDecay();
QString xmlKeyModIntensity();
QString xmlKeyModSustain();
QString xmlKeyModTarget();
QString xmlKeyModCurve();

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
QString xmlKeyVoice();
QString xmlKeyVoices();
QString xmlKeySamplePath();
QString xmlKeyChannelMode();
QString xmlKeyChromaticMode();
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

QString xmlKeyFrequency();
QString xmlKeyQ();
QString xmlKeyStages();

QString xmlKeyBrightness();
QString xmlKeyInharmonicity();
QString xmlKeyHardness();
QString xmlKeyStringDetune();
QString xmlKeyStretch();
QString xmlKeyRichness();
QString xmlKeyDoubleDecay();

QString xmlKeyLufsMeter();
QString xmlKeyDbTpMeter();

QString xmlKeyBandCount();
QString xmlKeyDbRange();
QString xmlKeyShowPinkNoise();
QString xmlKeyPinkNoiseLevel();
QString xmlKeySpeed();
QString xmlKeyZoom();
QString xmlKeyShowGuides();
QString xmlKeyFftRate();

QString embeddedDataPathPrefix();

QString xmlValueFalse();
QString xmlValueTrue();
QString xmlValueInt();
QString xmlValueBool();
QString xmlValueFloat();
QString xmlValueSineWave();
QString xmlValueRandom();
QString xmlValueLinear();
QString xmlValueExponential();
QString xmlValueLogarithmic();
QString xmlValueEaseIn();
QString xmlValueEaseOut();
QString xmlValueEaseInOut();
QString xmlValueSamplers();
QString xmlValueSynths();
QString xmlValueMixers();
QString xmlValueDrums();

QString xmlKeyStringsBalance();
QString xmlKeyStringsLevel8();
QString xmlKeyStringsLevel4();
QString xmlKeyStringsAttack();
QString xmlKeyStringsRelease();
QString xmlKeyVoiceMale8();
QString xmlKeyVoiceMale4();
QString xmlKeyVoiceFemale8();
QString xmlKeyVoiceFemale4();
QString xmlKeyVoiceAttack();
QString xmlKeyVoiceRelease();
QString xmlKeyVibratoRate();
QString xmlKeyVibratoDepth();
QString xmlKeyVibratoDelay();
QString xmlKeyEnsembleEnabled();
QString xmlKeyEnsembleMode();
QString xmlKeyVocoderEnabled();
QString xmlKeyVocoderSidechain();

QString xmlKeyContrabass();
QString xmlKeyCello();
QString xmlKeyViola();
QString xmlKeyViolin();
QString xmlKeyTrumpet();
QString xmlKeyHorn();
QString xmlKeyVolumeBass();
QString xmlKeyPhaserEnabled();
QString xmlKeyPhaserColor();
QString xmlKeyPhaserRate();
QString xmlKeyVelocitySensitivity();
QString xmlKeyFaderPosition();
QString xmlKeySendTap();

} // namespace NahdXml
} // namespace noteahead::Constants

#endif // CONSTANTS_HPP
