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

#include "nahd_xml_keys.hpp"

#include "../../common/constants.hpp"

namespace noteahead {

NahdXmlKeys::NahdXmlKeys(QObject * parent)
  : QObject { parent }
  , m_airFreq { Constants::NahdXml::xmlKeyAirFreq() }
  , m_airGain { Constants::NahdXml::xmlKeyAirGain() }
  , m_amount { Constants::NahdXml::xmlKeyAmount() }
  , m_attack { Constants::NahdXml::xmlKeyAttack() }
  , m_autoGain { Constants::NahdXml::xmlKeyAutoGain() }
  , m_bandCount { Constants::NahdXml::xmlKeyBandCount() }
  , m_bandwidth { Constants::NahdXml::xmlKeyBandwidth() }
  , m_bassDecay { Constants::NahdXml::xmlKeyBassDecay() }
  , m_bassFreq { Constants::NahdXml::xmlKeyBassFreq() }
  , m_bassGain { Constants::NahdXml::xmlKeyBassGain() }
  , m_bias { Constants::NahdXml::xmlKeyBias() }
  , m_bitDepth { Constants::NahdXml::xmlKeyBitDepth() }
  , m_blend { Constants::NahdXml::xmlKeyBlend() }
  , m_boost { Constants::NahdXml::xmlKeyBoost() }
  , m_ceiling { Constants::NahdXml::xmlKeyCeiling() }
  , m_channelMode { Constants::NahdXml::xmlKeyChannelMode() }
  , m_color { Constants::NahdXml::xmlKeyColor() }
  , m_converters { Constants::NahdXml::xmlKeyConverters() }
  , m_cutoff { Constants::NahdXml::xmlKeyCutoff() }
  , m_damping { Constants::NahdXml::xmlKeyDamping() }
  , m_dbRange { Constants::NahdXml::xmlKeyDbRange() }
  , m_dcBlock { Constants::NahdXml::xmlKeyDcBlock() }
  , m_decay { Constants::NahdXml::xmlKeyDecay() }
  , m_delay { Constants::NahdXml::xmlKeyDelay() }
  , m_delaySyncDivision { Constants::NahdXml::xmlKeyDelaySyncDivision() }
  , m_delayTime { Constants::NahdXml::xmlKeyDelayTime() }
  , m_depth { Constants::NahdXml::xmlKeyDepth() }
  , m_detectorFilter { Constants::NahdXml::xmlKeyDetectorFilter() }
  , m_detectorSource { Constants::NahdXml::xmlKeyDetectorSource() }
  , m_detune { Constants::NahdXml::xmlKeyDetune() }
  , m_diffusion { Constants::NahdXml::xmlKeyDiffusion() }
  , m_drive { Constants::NahdXml::xmlKeyDrive() }
  , m_driveDb { Constants::NahdXml::xmlKeyDriveDb() }
  , m_dynamics { Constants::NahdXml::xmlKeyDynamics() }
  , m_earlyLate { Constants::NahdXml::xmlKeyEarlyLate() }
  , m_enhance { Constants::NahdXml::xmlKeyEnhance() }
  , m_envMod { Constants::NahdXml::xmlKeyEnvMod() }
  , m_environment { Constants::NahdXml::xmlKeyEnvironment() }
  , m_feedback { Constants::NahdXml::xmlKeyFeedback() }
  , m_fftRate { Constants::NahdXml::xmlKeyFftRate() }
  , m_filterSlope { Constants::NahdXml::xmlKeyFilterSlope() }
  , m_filterType { Constants::NahdXml::xmlKeyFilterType() }
  , m_freeze { Constants::NahdXml::xmlKeyFreeze() }
  , m_frequency { Constants::NahdXml::xmlKeyFrequency() }
  , m_fuzz { Constants::NahdXml::xmlKeyFuzz() }
  , m_gain { Constants::NahdXml::xmlKeyGain() }
  , m_gated { Constants::NahdXml::xmlKeyGated() }
  , m_harmonics { Constants::NahdXml::xmlKeyHarmonics() }
  , m_highAtten { Constants::NahdXml::xmlKeyHighAtten() }
  , m_highAttenFreq { Constants::NahdXml::xmlKeyHighAttenFreq() }
  , m_highBoost { Constants::NahdXml::xmlKeyHighBoost() }
  , m_highBoostFreq { Constants::NahdXml::xmlKeyHighBoostFreq() }
  , m_highFreq { Constants::NahdXml::xmlKeyHighFreq() }
  , m_highGain { Constants::NahdXml::xmlKeyHighGain() }
  , m_hold { Constants::NahdXml::xmlKeyHold() }
  , m_hpfCutoff { Constants::NahdXml::xmlKeyHpfCutoff() }
  , m_intensity { Constants::NahdXml::xmlKeyIntensity() }
  , m_invertLeft { Constants::NahdXml::xmlKeyInvertLeft() }
  , m_invertRight { Constants::NahdXml::xmlKeyInvertRight() }
  , m_knee { Constants::NahdXml::xmlKeyKnee() }
  , m_lfo2Intensity { Constants::NahdXml::xmlKeyLfo2Intensity() }
  , m_lfo2Mode { Constants::NahdXml::xmlKeyLfo2Mode() }
  , m_lfo2Rate { Constants::NahdXml::xmlKeyLfo2Rate() }
  , m_lfo2Waveform { Constants::NahdXml::xmlKeyLfo2Waveform() }
  , m_lfoIntensity { Constants::NahdXml::xmlKeyLfoIntensity() }
  , m_lfoMode { Constants::NahdXml::xmlKeyLfoMode() }
  , m_lfoRate { Constants::NahdXml::xmlKeyLfoRate() }
  , m_lfoWaveform { Constants::NahdXml::xmlKeyLfoWaveform() }
  , m_linearPhase { Constants::NahdXml::xmlKeyLinearPhase() }
  , m_listen { Constants::NahdXml::xmlKeyListen() }
  , m_lookahead { Constants::NahdXml::xmlKeyLookahead() }
  , m_lowAtten { Constants::NahdXml::xmlKeyLowAtten() }
  , m_lowBoost { Constants::NahdXml::xmlKeyLowBoost() }
  , m_lowFreq { Constants::NahdXml::xmlKeyLowFreq() }
  , m_lpfCutoff { Constants::NahdXml::xmlKeyLpfCutoff() }
  , m_makeup { Constants::NahdXml::xmlKeyMakeup() }
  , m_midDecay { Constants::NahdXml::xmlKeyMidDecay() }
  , m_midFreq { Constants::NahdXml::xmlKeyMidFreq() }
  , m_midGain { Constants::NahdXml::xmlKeyMidGain() }
  , m_midQ { Constants::NahdXml::xmlKeyMidQ() }
  , m_mix { Constants::NahdXml::xmlKeyMix() }
  , m_mode { Constants::NahdXml::xmlKeyMode() }
  , m_monoBass { Constants::NahdXml::xmlKeyMonoBass() }
  , m_monoBassFrequency { Constants::NahdXml::xmlKeyMonoBassFrequency() }
  , m_monoFreq { Constants::NahdXml::xmlKeyMonoFreq() }
  , m_peak { Constants::NahdXml::xmlKeyPeak() }
  , m_peakReduction { Constants::NahdXml::xmlKeyPeakReduction() }
  , m_pinkNoiseLevel { Constants::NahdXml::xmlKeyPinkNoiseLevel() }
  , m_preDelay { Constants::NahdXml::xmlKeyPreDelay() }
  , m_program { Constants::NahdXml::xmlKeyProgram() }
  , m_q { Constants::NahdXml::xmlKeyQ() }
  , m_range { Constants::NahdXml::xmlKeyRange() }
  , m_rate { Constants::NahdXml::xmlKeyRate() }
  , m_rateDivider { Constants::NahdXml::xmlKeyRateDivider() }
  , m_ratio { Constants::NahdXml::xmlKeyRatio() }
  , m_release { Constants::NahdXml::xmlKeyRelease() }
  , m_resonance { Constants::NahdXml::xmlKeyResonance() }
  , m_room { Constants::NahdXml::xmlKeyRoom() }
  , m_sampleRate { Constants::NahdXml::xmlKeySampleRate() }
  , m_shape { Constants::NahdXml::xmlKeyShape() }
  , m_showGuides { Constants::NahdXml::xmlKeyShowGuides() }
  , m_showPinkNoise { Constants::NahdXml::xmlKeyShowPinkNoise() }
  , m_sideChainLpf { Constants::NahdXml::xmlKeySideChainLpf() }
  , m_sideChainSourceDevice { Constants::NahdXml::xmlKeySideChainSourceDevice() }
  , m_size { Constants::NahdXml::xmlKeySize() }
  , m_solo { Constants::NahdXml::xmlKeySolo() }
  , m_speed { Constants::NahdXml::xmlKeySpeed() }
  , m_splitFreq { Constants::NahdXml::xmlKeySplitFreq() }
  , m_spread { Constants::NahdXml::xmlKeySpread() }
  , m_stages { Constants::NahdXml::xmlKeyStages() }
  , m_stereoMode { Constants::NahdXml::xmlKeyStereoMode() }
  , m_stereoPhase { Constants::NahdXml::xmlKeyStereoPhase() }
  , m_sustain { Constants::NahdXml::xmlKeySustain() }
  , m_sync { Constants::NahdXml::xmlKeySync() }
  , m_threshold { Constants::NahdXml::xmlKeyThreshold() }
  , m_timbre { Constants::NahdXml::xmlKeyTimbre() }
  , m_tone { Constants::NahdXml::xmlKeyTone() }
  , m_trebleDecay { Constants::NahdXml::xmlKeyTrebleDecay() }
  , m_tune { Constants::NahdXml::xmlKeyTune() }
  , m_waveform { Constants::NahdXml::xmlKeyWaveform() }
  , m_width { Constants::NahdXml::xmlKeyWidth() }
  , m_zeroFill { Constants::NahdXml::xmlKeyZeroFill() }
  , m_zoom { Constants::NahdXml::xmlKeyZoom() }
{
}

QString NahdXmlKeys::bandAttack(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandAttack(index);
}

QString NahdXmlKeys::bandBypass(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandBypass(index);
}

QString NahdXmlKeys::bandFreq(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandFreq(index);
}

QString NahdXmlKeys::bandGain(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandGain(index);
}

QString NahdXmlKeys::bandKnee(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandKnee(index);
}

QString NahdXmlKeys::bandMakeup(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandMakeup(index);
}

QString NahdXmlKeys::bandQ(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandQ(index);
}

QString NahdXmlKeys::bandRatio(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandRatio(index);
}

QString NahdXmlKeys::bandRelease(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandRelease(index);
}

QString NahdXmlKeys::bandSlope(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandSlope(index);
}

QString NahdXmlKeys::bandSolo(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandSolo(index);
}

QString NahdXmlKeys::bandThreshold(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandThreshold(index);
}

QString NahdXmlKeys::bandType(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandType(index);
}

QString NahdXmlKeys::bandWidth(quint32 index) const
{
    return Constants::NahdXml::xmlKeyBandWidth(index);
}

QString NahdXmlKeys::crossoverFreq(quint32 index) const
{
    return Constants::NahdXml::xmlKeyCrossoverFreq(index);
}

} // namespace noteahead
