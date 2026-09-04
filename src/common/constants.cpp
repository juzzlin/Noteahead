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

#include <map>

namespace noteahead::Constants {

QString applicationName()
{
    return "Noteahead";
}

QString exampleSongPath()
{
    return ":/examples/Example.nahd";
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

QString effectSettingsExtension()
{
    return ".nahdeff";
}

QString effectRackSettingsExtension()
{
    return ".nahdrack";
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

quint64 defaultVisibleUnitCount()
{
    return 6;
}

quint64 minVisibleUnitCount()
{
    return 2;
}

quint64 minTrackCount()
{
    return 6;
}

size_t deviceRackSize()
{
    return 16;
}

size_t effectRackSize()
{
    return 16;
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

QString pianoSynthV2DeviceName()
{
    return "Noteahead PianoSynth V2";
}

QString pianoSynthV3DeviceName()
{
    return "Noteahead PianoSynth V3";
}

QString kick808DeviceName()
{
    return "Noteahead Kick808";
}

QString subMixerDeviceName()
{
    return "Noteahead SubMixer";
}

QString stringVoiceDeviceName()
{
    return "Noteahead String & Voice";
}

QString stringVoiceV2DeviceName()
{
    return "Noteahead String & Voice V2";
}

QString stringEnsembleDeviceName()
{
    return "Noteahead String Ensemble";
}

QString speechDeviceName()
{
    return "Noteahead Speech";
}

QString internalDevicePortPrefix()
{
    return "Noteahead Internal Device";
}

int audioCallbackRealTimePriority()
{
    // Modest on purpose: high enough to beat ordinary threads, low enough not to fight the rest of
    // the system. RtAudio otherwise defaults this to 1, the lowest real-time priority there is,
    // which leaves no room for the workers to sit below it.
    return 10;
}

int playbackScheduleLookaheadMs()
{
    // Two of the largest buffers a backend is likely to hand over, which covers a server that wakes
    // once per 2048 frames as well as one that wakes per buffer.
    return 40;
}

QString pulseLatencyEnvironmentVariable()
{
    return "PULSE_LATENCY_MSEC";
}

int pulseLatencyBufferCount()
{
    return 2;
}

double defaultSampleRate()
{
    return 48000.0;
}

namespace Language {

QStringList supportedLanguages()
{
    return { "fi", "de", "es", "fr", "it", "nl", "pl", "pt_BR", "zh_CN" };
}

QString nativeLanguageName(const QString & language)
{
    static const std::map<QString, QString> nativeNames {
        { "de", QString::fromUtf8("Deutsch") },
        { "en", QString::fromUtf8("English") },
        { "es", QString::fromUtf8("Espa\u00f1ol") },
        { "fi", QString::fromUtf8("Suomi") },
        { "fr", QString::fromUtf8("Fran\u00e7ais") },
        { "it", QString::fromUtf8("Italiano") },
        { "nl", QString::fromUtf8("Nederlands") },
        { "pl", QString::fromUtf8("Polski") },
        { "pt_BR", QString::fromUtf8("Portugu\u00eas (Brasil)") },
        { "zh_CN", QString::fromUtf8("\u7b80\u4f53\u4e2d\u6587") }
    };
    if (const auto it = nativeNames.find(language); it != nativeNames.end()) {
        return it->second;
    }
    return language;
}

QString translationsResourceBase()
{
    return ":/translations/noteahead_";
}

QString sourceLanguage()
{
    return "en";
}

} // namespace Language

namespace RackEffectType {
QString reverb()
{
    return "reverb";
}

QString compressor()
{
    return "compressor";
}

QString multibandCompressor()
{
    return "multibandcompressor";
}

QString delay()
{
    return "delay";
}

QString monitor()
{
    return "monitor";
}

QString gain()
{
    return "gain";
}

QString dimension()
{
    return "dimension";
}

QString earlyReflections()
{
    return "earlyReflections";
}

QString chorus()
{
    return "chorus";
}

QString clipper()
{
    return "clipper";
}

QString drive()
{
    return "drive";
}

QString analogFuzz()
{
    return "analogFuzz";
}

QString bassGrinder()
{
    return "bassGrinder";
}

QString saturator()
{
    return "saturator";
}

QString tubeStage()
{
    return "tubeStage";
}

QString waveDesigner()
{
    return "waveDesigner";
}

QString stereoEnhancer()
{
    return "stereoEnhancer";
}

QString stereoExciter()
{
    return "stereoExciter";
}

QString stereoWidener()
{
    return "stereoWidener";
}

QString stereoFieldMeter()
{
    return "stereoFieldMeter";
}

QString limiter()
{
    return "limiter";
}

QString endless()
{
    return "endless";
}

QString panner()
{
    return "panner";
}

QString autoPanner()
{
    return "autopanner";
}

QString autoDucker()
{
    return "autoducker";
}

QString autoFilter()
{
    return "autofilter";
}

QString phaser()
{
    return "phaser";
}

QString eq8BandParametric()
{
    return "eq8bandparametric";
}

QString vintagePassiveEq()
{
    return "vintagepassiveeq";
}

QString airBandEq()
{
    return "airbandeq";
}

QString simpleEq()
{
    return "simpleeq";
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

QString rta()
{
    return "rta";
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

QString xmlKeyColumnIndices()
{
    return "columnIndices";
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

QString xmlKeyAutoNoteOffSyncEnabled()
{
    return "autoNoteOffSyncEnabled";
}

QString xmlKeyAutoNoteOffSyncDenominator()
{
    return "autoNoteOffSyncDenominator";
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

QString xmlKeyCurve()
{
    return "curve";
}

QString xmlKeyComment()
{
    return "comment";
}

QString xmlKeyComposer()
{
    return "Composer";
}

QString xmlKeyExportTags()
{
    return "ExportTags";
}

QString xmlKeyNotes()
{
    return "Notes";
}

QString xmlKeyMetadata()
{
    return "Metadata";
}

QString xmlKeyRenderSettings()
{
    return "RenderSettings";
}

QString xmlKeySongSettings()
{
    // Not plain "Settings": that name is already taken by the element inside Devices and EffectRack,
    // and Song's deserialization dispatches on element names alone.
    return "SongSettings";
}

QString xmlKeyFormat()
{
    return "format";
}

QString xmlKeySampleRate()
{
    return "sampleRate";
}

QString xmlKeyBitDepth()
{
    return "bitDepth";
}

QString xmlKeyOversampleFactor()
{
    return "oversampleFactor";
}

QString xmlKeyNormalizeEnabled()
{
    return "normalizeEnabled";
}

QString xmlKeyNormalizeLevelTenthsDb()
{
    return "normalizeLevelTenthsDb";
}

QString xmlKeyTrimEnabled()
{
    return "trimEnabled";
}

QString xmlKeyTrimMinutes()
{
    return "trimMinutes";
}

QString xmlKeyTrimSeconds()
{
    return "trimSeconds";
}

QString xmlKeyFadeOutEnabled()
{
    return "fadeOutEnabled";
}

QString xmlKeyFadeOutSeconds()
{
    return "fadeOutSeconds";
}

QString xmlKeyFadeOutTenths()
{
    return "fadeOutTenths";
}

QString xmlKeySilenceEnabled()
{
    return "silenceEnabled";
}

QString xmlKeySilenceSeconds()
{
    return "silenceSeconds";
}

QString xmlKeySilenceTenths()
{
    return "silenceTenths";
}

QString xmlKeyFastRender()
{
    return "fastRender";
}

QString xmlKeyAnalyzeEnabled()
{
    return "analyzeEnabled";
}

QString xmlKeyTags()
{
    return "Tags";
}

QString xmlKeyTag()
{
    return "Tag";
}

QString xmlKeyTitle()
{
    return "Title";
}

QString xmlKeyArtist()
{
    return "Artist";
}

QString xmlKeyAlbum()
{
    return "Album";
}

QString xmlKeyDate()
{
    return "Date";
}

QString xmlKeyGenre()
{
    return "Genre";
}

QString xmlKeyTrackNumber()
{
    return "TrackNumber";
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

QString xmlKeyNone()
{
    return "none";
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

QString xmlKeyMembers()
{
    return "Members";
}

QString xmlKeyMember()
{
    return "Member";
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

QString xmlKeyFader()
{
    return "fader";
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

QString xmlKeyStereoMode()
{
    return "stereoMode";
}

QString xmlKeyStereoPhase()
{
    return "stereoPhase";
}

QString xmlKeyPhrase()
{
    return "phrase";
}

QString xmlKeyGlide()
{
    return "glide";
}

QString xmlKeyFormantShift()
{
    return "formantShift";
}

QString xmlKeyBreathiness()
{
    return "breathiness";
}

QString xmlKeyConsonantLevel()
{
    return "consonantLevel";
}

QString xmlKeySibilance()
{
    return "sibilance";
}

QString xmlKeyVoiceType()
{
    return "voiceType";
}

QString xmlKeyIntonation()
{
    return "intonation";
}

QString xmlKeyTriggerMode()
{
    return "triggerMode";
}

QString xmlKeySyncMode()
{
    return "syncMode";
}

QString xmlKeySyncLength()
{
    return "syncLength";
}

QString xmlKeySyncDivision()
{
    return "syncDivision";
}

QString xmlKeyRate()
{
    return "rate";
}

QString xmlKeyDepth()
{
    return "depth";
}

QString xmlKeyDetune()
{
    return "detune";
}

QString xmlKeyFeedback()
{
    return "feedback";
}

QString xmlKeyRateDivider()
{
    return "rateDivider";
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

QString xmlKeyCeiling()
{
    return "ceiling";
}

QString xmlKeyBoost()
{
    return "boost";
}

QString xmlKeyFreeze()
{
    return "freeze";
}

QString xmlKeyGated()
{
    return "gated";
}

QString xmlKeyHold()
{
    return "hold";
}

QString xmlKeyDrive()
{
    return "drive";
}

QString xmlKeyDriveDb()
{
    return "driveDb";
}

QString xmlKeyFuzz()
{
    return "fuzz";
}

QString xmlKeyBias()
{
    return "bias";
}

QString xmlKeyBlend()
{
    return "blend";
}

QString xmlKeySplitFreq()
{
    return "splitFreq";
}

QString xmlKeyColor()
{
    return "color";
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

QString xmlKeyDiffusion()
{
    return "diffusion";
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

QString xmlKeyFilterType()
{
    return "filterType";
}

QString xmlKeyFilterSlope()
{
    return "filterSlope";
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

QString xmlKeyBandThreshold(size_t bandIndex)
{
    return QString { "band%1Threshold" }.arg(bandIndex + 1);
}

QString xmlKeyBandRatio(size_t bandIndex)
{
    return QString { "band%1Ratio" }.arg(bandIndex + 1);
}

QString xmlKeyBandKnee(size_t bandIndex)
{
    return QString { "band%1Knee" }.arg(bandIndex + 1);
}

QString xmlKeyBandAttack(size_t bandIndex)
{
    return QString { "band%1Attack" }.arg(bandIndex + 1);
}

QString xmlKeyBandRelease(size_t bandIndex)
{
    return QString { "band%1Release" }.arg(bandIndex + 1);
}

QString xmlKeyBandMakeup(size_t bandIndex)
{
    return QString { "band%1Makeup" }.arg(bandIndex + 1);
}

QString xmlKeyBandBypass(size_t bandIndex)
{
    return QString { "band%1Bypass" }.arg(bandIndex + 1);
}

QString xmlKeyBandSolo(size_t bandIndex)
{
    return QString { "band%1Solo" }.arg(bandIndex + 1);
}

QString xmlKeyBandWidth(size_t bandIndex)
{
    return QString { "band%1Width" }.arg(bandIndex + 1);
}

QString xmlKeyCrossoverFreq(size_t crossoverIndex)
{
    return QString { "crossover%1Freq" }.arg(crossoverIndex + 1);
}

QString xmlKeyAmount()
{
    return "amount";
}

QString xmlKeyBassGain()
{
    return "bassGain";
}

QString xmlKeyBassFreq()
{
    return "bassFreq";
}

QString xmlKeyMidGain()
{
    return "midGain";
}

QString xmlKeyMidFreq()
{
    return "midFreq";
}

QString xmlKeyMidQ()
{
    return "midQ";
}

QString xmlKeyHighGain()
{
    return "highGain";
}

QString xmlKeyHighFreq()
{
    return "highFreq";
}

QString xmlKeyPeak()
{
    return "peak";
}

QString xmlKeyZeroFill()
{
    return "zeroFill";
}

QString xmlKeyTimbre()
{
    return "timbre";
}

QString xmlKeyHarmonics()
{
    return "harmonics";
}

QString xmlKeySolo()
{
    return "solo";
}

QString xmlKeySpread()
{
    return "spread";
}

QString xmlKeyMonoBass()
{
    return "monoBass";
}

QString xmlKeyMonoFreq()
{
    return "monoFreq";
}

QString xmlKeyLowFreq()
{
    return "lowFreq";
}

QString xmlKeyLowBoost()
{
    return "lowBoost";
}

QString xmlKeyLowAtten()
{
    return "lowAtten";
}

QString xmlKeyHighBoostFreq()
{
    return "highBoostFreq";
}

QString xmlKeyHighBoost()
{
    return "highBoost";
}

QString xmlKeyBandwidth()
{
    return "bandwidth";
}

QString xmlKeyHighAttenFreq()
{
    return "highAttenFreq";
}

QString xmlKeyHighAtten()
{
    return "highAtten";
}

QString xmlKeyAirFreq()
{
    return "airFreq";
}

QString xmlKeyAirGain()
{
    return "airGain";
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

QString xmlKeyVco1Roundness()
{
    return "vco1Roundness";
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

QString xmlKeyVco2Roundness()
{
    return "vco2Roundness";
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

QString xmlKeyVco3Roundness()
{
    return "vco3Roundness";
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

QString xmlKeyAmpCurve()
{
    return "ampCurve";
}

QString xmlKeyModAttack()
{
    return "modAttack";
}

QString xmlKeyModDecay()
{
    return "modDecay";
}

QString xmlKeyModSustain()
{
    return "modSustain";
}

QString xmlKeyModIntensity()
{
    return "modIntensity";
}

QString xmlKeyModTarget()
{
    return "modTarget";
}

QString xmlKeyModCurve()
{
    return "modCurve";
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

QString xmlKeyVoice()
{
    return "Voice";
}

QString xmlKeyVoices()
{
    return "Voices";
}

QString xmlKeySamplePath()
{
    return "path";
}

QString xmlKeyChannelMode()
{
    return "channelMode";
}

QString xmlKeyChromaticMode()
{
    return "chromaticMode";
}

QString xmlKeyEmbedWaveData()
{
    return "embedWaveData";
}

QString xmlKeyStartOffset()
{
    return "startOffset";
}

QString xmlKeyEndOffset()
{
    return "endOffset";
}

QString xmlKeyReverse()
{
    return "reverse";
}

QString xmlKeyLoop()
{
    return "loop";
}

QString xmlKeyLoopStart()
{
    return "loopStart";
}

QString xmlKeyChokeGroup()
{
    return "chokeGroup";
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

QString xmlKeyFrequency()
{
    return "frequency";
}

QString xmlKeyQ()
{
    return "q";
}

QString xmlKeyStages()
{
    return "stages";
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

QString xmlValueLinear()
{
    return "Linear";
}

QString xmlValueExponential()
{
    return "Exponential";
}

QString xmlValueLogarithmic()
{
    return "Logarithmic";
}

QString xmlValueEaseIn()
{
    return "EaseIn";
}

QString xmlValueEaseOut()
{
    return "EaseOut";
}

QString xmlValueEaseInOut()
{
    return "EaseInOut";
}

QString xmlValueSamplers()
{
    return "Samplers";
}

QString xmlValueMixers()
{
    return "Mixers";
}

QString xmlValueSynths()
{
    return "Synths";
}

QString xmlValueDrums()
{
    return "Drums";
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

QString xmlKeyStretch()
{
    return "stretch";
}

QString xmlKeyRichness()
{
    return "richness";
}

QString xmlKeyDoubleDecay()
{
    return "doubleDecay";
}

QString xmlKeyBandCount()
{
    return "bandCount";
}

QString xmlKeyDbRange()
{
    return "dbRange";
}

QString xmlKeyShowPinkNoise()
{
    return "showPinkNoise";
}

QString xmlKeyPinkNoiseLevel()
{
    return "pinkNoiseLevel";
}

QString xmlKeySpeed()
{
    return "speed";
}

QString xmlKeyZoom()
{
    return "zoom";
}

QString xmlKeyShowGuides()
{
    return "showGuides";
}

QString xmlKeyFftRate()
{
    return "fftRate";
}

QString xmlKeyStringsBalance()
{
    return "stringsBalance";
}

QString xmlKeyStringsLevel8()
{
    return "stringsLevel8";
}

QString xmlKeyVoiceBalance()
{
    return "voiceBalance";
}

QString xmlKeyVoiceUpperMale8()
{
    return "voiceUpperMale8";
}

QString xmlKeyStringsLevel4()
{
    return "stringsLevel4";
}

QString xmlKeyStringsUpper()
{
    return "stringsUpper";
}

QString xmlKeyStringsLower()
{
    return "stringsLower";
}

QString xmlKeyStringsTone()
{
    return "stringsTone";
}

QString xmlKeyStringsAttack()
{
    return "stringsAttack";
}

QString xmlKeyStringsRelease()
{
    return "stringsRelease";
}

QString xmlKeyVoiceMale8()
{
    return "voiceMale8";
}

QString xmlKeyVoiceMale4()
{
    return "voiceMale4";
}

QString xmlKeyVoiceFemale8()
{
    return "voiceFemale8";
}

QString xmlKeyVoiceFemale4()
{
    return "voiceFemale4";
}

QString xmlKeyVoiceAttack()
{
    return "voiceAttack";
}

QString xmlKeyVoiceRelease()
{
    return "voiceRelease";
}

QString xmlKeyVibratoRate()
{
    return "vibratoRate";
}

QString xmlKeyVibratoDepth()
{
    return "vibratoDepth";
}

QString xmlKeyVibratoDelay()
{
    return "vibratoDelay";
}

QString xmlKeyEnsembleEnabled()
{
    return "ensembleEnabled";
}

QString xmlKeyEnsembleMode()
{
    return "ensembleMode";
}

QString xmlKeyVocoderEnabled()
{
    return "vocoderEnabled";
}

QString xmlKeyVocoderSidechain()
{
    return "vocoderSidechain";
}

QString xmlKeyContrabass()
{
    return "contrabass";
}

QString xmlKeyCello()
{
    return "cello";
}

QString xmlKeyViola()
{
    return "viola";
}

QString xmlKeyViolin()
{
    return "violin";
}

QString xmlKeyTrumpet()
{
    return "trumpet";
}

QString xmlKeyHorn()
{
    return "horn";
}

QString xmlKeyVolumeBass()
{
    return "volumeBass";
}

QString xmlKeyPhaserEnabled()
{
    return "phaserEnabled";
}

QString xmlKeyPhaserColor()
{
    return "phaserColor";
}

QString xmlKeyPhaserRate()
{
    return "phaserRate";
}

QString xmlKeyVelocitySensitivity()
{
    return "velocitySensitivity";
}

QString xmlKeyFaderPosition()
{
    return "faderPosition";
}

QString xmlKeySendTap()
{
    return "sendTap";
}

} // namespace NahdXml

} // namespace noteahead::Constants
