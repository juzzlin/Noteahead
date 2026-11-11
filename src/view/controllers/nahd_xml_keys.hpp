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

#ifndef NAHD_XML_KEYS_HPP
#define NAHD_XML_KEYS_HPP

#include <QObject>
#include <QString>

namespace noteahead {

//! The parameter keys of Constants::NahdXml, for QML: NahdXml.lfoRate is xmlKeyLfoRate(). One
//! object for every dialog, so that no controller needs a getter per key and per effect. Add a key
//! here when a dialog needs one it does not have yet.
class NahdXmlKeys : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString airFreq MEMBER m_airFreq CONSTANT)
    Q_PROPERTY(QString airGain MEMBER m_airGain CONSTANT)
    Q_PROPERTY(QString amount MEMBER m_amount CONSTANT)
    Q_PROPERTY(QString attack MEMBER m_attack CONSTANT)
    Q_PROPERTY(QString autoGain MEMBER m_autoGain CONSTANT)
    Q_PROPERTY(QString bandCount MEMBER m_bandCount CONSTANT)
    Q_PROPERTY(QString bandwidth MEMBER m_bandwidth CONSTANT)
    Q_PROPERTY(QString bassDecay MEMBER m_bassDecay CONSTANT)
    Q_PROPERTY(QString bassFreq MEMBER m_bassFreq CONSTANT)
    Q_PROPERTY(QString bassGain MEMBER m_bassGain CONSTANT)
    Q_PROPERTY(QString bias MEMBER m_bias CONSTANT)
    Q_PROPERTY(QString bitDepth MEMBER m_bitDepth CONSTANT)
    Q_PROPERTY(QString blend MEMBER m_blend CONSTANT)
    Q_PROPERTY(QString boost MEMBER m_boost CONSTANT)
    Q_PROPERTY(QString ceiling MEMBER m_ceiling CONSTANT)
    Q_PROPERTY(QString channelMode MEMBER m_channelMode CONSTANT)
    Q_PROPERTY(QString color MEMBER m_color CONSTANT)
    Q_PROPERTY(QString converters MEMBER m_converters CONSTANT)
    Q_PROPERTY(QString cutoff MEMBER m_cutoff CONSTANT)
    Q_PROPERTY(QString damping MEMBER m_damping CONSTANT)
    Q_PROPERTY(QString dbRange MEMBER m_dbRange CONSTANT)
    Q_PROPERTY(QString dcBlock MEMBER m_dcBlock CONSTANT)
    Q_PROPERTY(QString decay MEMBER m_decay CONSTANT)
    Q_PROPERTY(QString delay MEMBER m_delay CONSTANT)
    Q_PROPERTY(QString delaySyncDivision MEMBER m_delaySyncDivision CONSTANT)
    Q_PROPERTY(QString delayTime MEMBER m_delayTime CONSTANT)
    Q_PROPERTY(QString depth MEMBER m_depth CONSTANT)
    Q_PROPERTY(QString detectorFilter MEMBER m_detectorFilter CONSTANT)
    Q_PROPERTY(QString detectorSource MEMBER m_detectorSource CONSTANT)
    Q_PROPERTY(QString detune MEMBER m_detune CONSTANT)
    Q_PROPERTY(QString diffusion MEMBER m_diffusion CONSTANT)
    Q_PROPERTY(QString drive MEMBER m_drive CONSTANT)
    Q_PROPERTY(QString driveDb MEMBER m_driveDb CONSTANT)
    Q_PROPERTY(QString dynamics MEMBER m_dynamics CONSTANT)
    Q_PROPERTY(QString earlyLate MEMBER m_earlyLate CONSTANT)
    Q_PROPERTY(QString enhance MEMBER m_enhance CONSTANT)
    Q_PROPERTY(QString envMod MEMBER m_envMod CONSTANT)
    Q_PROPERTY(QString environment MEMBER m_environment CONSTANT)
    Q_PROPERTY(QString feedback MEMBER m_feedback CONSTANT)
    Q_PROPERTY(QString fftRate MEMBER m_fftRate CONSTANT)
    Q_PROPERTY(QString filterSlope MEMBER m_filterSlope CONSTANT)
    Q_PROPERTY(QString filterType MEMBER m_filterType CONSTANT)
    Q_PROPERTY(QString freeze MEMBER m_freeze CONSTANT)
    Q_PROPERTY(QString frequency MEMBER m_frequency CONSTANT)
    Q_PROPERTY(QString fuzz MEMBER m_fuzz CONSTANT)
    Q_PROPERTY(QString gain MEMBER m_gain CONSTANT)
    Q_PROPERTY(QString gated MEMBER m_gated CONSTANT)
    Q_PROPERTY(QString harmonics MEMBER m_harmonics CONSTANT)
    Q_PROPERTY(QString highAtten MEMBER m_highAtten CONSTANT)
    Q_PROPERTY(QString highAttenFreq MEMBER m_highAttenFreq CONSTANT)
    Q_PROPERTY(QString highBoost MEMBER m_highBoost CONSTANT)
    Q_PROPERTY(QString highBoostFreq MEMBER m_highBoostFreq CONSTANT)
    Q_PROPERTY(QString highFreq MEMBER m_highFreq CONSTANT)
    Q_PROPERTY(QString highGain MEMBER m_highGain CONSTANT)
    Q_PROPERTY(QString hold MEMBER m_hold CONSTANT)
    Q_PROPERTY(QString hpfCutoff MEMBER m_hpfCutoff CONSTANT)
    Q_PROPERTY(QString intensity MEMBER m_intensity CONSTANT)
    Q_PROPERTY(QString invertLeft MEMBER m_invertLeft CONSTANT)
    Q_PROPERTY(QString invertRight MEMBER m_invertRight CONSTANT)
    Q_PROPERTY(QString knee MEMBER m_knee CONSTANT)
    Q_PROPERTY(QString lfo2Intensity MEMBER m_lfo2Intensity CONSTANT)
    Q_PROPERTY(QString lfo2Mode MEMBER m_lfo2Mode CONSTANT)
    Q_PROPERTY(QString lfo2Rate MEMBER m_lfo2Rate CONSTANT)
    Q_PROPERTY(QString lfo2Waveform MEMBER m_lfo2Waveform CONSTANT)
    Q_PROPERTY(QString lfoIntensity MEMBER m_lfoIntensity CONSTANT)
    Q_PROPERTY(QString lfoMode MEMBER m_lfoMode CONSTANT)
    Q_PROPERTY(QString lfoRate MEMBER m_lfoRate CONSTANT)
    Q_PROPERTY(QString lfoWaveform MEMBER m_lfoWaveform CONSTANT)
    Q_PROPERTY(QString linearPhase MEMBER m_linearPhase CONSTANT)
    Q_PROPERTY(QString listen MEMBER m_listen CONSTANT)
    Q_PROPERTY(QString lookahead MEMBER m_lookahead CONSTANT)
    Q_PROPERTY(QString lowAtten MEMBER m_lowAtten CONSTANT)
    Q_PROPERTY(QString lowBoost MEMBER m_lowBoost CONSTANT)
    Q_PROPERTY(QString lowFreq MEMBER m_lowFreq CONSTANT)
    Q_PROPERTY(QString lpfCutoff MEMBER m_lpfCutoff CONSTANT)
    Q_PROPERTY(QString makeup MEMBER m_makeup CONSTANT)
    Q_PROPERTY(QString midDecay MEMBER m_midDecay CONSTANT)
    Q_PROPERTY(QString midFreq MEMBER m_midFreq CONSTANT)
    Q_PROPERTY(QString midGain MEMBER m_midGain CONSTANT)
    Q_PROPERTY(QString midQ MEMBER m_midQ CONSTANT)
    Q_PROPERTY(QString mix MEMBER m_mix CONSTANT)
    Q_PROPERTY(QString mode MEMBER m_mode CONSTANT)
    Q_PROPERTY(QString monoBass MEMBER m_monoBass CONSTANT)
    Q_PROPERTY(QString monoBassFrequency MEMBER m_monoBassFrequency CONSTANT)
    Q_PROPERTY(QString monoFreq MEMBER m_monoFreq CONSTANT)
    Q_PROPERTY(QString peak MEMBER m_peak CONSTANT)
    Q_PROPERTY(QString peakReduction MEMBER m_peakReduction CONSTANT)
    Q_PROPERTY(QString pinkNoiseLevel MEMBER m_pinkNoiseLevel CONSTANT)
    Q_PROPERTY(QString preDelay MEMBER m_preDelay CONSTANT)
    Q_PROPERTY(QString program MEMBER m_program CONSTANT)
    Q_PROPERTY(QString q MEMBER m_q CONSTANT)
    Q_PROPERTY(QString range MEMBER m_range CONSTANT)
    Q_PROPERTY(QString rate MEMBER m_rate CONSTANT)
    Q_PROPERTY(QString rateDivider MEMBER m_rateDivider CONSTANT)
    Q_PROPERTY(QString ratio MEMBER m_ratio CONSTANT)
    Q_PROPERTY(QString release MEMBER m_release CONSTANT)
    Q_PROPERTY(QString resonance MEMBER m_resonance CONSTANT)
    Q_PROPERTY(QString room MEMBER m_room CONSTANT)
    Q_PROPERTY(QString sampleRate MEMBER m_sampleRate CONSTANT)
    Q_PROPERTY(QString shape MEMBER m_shape CONSTANT)
    Q_PROPERTY(QString showGuides MEMBER m_showGuides CONSTANT)
    Q_PROPERTY(QString showPinkNoise MEMBER m_showPinkNoise CONSTANT)
    Q_PROPERTY(QString sideChainLpf MEMBER m_sideChainLpf CONSTANT)
    Q_PROPERTY(QString sideChainSourceDevice MEMBER m_sideChainSourceDevice CONSTANT)
    Q_PROPERTY(QString size MEMBER m_size CONSTANT)
    Q_PROPERTY(QString solo MEMBER m_solo CONSTANT)
    Q_PROPERTY(QString speed MEMBER m_speed CONSTANT)
    Q_PROPERTY(QString splitFreq MEMBER m_splitFreq CONSTANT)
    Q_PROPERTY(QString spread MEMBER m_spread CONSTANT)
    Q_PROPERTY(QString stages MEMBER m_stages CONSTANT)
    Q_PROPERTY(QString stereoMode MEMBER m_stereoMode CONSTANT)
    Q_PROPERTY(QString stereoPhase MEMBER m_stereoPhase CONSTANT)
    Q_PROPERTY(QString sustain MEMBER m_sustain CONSTANT)
    Q_PROPERTY(QString sync MEMBER m_sync CONSTANT)
    Q_PROPERTY(QString threshold MEMBER m_threshold CONSTANT)
    Q_PROPERTY(QString timbre MEMBER m_timbre CONSTANT)
    Q_PROPERTY(QString tone MEMBER m_tone CONSTANT)
    Q_PROPERTY(QString trebleDecay MEMBER m_trebleDecay CONSTANT)
    Q_PROPERTY(QString tune MEMBER m_tune CONSTANT)
    Q_PROPERTY(QString waveform MEMBER m_waveform CONSTANT)
    Q_PROPERTY(QString width MEMBER m_width CONSTANT)
    Q_PROPERTY(QString zeroFill MEMBER m_zeroFill CONSTANT)
    Q_PROPERTY(QString zoom MEMBER m_zoom CONSTANT)

