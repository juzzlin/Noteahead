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

#ifndef AUTO_FILTER_HPP
#define AUTO_FILTER_HPP

#include <cstdint>

#include "../dsp/cascaded_svf.hpp"
#include "../dsp/lfo.hpp"
#include "effect.hpp"

namespace noteahead {

//! Cutoff and resonance sweeps that belong to the rack rather than to the device the sound came
//! from, so that anything routed through it can be swept: an LFO on the cutoff, a second one on the
//! resonance and an envelope follower that opens the filter with the signal's own level.
class AutoFilter : public Effect
{
public:
    AutoFilter();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void sync() override;
    void setBpm(float bpm) override;
    void reset() override;

    //! Cutoff modulation an intensity of ±100 % reaches, in octaves around the base cutoff.
    static double maxModulationOctaves();

    //! Resonance the sweep is allowed to reach. Short of the self-oscillation the filter would run
    //! into at 1, so that a full-intensity sweep cannot make it scream.
    static double maxResonance();

    //! Level that reads as a fully open envelope follower.
    static double envelopeFloorDb();

protected:
    void processSample(double & left, double & right) override;

private:
    void applyParameters();
    void updateLfoFrequencies();
    void updateEnvelopeCoefficients();
    void updateSampleRateDependents();
    void updateEnvelope(double left, double right);
    void updateModulation();

    //! Gain that puts the band-pass peak back at unity. CascadedSvf's band-pass tap peaks at 1/k
    //! per stage, where k = 2(1 - resonance), so without this the mode's level rides on the
    //! resonance: 6 dB down at the bottom of the range and 28 dB up at the top. Undoing it leaves
    //! resonance doing what it should in this mode, which is narrowing the band rather than
    //! turning it up. The other modes pass their band at unity already and need none of this.
    double bandPassCompensation(double resonance) const;

    //! Modulation reaches the filters this many samples apart rather than every sample, because a
    //! moved cutoff costs the filter a tan() and a divide to recompute its coefficients. At the
    //! lowest sample rate this still updates well above 1 kHz, which no LFO capped at 20 Hz can
    //! outrun.
    static constexpr uint32_t controlRateStride = 16;

    CascadedSvf m_filterL;
    CascadedSvf m_filterR;

    //! One LFO per channel per target: the right-hand pair runs the same shape at a phase offset,
    //! which is what turns a sweep into a stereo one.
    Lfo m_cutoffLfoL;
    Lfo m_cutoffLfoR;
    Lfo m_resonanceLfoL;
    Lfo m_resonanceLfoR;

    double m_cutoff { 0.7 };
    double m_resonance { 0.3 };

    Lfo::Mode m_cutoffLfoMode { Lfo::Mode::Normal };
    Lfo::Mode m_resonanceLfoMode { Lfo::Mode::Normal };
    double m_cutoffLfoRate { 0.5 };
    double m_resonanceLfoRate { 0.5 };
    //! Bipolar, -1 to 1, after the cubic taper the knobs read out in.
    double m_cutoffLfoIntensity { 0.0 };
    double m_resonanceLfoIntensity { 0.0 };
    //! Right-channel LFO offset in cycles, 0 to 0.5.
    double m_stereoPhase { 0.0 };

    double m_envIntensity { 0.0 };
    double m_envAttackMs { 10.0 };
    double m_envReleaseMs { 200.0 };
    double m_envAttackCoefficient { 0.0 };
    double m_envReleaseCoefficient { 0.0 };
    double m_envelopeDb { -60.0 };

    double m_gain { 1.0 };
    //! Output gain per channel: the Gain knob, and the band-pass normalization when that mode is
    //! selected, which has to follow the resonance as it is modulated.
    double m_outputGainL { 1.0 };
    double m_outputGainR { 1.0 };
    bool m_isBandPass { false };
    int m_filterStages { 2 };

    //! Octaves the filter's own normalized 0 to 1 cutoff spans at this sample rate, which is what
    //! turns an octave-denominated modulation into an offset the filter takes.
    double m_octaveSpan { 1.0 };
    double m_appliedSampleRate { 0.0 };
    uint32_t m_controlCounter { 0 };
    bool m_shouldApplyParameters { true };
};

} // namespace noteahead

#endif // AUTO_FILTER_HPP
