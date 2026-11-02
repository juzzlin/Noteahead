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

#ifndef MULTI_ENGINE_HPP
#define MULTI_ENGINE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "dsp_component.hpp"

namespace noteahead {

//! The Synth's MULTI ENGINE, after the Minilogue XD's: a noise generator, or a VPM oscillator.
//!
//! VPM (variable phase modulation) is the XD's name for a two-operator phase modulation pair: a
//! modulator with self-feedback moves the phase of a carrier, by a depth an internal attack-decay
//! envelope can sweep, and noise can move the carrier's phase too. The carrier then goes through
//! a drive stage. The sixteen types follow the recipes the XD's manual gives for them; the values
//! that make them up are ours, not measured from the XD.
class MultiEngine : public DspComponent
{
public:
    //! Persisted as an ordinal, so append-only.
    enum class Engine
    {
        Noise,
        Vpm
    };

    //! The noise generators. Persisted as an ordinal, so append-only.
    enum class Type
    {
        High,
        Low,
        Peak,
        Decim
    };

    //! The VPM oscillator types, in the XD's order. Persisted as an ordinal, so append-only.
    enum class VpmType
    {
        Sin1,
        Sin2,
        Sin3,
        Sin4,
        Saw1,
        Saw2,
        Squ1,
        Squ2,
        Fat1,
        Fat2,
        Air1,
        Air2,
        Decay1,
        Decay2,
        Creep,
        Throat
    };

    static constexpr size_t VpmTypeCount { 16 };

    static std::vector<std::string> vpmTypeNames();

    //! The steps of the XD's RATIO OFFSET, which multiply the type's own modulator ratio.
    static std::vector<double> vpmRatioOffsets();
    static std::vector<std::string> vpmRatioOffsetNames();
    //! The step that leaves each type's ratio as it is, 1:1.
    static constexpr size_t VpmRatioOffsetUnity { 2 };

    //! The XD's six VPM parameters, each -1..1 relative to the type's own setting: 0 is the type
    //! as designed, -1 takes the setting away entirely and +1 takes it to its maximum.
    struct VpmOffsets
    {
        double feedback { 0.0 };
        double noiseDepth { 0.0 };
        double shapeModInt { 0.0 };
        double modAttack { 0.0 };
        double modDecay { 0.0 };
        double modKeyTrack { 0.0 };
    };

    MultiEngine();

    void setEngine(Engine engine);
    Engine engine() const;

    void setType(Type type);
    void setShape(float shape);
    void setKeyTrack(float keyTrack);
    void setNote(uint8_t note);

    void setVpmType(VpmType type);
    void setVpmRatioOffset(size_t index);
    void setVpmOffsets(const VpmOffsets & offsets);
    //! The pitch the VPM oscillator plays, octave and modulation included. Noise ignores it.
    void setFrequency(double frequency);
    //! Restarts the internal envelope for a new note. With @p resetPhases the operators start
    //! from zero too, which a voice starting from silence wants and a retrigger must not do.
    void trigger(bool resetPhases);
    //! Keeps the noise source's in-band level independent of the oversampling factor.
    void setOversampleFactor(uint8_t factor);

    float nextSample();
    void reset();

private:
    //! What one VPM type is made of. Depths are in radians of carrier phase.
    struct VpmDefinition
    {
        enum class Wave
        {
            Sine,
            Saw,
            Square
        };
        enum class Modulator
        {
            Sine,
            Noise
        };
        Wave carrier { Wave::Sine };
        Modulator modulator { Modulator::Sine };
        //! Modulator frequency over carrier frequency.
        double ratio { 1.0 };
        //! Depth at Shape zero, and what Shape adds on top at full.
        double baseDepth { 0.0 };
        double depthRange { 4.0 };
        //! Modulator self-feedback, and carrier self-feedback for a sine standing in for a saw.
        double feedback { 0.0 };
        double carrierFeedback { 0.0 };
        //! Noise added straight to the carrier's phase, and the corner it is low passed at.
        double noiseDepth { 0.0 };
        double noiseCutoff { 3000.0 };
        //! What Shape adds on top of the noise depth at full, and the octaves it opens the corner by.
        double noiseShapeRange { 0.0 };
        double noiseShapeOctaves { 0.0 };
        //! Depth the internal envelope adds at its peak, and its times in seconds.
        double envelopeDepth { 0.0 };
        double attack { 0.005 };
        double decay { 0.5 };
        //! 0..1: how much the depth falls away above middle C.
        double keyTrack { 0.3 };
        //! Gain into the carrier's saturator, 1 for none.
        double drive { 1.0 };
        //! A slow wander of the modulator ratio, as a fraction of it, and its rate in Hz.
        double ratioWander { 0.0 };
        double ratioWanderRate { 0.0 };
        //! A wobble of the depth, as a fraction of it, and its rate in Hz.
        double depthWobble { 0.0 };
        double depthWobbleRate { 0.0 };
    };

    static const std::array<VpmDefinition, VpmTypeCount> & vpmDefinitions();
    static double applyOffset(double base, double offset, double maximum);

    float nextNoiseSample();
    float nextVpmSample();
    double nextPhaseNoise(double cutoff);
    double carrierWave(VpmDefinition::Wave wave, double phase) const;

    Engine m_engine { Engine::Noise };
    VpmType m_vpmType { VpmType::Sin1 };
    size_t m_vpmRatioOffset { VpmRatioOffsetUnity };
    VpmOffsets m_vpmOffsets;
    double m_frequency { 261.6255653005986 };

    // VPM state
    double m_carrierPhase { 0.0 };
    double m_modulatorPhase { 0.0 };
    double m_wanderPhase { 0.0 };
    double m_wobblePhase { 0.0 };
    std::array<double, 2> m_modulatorHistory {};
    std::array<double, 2> m_carrierHistory {};
    double m_envelope { 0.0 };
    bool m_envelopeAttacking { false };
    double m_phaseNoise { 0.0 };

    float m_noiseGain { 1.0f };
    Type m_type { Type::Low };
    float m_shape { 0.5f };
    float m_keyTrack { 0.0f };
    uint8_t m_note { 60 };

    //! Fixed so a render is reproducible: the engine is re-seeded in reset(), which
    //! every render start runs, rather than carrying state over from earlier playback.
    static constexpr uint32_t RngSeed { 1007 };
    std::mt19937 m_rng { RngSeed };
    std::uniform_real_distribution<float> m_dist;

    // Filter states
    double m_s1 { 0.0 };
    double m_s2 { 0.0 };

    // Decimator state
    float m_lastSample { 0.0f };
    double m_phase { 0.0 };

    // Cached coefficients
    double m_g { 0.0 };
    double m_k { 0.0 };
    double m_damping { 0.0 };
    double m_decimRate { 0.0 };
    double m_lastFrequency { -1.0 };
    double m_lastResonance { -1.0 };
    double m_lastSampleRate { -1.0 };
    float m_lastDecimRateParam { -1.0f };
    uint8_t m_lastNote { 0xFF };

    double maxFrequency() const;
    double shapeToFrequency(float shape) const;
    double keyTrackedFrequency(double frequency) const;
    void updateCoefficients(double frequency, float resonance);
    void updateDecimRate();
    float processFilter(float input, int mode);
};

} // namespace noteahead

#endif // MULTI_ENGINE_HPP