public:
    explicit NahdXmlKeys(QObject * parent = nullptr);

    //! The keys numbered by band or crossover.
    Q_INVOKABLE QString bandAttack(quint32 index) const;
    Q_INVOKABLE QString bandBypass(quint32 index) const;
    Q_INVOKABLE QString bandFreq(quint32 index) const;
    Q_INVOKABLE QString bandGain(quint32 index) const;
    Q_INVOKABLE QString bandKnee(quint32 index) const;
    Q_INVOKABLE QString bandMakeup(quint32 index) const;
    Q_INVOKABLE QString bandQ(quint32 index) const;
    Q_INVOKABLE QString bandRatio(quint32 index) const;
    Q_INVOKABLE QString bandRelease(quint32 index) const;
    Q_INVOKABLE QString bandSlope(quint32 index) const;
    Q_INVOKABLE QString bandSolo(quint32 index) const;
    Q_INVOKABLE QString bandThreshold(quint32 index) const;
    Q_INVOKABLE QString bandType(quint32 index) const;
    Q_INVOKABLE QString bandWidth(quint32 index) const;
    Q_INVOKABLE QString crossoverFreq(quint32 index) const;

private:
    QString m_airFreq;
    QString m_airGain;
    QString m_amount;
    QString m_attack;
    QString m_autoGain;
    QString m_bandCount;
    QString m_bandwidth;
    QString m_bassDecay;
    QString m_bassFreq;
    QString m_bassGain;
    QString m_bias;
    QString m_bitDepth;
    QString m_blend;
    QString m_boost;
    QString m_ceiling;
    QString m_channelMode;
    QString m_color;
    QString m_converters;
    QString m_cutoff;
    QString m_damping;
    QString m_dbRange;
    QString m_dcBlock;
    QString m_decay;
    QString m_delay;
    QString m_delaySyncDivision;
    QString m_delayTime;
    QString m_depth;
    QString m_detectorFilter;
    QString m_detectorSource;
    QString m_detune;
    QString m_diffusion;
    QString m_drive;
    QString m_driveDb;
    QString m_dynamics;
    QString m_earlyLate;
    QString m_enhance;
    QString m_envMod;
    QString m_environment;
    QString m_feedback;
    QString m_fftRate;
    QString m_filterSlope;
    QString m_filterType;
    QString m_freeze;
    QString m_frequency;
    QString m_fuzz;
    QString m_gain;
    QString m_gated;
    QString m_harmonics;
    QString m_highAtten;
    QString m_highAttenFreq;
    QString m_highBoost;
    QString m_highBoostFreq;
    QString m_highFreq;
    QString m_highGain;
    QString m_hold;
    QString m_hpfCutoff;
    QString m_intensity;
    QString m_invertLeft;
    QString m_invertRight;
    QString m_knee;
    QString m_lfo2Intensity;
    QString m_lfo2Mode;
    QString m_lfo2Rate;
    QString m_lfo2Waveform;
    QString m_lfoIntensity;
    QString m_lfoMode;
    QString m_lfoRate;
    QString m_lfoWaveform;
    QString m_linearPhase;
    QString m_listen;
    QString m_lookahead;
    QString m_lowAtten;
    QString m_lowBoost;
    QString m_lowFreq;
    QString m_lpfCutoff;
    QString m_makeup;
    QString m_midDecay;
    QString m_midFreq;
    QString m_midGain;
    QString m_midQ;
    QString m_mix;
    QString m_mode;
    QString m_monoBass;
    QString m_monoBassFrequency;
    QString m_monoFreq;
    QString m_peak;
    QString m_peakReduction;
    QString m_pinkNoiseLevel;
    QString m_preDelay;
    QString m_program;
    QString m_q;
    QString m_range;
    QString m_rate;
    QString m_rateDivider;
    QString m_ratio;
    QString m_release;
    QString m_resonance;
    QString m_room;
    QString m_sampleRate;
    QString m_shape;
    QString m_showGuides;
    QString m_showPinkNoise;
    QString m_sideChainLpf;
    QString m_sideChainSourceDevice;
    QString m_size;
    QString m_solo;
    QString m_speed;
    QString m_splitFreq;
    QString m_spread;
    QString m_stages;
    QString m_stereoMode;
    QString m_stereoPhase;
    QString m_sustain;
    QString m_sync;
    QString m_threshold;
    QString m_timbre;
    QString m_tone;
    QString m_trebleDecay;
    QString m_tune;
    QString m_waveform;
    QString m_width;
    QString m_zeroFill;
    QString m_zoom;
};

} // namespace noteahead

#endif // NAHD_XML_KEYS_HPP
