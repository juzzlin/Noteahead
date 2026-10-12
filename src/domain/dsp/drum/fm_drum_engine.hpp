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

#ifndef FM_DRUM_ENGINE_HPP
#define FM_DRUM_ENGINE_HPP

#include "../adsr_envelope.hpp"
#include "../base_rate_source.hpp"
#include "../cascaded_svf.hpp"
#include "../fm_operator.hpp"
#include "drum_engine.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>

namespace noteahead {

//! One pad of the FM Drums: four operators, a pitch envelope and a noise source.
//!
//! The same voice on every pad, so a pad is whatever its settings make it. The operators run the FM
//! Synth's algorithms -- see FmSynthDevice::algorithms() -- and differ from the synth's in two ways
//! that both come from a drum being struck rather than held. Their envelopes are attack-decay only:
//! nothing sustains and nothing is released, so a note-off means nothing here. And there is no
//! keyboard: the pitch is the pad's, and an operator's ratio is a plain multiple of it, whole or not,
//! which is where the metallic and inharmonic sounds come from.
//!
//! The pitch envelope bends every operator together, so the ratios between them, and with them the
//! timbre, hold while the drum falls in pitch. That one control is what turns a tuned bell into a
//! kick or a tom.
//!
//! FM alone cannot make a snare or a hat sound like one, which is what the noise is for. It can be
//! heard on its own, and it can also modulate the phase of one operator: noise into a carrier's
//! phase is the classic way FM gets a rattle that still sits on the drum's pitch.
//!
//! Everything is set in physical units. Mapping knob positions onto them is the device's business.
class FmDrumEngine : public DrumEngine
{
public:
    static constexpr size_t OperatorCount { 4 };

    //! The operator that feeds back into itself: the top of the chain, as in the FM Synth.
    static constexpr size_t FeedbackOperator { OperatorCount - 1 };

    //! Serialized as an ordinal by the device, so this is append-only.
    enum class NoiseFilterMode
    {
        LowPass = 0,
        BandPass,
        HighPass
    };

    static constexpr size_t NoiseFilterModeCount { 3 };

    FmDrumEngine();
    virtual ~FmDrumEngine() override = default;

    void setSampleRate(double sampleRate) override;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;
    //! Fades the drum out over DrumEngine::ChokeFadeSeconds: what an open hat does when the closed
    //! one is struck.
    void stop() override;

    //! Who modulates whom and who is heard, as FmSynthDevice::Algorithm has it: bit masks over the
    //! operators, bit 0 being operator 1, and every modulator above the operator it modulates.
    void setAlgorithm(const std::array<uint8_t, OperatorCount> & modulators, uint8_t carriers);
    void setBaseFrequency(double hz);

    void setOperatorRatio(size_t op, double ratio);
    //! 0..1, linear. For a carrier that is how loud it is; for a modulator, how deep it modulates.
    void setOperatorLevel(size_t op, double level);
    void setOperatorWaveform(size_t op, FmOperator::Waveform waveform);
    void setOperatorAttack(size_t op, double seconds);
    void setOperatorDecay(size_t op, double seconds);
    //! 0..1, see AdsrEnvelope::setCurve().
    void setOperatorCurve(size_t op, double curve);
    //! Self-feedback of the top operator, 0..1 of FmOperator::MaxIndexCycles.
    void setFeedback(double feedback);

    //! How far above the pad's pitch the drum starts, in semitones. Negative starts it below.
    void setPitchEnvelopeAmount(double semitones);
    //! Time constant of the fall back to the pad's pitch: the bend is about a third left after this.
    void setPitchEnvelopeDecay(double seconds);

    //! Noise straight to the output, 0..1.
    void setNoiseLevel(double level);
    //! Noise into the phase of the operator chosen by setNoiseTarget(), 0..1 of
    //! FmOperator::MaxIndexCycles.
    void setNoiseModulation(double amount);
    void setNoiseTarget(size_t op);
    void setNoiseAttack(double seconds);
    void setNoiseDecay(double seconds);
    void setNoiseCurve(double curve);
    void setNoiseFilterMode(NoiseFilterMode mode);
    //! 0..1, as CascadedSvf takes it: 20 Hz to 20 kHz on a log scale.
    void setNoiseCutoff(double cutoff);
    void setNoiseResonance(double resonance);

    //! How much a soft hit is quieter than a hard one, 0..1. Zero ignores velocity altogether.
    void setVelocityToLevel(double amount);
    //! How much a soft hit is duller than a hard one, 0..1: velocity scales every modulator's depth,
    //! which is what a struck drum does and what makes a fast roll sound played rather than looped.
    void setVelocityToModulation(double amount);

    //! The overall level, 0..1, after everything else.
    void setOutputLevel(double level);

private:
    //! Peak of a single full-level carrier. Leaves the same room as the analogue engines leave, so
    //! the two kits can be swapped without the mix moving: measured against Drum Synth V2, pad for
    //! pad, over the first hundred milliseconds.
    static constexpr double OutputGain { 0.7 };

    //! Make-up for the noise heard on its own. Filtered noise carries far less power than a sine of
    //! the same peak -- a band pass keeps only a slice of it -- so without this the noise knob ran
    //! out well before a clap could stand level with the drums around it.
    static constexpr double NoiseGain { 2.5 };

    //! How long a retrigger takes to blend away the last output of the hit it cut off. Starting
    //! every hit at phase zero is what makes a drum sound the same every time, and that restart is
    //! a step unless the old sound is let go of first.
    static constexpr double RetriggerBlendSeconds { 0.002 };

    std::array<FmOperator, OperatorCount> m_operators;
    std::array<double, OperatorCount> m_ratios {};
    //! Serial, 4>3>2>1, until told otherwise.
    std::array<uint8_t, OperatorCount> m_modulators { 0b0010, 0b0100, 0b1000, 0b0000 };
    uint8_t m_carriers { 0b0001 };
    double m_baseFrequency { 100.0 };
    double m_feedback { 0.0 };

    double m_pitchEnvelopeAmount { 0.0 };
    double m_pitchEnvelopeDecay { 0.05 };
    //! Fraction of the bend still left, falling from one at the strike.
    double m_pitchEnvelope { 0.0 };
    //! What m_pitchEnvelope is multiplied by every sample.
    double m_pitchEnvelopeCoefficient { 0.0 };

    AdsrEnvelope m_noiseEnvelope;
    CascadedSvf m_noiseFilter;
    BaseRateSource m_noiseBank;
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist { -1.0f, 1.0f };
    double m_noiseLevel { 0.0 };
    double m_noiseModulation { 0.0 };
    size_t m_noiseTarget { 0 };

    double m_velocityToLevel { 1.0 };
    double m_velocityToModulation { 0.0 };
    double m_outputLevel { 1.0 };

    double m_levelScale { 1.0 };
    double m_modulationScale { 1.0 };

    double m_chokeGain { 1.0 };
    bool m_stopping { false };
    bool m_active { false };

    double m_lastOut { 0.0 };
    double m_retriggerOffset { 0.0 };

    bool isAudible() const;
    void updatePitchEnvelopeCoefficient();
};

} // namespace noteahead

#endif // FM_DRUM_ENGINE_HPP
