// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY;} without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#include "constants.hpp"

namespace noteahead::Constants {

QString applicationName()
{
    return "Noteahead";
}

QString applicationVersion()
{
    return VERSION;
}

QString copyright()
{
    return "Copyright (c) 2020-2026 Jussi Lind <jussi.lind@iki.fi>";
}

QString license()
{
    return "The GNU General Public License v3.0";
}

QString fileFormatVersion()
{
    return "1.0";
}

QString fileFormatExtension()
{
    return ".nahd";
}

QString deviceSettingsExtension()
{
    return ".nahddev";
}

QString effectRackSettingsExtension()
{
    return ".nahdeff";
}

QString midiFileExtension()
{
    return ".mid";
}

QString qSettingsCompanyName()
{
    return applicationName();
}

QString webSiteUrl()
{
    return "https://juzzlin.github.io/Noteahead";
}

QString qSettingSoftwareName()
{
    return applicationName();
}

size_t defaultPatternLineCount()
{
    return 64;
}

size_t defaultTrackCount()
{
    return 8;
}

size_t deviceRackSize()
{
    return 8;
}

size_t effectRackSize()
{
    return 8;
}

QString samplerDeviceName()
{
    return "Noteahead Sampler";
}

QString synthDeviceName()
{
    return "Noteahead Synth";
}

QString wavetableSynthDeviceName()
{
    return "Noteahead WavetableSynth";
}

QString bassSynthDeviceName()
{
    return "Noteahead BassSynth";
}

QString drumSynthDeviceName()
{
    return "Noteahead DrumSynth";
}

QString pianoSynthDeviceName()
{
    return "Noteahead PianoSynth";
}

QString internalDevicePortPrefix()
{
    return "Noteahead Internal Device";
}

double defaultSampleRate()
{
    return 48000.0;
}

namespace RackEffectType {
QString reverb()
{
    return "reverb";
}

QString compressor()
{
    return "compressor";
}

QString delay()
{
    return "delay";
}

QString chorus()
{
    return "chorus";
}

QString clipper()
{
    return "clipper";
}

QString panner()
{
    return "panner";
}

QString autoPanner()
{
    return "autopanner";
}

QString eq8BandParametric()
{
    return "eq8bandparametric";
}

QString allPassFilter()
{
    return "allpassfilter";
}

QString lufsMeter()
{
    return "lufsmeter";
}

QString dbtpMeter()
{
    return "dbtpmeter";
}
} // namespace RackEffectType

namespace NahdXml {

QString xmlKeyFileFormatVersion()
{
    return "fileFormatVersion";
}

QString xmlKeyApplicationName()
{
    return "applicationName";
}

QString xmlKeyApplicationVersion()
{
    return "applicationVersion";
}

QString xmlKeyCreatedDate()
{
    return "createdDate";
}

QString xmlKeyTypeName()
{
    return "typeName";
}

QString xmlKeyBeatsPerMinute()
{
    return "beatsPerMinute";
}

QString xmlKeyBankEnabled()
{
    return "bankEnabled";
}

QString xmlKeyBankLsb()
{
    return "bankLsb";
}

QString xmlKeyBankMsb()
{
    return "bankMsb";
}

QString xmlKeyBankByteOrderSwapped()
{
    return "bankByteOrderSwapped";
}

QString xmlKeyChannel()
{
    return "channel";
}

QString xmlKeyGroup()
{
    return "group";
}

QString xmlKeyColumn()
{
    return "Column";
}

QString xmlKeyColumns()
{
    return "Columns";
}

QString xmlKeyColumnCount()
{
    return "columnCount";
}

QString xmlKeyColumnSettings()
{
    return "ColumnSettings";
}

QString xmlKeyChordNote1Offset()
{
    return "chordNote1Offset";
}

QString xmlKeyChordNote1Velocity()
{
    return "chordNote1Velocity";
}

QString xmlKeyChordNote1Delay()
{
    return "chordNote1Delay";
}

QString xmlKeyChordNote2Offset()
{
    return "chordNote2Offset";
}

QString xmlKeyChordNote2Velocity()
{
    return "chordNote2Velocity";
}

QString xmlKeyChordNote2Delay()
{
    return "chordNote2Delay";
}

QString xmlKeyChordNote3Offset()
{
    return "chordNote3Offset";
}

QString xmlKeyChordNote3Velocity()
{
    return "chordNote3Velocity";
}

QString xmlKeyChordNote3Delay()
{
    return "chordNote3Delay";
}

QString xmlKeyArpeggiatorEnabled()
{
    return "arpeggiatorEnabled";
}

QString xmlKeyArpeggiatorPattern()
{
    return "arpeggiatorPattern";
}

QString xmlKeyArpeggiatorEventsPerBeat()
{
    return "arpeggiatorEventsPerBeat";
}

QString xmlKeyController()
{
    return "controller";
}

QString xmlKeyEnabled()
{
    return "enabled";
}

QString xmlKeyCutoff()
{
    return "cutoff";
}

QString xmlKeyDelay()
{
    return "delay";
}

QString xmlKeyMidiDelayEnabled()
{
    return "midiDelayEnabled";
}

QString xmlKeyMidiDelayLines()
{
    return "midiDelayLines";
}

QString xmlKeyMidiDelayFeedback()
{
    return "midiDelayFeedback";
}

QString xmlKeyMidiDelayMaxRepetitions()
{
    return "midiDelayMaxRepetitions";
}

QString xmlKeyTranspose()
{
    return "transpose";
}

QString xmlKeyDrumTrack()
{
    return "drumTrack";
}

QString xmlKeyVelocityJitter()
{
    return "velocityJitter";
}

QString xmlKeyVelocityKeyTrack()
{
    return "velocityKeyTrack";
}

QString xmlKeyVelocityKeyTrackOffset()
{
    return "velocityKeyTrackOffset";
}

QString xmlKeyAutoNoteOffOffset()
{
    return "autoNoteOffOffset";
}

QString xmlKeyIndex()
{
    return "index";
}

QString xmlKeyInstrument()
{
    return "Instrument";
}

QString xmlKeyInstrumentSettings()
{
    return "InstrumentSettings";
}

QString xmlKeySendMidiClock()
{
    return "sendMidiClock";
}

QString xmlKeySendTransport()
{
    return "sendTransport";
}

QString xmlKeyPatternAttr()
{
    return "pattern";
}

QString xmlKeyPlayOrder()
{
    return "PlayOrder";
}

QString xmlKeyPosition()
{
    return "Position";
}

QString xmlKeySkipped()
{
    return "skipped";
}

QString xmlKeyLength()
{
    return "length";
}

QString xmlKeyLine()
{
    return "Line";
}

QString xmlKeyLineEvent()
{
    return "LineEvent";
}

QString xmlKeyLines()
{
    return "Lines";
}

QString xmlKeyLineCount()
{
    return "lineCount";
}

QString xmlKeyLinesPerBeat()
{
    return "linesPerBeat";
}

QString xmlKeyLookahead()
{
    return "lookahead";
}

QString xmlKeyParameters()
{
    return "Parameters";
}

QString xmlKeyMidiCcSetting()

{
    return "MidiCcSetting";
}

QString xmlKeyMidiSideChain()
{
    return "MidiSideChain";
}

QString xmlKeyMixer()
{
    return "Mixer";
}

QString xmlKeyMasterEffects()
{
    return "MasterEffects";
}

QString xmlKeyInsertEffects()
{
    return "InsertEffects";
}

QString xmlKeySendEffects()
{
    return "SendEffects";
}

QString xmlKeyEffect()
{
    return "Effect";
}

QString xmlKeySend()
{
    return "Send";
}

QString xmlKeyDeviceSlot()
{
    return "deviceSlot";
}

QString xmlKeyEffectSlot()
{
    return "effectSlot";
}

QString xmlKeyAudioRecorder()
{
    return "AudioRecorder";
}

QString xmlKeyLatestRecordingFilePath()
{
    return "latestRecordingFilePath";
}

QString xmlKeyLatestRecordingStartTick()
{
    return "latestRecordingStartTick";
}

QString xmlKeyLatestRecordingEndTick()
{
    return "latestRecordingEndTick";
}

QString xmlKeyAutomation()
{
    return "Automation";
}

QString xmlKeyMidiCcAutomation()
{
    return "MidiCcAutomation";
}

QString xmlKeyPitchBendAutomation()
{
    return "PitchBendAutomation";
}

QString xmlKeyInterpolation()
{
    return "Interpolation";
}

QString xmlKeyLocation()
{
    return "Location";
}

QString xmlKeyId()
{
    return "id";
}

QString xmlKeyLine0()
{
    return "line0";
}

QString xmlKeyLine1()
{
    return "line1";
}

QString xmlKeyValue0()
{
    return "value0";
}

QString xmlKeyValue1()
{
    return "value1";
}

QString xmlKeyComment()
{
    return "comment";
}

QString xmlKeyModulation()
{
    return "Modulation";
}

QString xmlKeyCycles()
{
    return "cycles";
}

QString xmlKeyAmplitude()
{
    return "amplitude";
}

QString xmlKeyInverted()
{
    return "inverted";
}

QString xmlKeyOffset()
{
    return "offset";
}

QString xmlKeyColumnAttr()
{
    return "column";
}

QString xmlKeyColumnMuted()
{
    return "ColumMuted";
}

QString xmlKeyColumnSoloed()
{
    return "ColumSoloed";
}

QString xmlKeyColumnVelocityScale()
{
    return "ColumnVelocityScale";
}

QString xmlKeyTrackAttr()
{
    return "track";
}

QString xmlKeyTrackMuted()
{
    return "TrackMuted";
}

QString xmlKeyTrackSoloed()
{
    return "TrackSoloed";
}

QString xmlKeyTrackVelocityScale()
{
    return "TrackVelocityScale";
}

QString xmlKeyName()
{
    return "name";
}

QString xmlKeyNote()
{
    return "note";
}

QString xmlKeyNoteOn()
{
    return "noteOn";
}

QString xmlKeyNoteOff()
{
    return "noteOff";
}

QString xmlKeyNoteData()
{
    return "NoteData";
}

QString xmlKeySlot()
{
    return "slot";
}

QString xmlKeyPan()
{
    return "pan";
}

QString xmlKeyReverbSend()
{
    return "reverbSend";
}

QString xmlKeyReverbSend1()
{
    return "reverbSend1";
}

QString xmlKeyReverbSend2()
{
    return "reverbSend2";
}

QString xmlKeyReverbSend3()
{
    return "reverbSend3";
}

QString xmlKeyReverbSend4()
{
    return "reverbSend4";
}

QString xmlKeyPatch()
{
    return "patch";
}

QString xmlKeyPattern()
{
    return "Pattern";
}

QString xmlKeyPatterns()
{
    return "Patterns";
}

QString xmlKeyPortName()
{
    return "portName";
}

QString xmlKeyRelease()
{
    return "release";
}

QString xmlKeyReleaseValue()
{
    return "releaseValue";
}

QString xmlKeyEventsPerBeat()
{
    return "eventsPerBeat";
}

QString xmlKeyLineOffset()
{
    return "lineOffset";
}

QString xmlKeyTrack()
{
    return "Track";
}

QString xmlKeyTracks()
{
    return "Tracks";
}

QString xmlKeyTrackCount()
{
    return "trackCount";
}

QString xmlKeyTargetValue()
{
    return "targetValue";
}

QString xmlKeyType()
{
    return "type";
}

QString xmlKeyValue()
{
    return "value";
}

QString xmlKeyVelocity()
{
    return "velocity";
}

QString xmlKeyVolume()
{
    return "volume";
}

QString xmlKeyGain()
{
    return "gain";
}

QString xmlKeyProject()
{
    return "Project";
}

QString xmlKeySettings()
{
    return "Settings";
}

QString xmlKeySong()
{
    return "Song";
}

QString xmlKeySourceColumn()
{
    return "sourceColumn";
}

QString xmlKeySourceTrack()
{
    return "sourceTrack";
}

QString xmlKeySideChain()
{
    return "SideChain";
}

QString xmlKeySideChainTarget()
{
    return "Target";
}

QString xmlKeySideChainSettings()
{
    return "SideChainSettings";
}

QString xmlKeyDevices()
{
    return "Devices";
}

QString xmlKeyDevice()
{
    return "Device";
}

QString xmlKeyCategory()
{
    return "category";
}

QString xmlKeyParameter()
{
    return "Parameter";
}

QString xmlKeyParameterValueType()
{
    return "valueType";
}

QString xmlKeyMin()
{
    return "min";
}

QString xmlKeyMax()
{
    return "max";
}

QString xmlKeyDefault()
{
    return "default";
}

QString xmlKeyScale()
{
    return "scale";
}

QString xmlKeySampler()
{
    return "Sampler";
}

QString xmlKeySynth()
{
    return "Synth";
}

QString xmlKeyOscillator()
{
    return "Oscillator";
}

QString xmlKeyWaveform()
{
    return "waveform";
}

QString xmlKeyLevel()
{
    return "level";
}

QString xmlKeyShape()
{
    return "shape";
}

QString xmlKeyPitch()
{
    return "pitch";
}

QString xmlKeySync()
{
    return "sync";
}

QString xmlKeyOctave()
{
    return "octave";
}

QString xmlKeyMultiType()
{
    return "multiType";
}

QString xmlKeyMultiKeyTrack()
{
    return "multiKeyTrack";
}

QString xmlKeyMode()
{
    return "mode";
}

QString xmlKeyRate()
{
    return "rate";
}

QString xmlKeyDepth()
{
    return "depth";
}

QString xmlKeyWidth()
{
    return "width";
}

QString xmlKeyMix()
{
    return "mix";
}

QString xmlKeyThreshold()
{
    return "threshold";
}

QString xmlKeyRatio()
{
    return "ratio";
}

QString xmlKeyKnee()
{
    return "knee";
}

QString xmlKeyMakeup()
{
    return "makeup";
}

QString xmlKeySideChainSourceDevice()
{
    return "sidechainSourceDevice";
}

QString xmlKeySideChainLpf()
{
    return "sideChainLpf";
}

QString xmlKeySize()
{
    return "size";
}

QString xmlKeyDamping()
{
    return "damping";
}

QString xmlKeyPreDelay()
{
    return "preDelay";
}

QString xmlKeyPitchBendRange()
{
    return "pitchBendRange";
}

QString xmlKeyResonance()
{
    return "resonance";
}

QString xmlKeyKeyTrack()
{
    return "keyTrack";
}

QString xmlKeyAttack()
{
    return "attack";
}

QString xmlKeyDecay()
{
    return "decay";
}

QString xmlKeySustain()
{
    return "sustain";
}

QString xmlKeyReleaseTime()
{
    return "release";
}

QString xmlKeyIntensity()
{
    return "intensity";
}

QString xmlKeyTarget()
{
    return "target";
}

QString xmlKeyDelayType()
{
    return "delayType";
}

QString xmlKeyDelayTime()
{
    return "delayTime";
}

QString xmlKeyDelayFeedback()
{
    return "delayFeedback";
}

QString xmlKeyDelayDepth()
{
    return "delayDepth";
}

QString xmlKeyDelayMix()
{
    return "delayMix";
}

QString xmlKeyDelaySync()
{
    return "delaySync";
}

QString xmlKeyDelaySyncDivision()
{
    return "delaySyncDivision";
}

QString xmlKeyDelayFeedbackLpf()
{
    return "delayFeedbackLpf";
}

QString xmlKeyDelayFeedbackHpf()
{
    return "delayFeedbackHpf";
}

QString xmlKeyOscillatorDrift()
{
    return "oscillatorDrift";
}

QString xmlKeyCrossModDepth()
{
    return "crossModDepth";
}

QString xmlKeyBandType(size_t bandIndex)
{
    return QString { "band%1Type" }.arg(bandIndex + 1);
}

QString xmlKeyBandFreq(size_t bandIndex)
{
    return QString { "band%1Freq" }.arg(bandIndex + 1);
}

QString xmlKeyBandGain(size_t bandIndex)
{
    return QString { "band%1Gain" }.arg(bandIndex + 1);
}

QString xmlKeyBandQ(size_t bandIndex)
{
    return QString { "band%1Q" }.arg(bandIndex + 1);
}

QString xmlKeyVco1Waveform()
{
    return "vco1Waveform";
}

QString xmlKeyVco1Octave()
{
    return "vco1Octave";
}

QString xmlKeyVco1Pitch()
{
    return "vco1Pitch";
}

QString xmlKeyVco1Shape()
{
    return "vco1Shape";
}

QString xmlKeyVco1Sync()
{
    return "vco1Sync";
}

QString xmlKeyVco2Waveform()
{
    return "vco2Waveform";
}

QString xmlKeyVco2Octave()
{
    return "vco2Octave";
}

QString xmlKeyVco2Pitch()
{
    return "vco2Pitch";
}

QString xmlKeyVco2Shape()
{
    return "vco2Shape";
}

QString xmlKeyVco2Sync()
{
    return "vco2Sync";
}

QString xmlKeyVco3Waveform()
{
    return "vco3Waveform";
}

QString xmlKeyVco3Octave()
{
    return "vco3Octave";
}

QString xmlKeyVco3Pitch()
{
    return "vco3Pitch";
}

QString xmlKeyVco3Shape()
{
    return "vco3Shape";
}

QString xmlKeyVco3Sync()
{
    return "vco3Sync";
}

QString xmlKeyOsc1Pos()
{
    return "osc1Pos";
}

QString xmlKeyOsc1Octave()
{
    return "osc1Octave";
}

QString xmlKeyOsc1Pitch()
{
    return "osc1Pitch";
}

QString xmlKeyOsc1Level()
{
    return "osc1Level";
}

QString xmlKeyOsc2Pos()
{
    return "osc2Pos";
}

QString xmlKeyOsc2Octave()
{
    return "osc2Octave";
}

QString xmlKeyOsc2Pitch()
{
    return "osc2Pitch";
}

QString xmlKeyOsc2Level()
{
    return "osc2Level";
}

QString xmlKeyNoiseLevel()
{
    return "noiseLevel";
}

QString xmlKeyLpfCutoff()
{
    return "lpfCutoff";
}

QString xmlKeyLpfResonance()
{
    return "lpfResonance";
}

QString xmlKeyHpfCutoff()
{
    return "hpfCutoff";
}

QString xmlKeyAmpAttack()
{
    return "ampAttack";
}

QString xmlKeyAmpDecay()
{
    return "ampDecay";
}

QString xmlKeyAmpSustain()
{
    return "ampSustain";
}

QString xmlKeyAmpRelease()
{
    return "ampRelease";
}

QString xmlKeyAmpVelocitySensitivity()
{
    return "ampVelocitySensitivity";
}

QString xmlKeyModAttack()
{
    return "modAttack";
}

QString xmlKeyModDecay()
{
    return "modDecay";
}

QString xmlKeyModIntensity()
{
    return "modIntensity";
}

QString xmlKeyModTarget()
{
    return "modTarget";
}

QString xmlKeyLfoWaveform()
{
    return "lfoWaveform";
}

QString xmlKeyLfoMode()
{
    return "lfoMode";
}

QString xmlKeyLfoRate()
{
    return "lfoRate";
}

QString xmlKeyLfoIntensity()
{
    return "lfoIntensity";
}

QString xmlKeyLfoTarget()
{
    return "lfoTarget";
}

QString xmlKeyLfo2Waveform()
{
    return "lfo2Waveform";
}

QString xmlKeyLfo2Mode()
{
    return "lfo2Mode";
}

QString xmlKeyLfo2Rate()
{
    return "lfo2Rate";
}

QString xmlKeyLfo2Intensity()
{
    return "lfo2Intensity";
}

QString xmlKeyLfo2Target()
{
    return "lfo2Target";
}

QString xmlKeyWavetableIndex()
{
    return "wavetableIndex";
}

QString xmlKeyVoiceMode()
{
    return "voiceMode";
}

QString xmlKeyVoiceDepth()
{
    return "voiceDepth";
}

QString xmlKeyPortamento()
{
    return "portamento";
}

QString xmlKeyPanSpread()
{
    return "panSpread";
}

QString xmlKeyMultiMode()
{
    return "multiMode";
}

QString xmlKeyMultiShape()
{
    return "multiShape";
}

QString xmlKeyMultiLevel()
{
    return "multiLevel";
}

QString xmlKeyMixLevel1()
{
    return "mixLevel1";
}

QString xmlKeyMixLevel2()
{
    return "mixLevel2";
}

QString xmlKeyMixLevel3()
{
    return "mixLevel3";
}

QString xmlKeyUserPresets()
{
    return "UserPresets";
}

QString xmlKeyPreset()
{
    return "Preset";
}

QString xmlKeyTypeId()
{
    return "typeId";
}

QString xmlKeyBassSynth()
{
    return "BassSynth";
}

QString xmlKeySubLevel()
{
    return "subLevel";
}

QString xmlKeySubOctave()
{
    return "subOctave";
}

QString xmlKeyEnvMod()
{
    return "envMod";
}

QString xmlKeyAccent()
{
    return "accent";
}

QString xmlKeySlide()
{
    return "slide";
}

QString xmlKeyDistDrive()
{
    return "distDrive";
}

QString xmlKeyDistTone()
{
    return "distTone";
}

QString xmlKeyDistLevel()
{
    return "distLevel";
}

QString xmlKeySample()
{
    return "Sample";
}

QString xmlKeySamples()
{
    return "Samples";
}

QString xmlKeySamplePath()
{
    return "path";
}

QString xmlKeyChannelMode()
{
    return "channelMode";
}

QString xmlKeyEmbedWaveData()
{
    return "embedWaveData";
}

QString xmlKeyStartOffset()
{
    return "startOffset";
}

QString xmlKeyData()
{
    return "Data";
}

QString xmlKeyDrumSynth()
{
    return "DrumSynth";
}

QString xmlKeyPad()
{
    return "Pad";
}

QString xmlKeyTune()
{
    return "tune";
}

QString xmlKeyClickTune()
{
    return "clickTune";
}

QString xmlKeySnappy()
{
    return "snappy";
}

QString xmlKeyTone()
{
    return "tone";
}

QString xmlKeyPitchDepth()
{
    return "pitchDepth";
}

QString xmlKeyPitchDecay()
{
    return "pitchDecay";
}

QString xmlKeyAllPassFilterFrequency()
{
    return "allPassFilterFrequency";
}

QString xmlKeyAllPassFilterQ()
{
    return "allPassFilterQ";
}

QString xmlKeyAllPassFilterStages()
{
    return "allPassFilterStages";
}

QString xmlKeyLufsMeter()
{
    return "lufsMeter";
}

QString xmlKeyDbTpMeter()
{
    return "dbtpMeter";
}

QString embeddedDataPathPrefix()
{
    return "nahd://";
}

QString xmlValueFalse()
{
    return "false";
}

QString xmlValueTrue()
{
    return "true";
}

QString xmlValueInt()
{
    return "int";
}

QString xmlValueBool()
{
    return "bool";
}

QString xmlValueFloat()
{
    return "float";
}

QString xmlValueSineWave()
{
    return "SineWave";
}

QString xmlValueRandom()
{
    return "Random";
}

QString xmlValueSamplers()
{
    return "Samplers";
}

QString xmlValueSynths()
{
    return "Synths";
}

QString xmlKeyBrightness()
{
    return "brightness";
}

QString xmlKeyInharmonicity()
{
    return "inharmonicity";
}

QString xmlKeyHardness()
{
    return "hardness";
}

QString xmlKeyStringDetune()
{
    return "stringDetune";
}

} // namespace NahdXml

} // namespace noteahead::Constants
