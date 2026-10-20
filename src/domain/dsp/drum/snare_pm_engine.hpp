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

#ifndef SNARE_PM_ENGINE_HPP
#define SNARE_PM_ENGINE_HPP

#include "../base_rate_source.hpp"
#include "../cascaded_svf.hpp"
#include "drum_engine.hpp"

#include <array>
#include <cstdint>
#include <random>

namespace noteahead {

//! Physical modelling snare drum engine.
//!
//! Simulates four coupled acoustic subsystems:
//! 1. Batter Head: circular 2D membrane modal resonator bank (Bessel mode ratios).
//! 2. Resonant Bottom Head: coupled membrane modes driven by internal shell air coupling.
//! 3. Snare Wires: non-linear chatter and contact rattle excited by bottom head excursion.
//! 4. Stick Strike & Body: transient impact click with hardness/position control and shell cavity resonance.
class SnarePmEngine : public DrumEngine
{
public:
    static constexpr int BatterModes = 8;
    static constexpr int BottomModes = 4;
    static constexpr int TotalModes = BatterModes + BottomModes;

    SnarePmEngine();
    ~SnarePmEngine() override;

    void trigger(float velocity) override;
    float nextSample() override;
    void nextSample(float & left, float & right);
    bool isActive() const override;
    void reset() override;
    void stop() override;

    // Head / Membrane parameters
    void setTune(float tuneHz);
    void setTone(float tone);
    void setDecay(float decaySeconds);
    void setHeadTension(float ratio);
    void setPitchDepth(float semitones);
    void setPitchDecay(float seconds);

    // Snare wire parameters
    void setSnappy(float snappy);
    void setSnareDecay(float seconds);
    void setSnareTension(float tension);
    void setSnareBuzz(float buzz);

    // Stick strike & body parameters
    void setClick(float click);
    void setHardness(float hardness);
    void setPosition(float position);
    void setBody(float body);
    void setDrive(float drive);
    void setPanSpread(float spread);

    void updateRates();

private:
    struct Mode
    {
        double re { 0.0 };
        double im { 0.0 };
        double tailRe { 0.0 };
        double tailIm { 0.0 };
        double ratio { 1.0 };
        double damping { 0.0 };
        double amplitude { 0.0 };
    };

    void initModes();
    void updateModeDamping();
    void triggerModes(float velocity);

    std::array<Mode, TotalModes> m_modes {};

    // Parameters
    float m_tune { 200.0f };
    float m_tone { 0.5f };
    float m_decay { 0.25f };
    float m_headTension { 1.35f };
    float m_pitchDepth { 6.0f };
    float m_pitchDecay { 0.025f };

    float m_snappy { 0.6f };
    float m_snareDecay { 0.28f };
    float m_snareTension { 0.5f };
    float m_snareBuzz { 0.5f };

    float m_click { 0.4f };
    float m_hardness { 0.5f };
    float m_position { 0.2f };
    float m_body { 0.3f };
    float m_drive { 0.0f };
    float m_panSpread { 0.5f };

    // State
    bool m_active { false };
    float m_velocity { 1.0f };

    // Pitch envelope
    double m_pitchEnv { 0.0 };
    double m_pitchEnvDecayRate { 1.0 };

    // Snare wire envelope, buzz & filters
    double m_snareEnv { 0.0 };
    double m_snareEnvDecayRate { 1.0 };
    double m_buzzEnv { 0.0 };
    double m_buzzEnvDecayRate { 1.0 };
    CascadedSvf m_snareFilterL {};
    CascadedSvf m_snareFilterR {};

    // Shell body resonator
    CascadedSvf m_shellResonator {};

    // Stick impact click state
    double m_clickSample { 0.0 };
    double m_clickPhase { 0.0 };
    double m_clickPhaseIncrement { 0.0 };
    double m_clickDecayRate { 1.0 };

    // Tail damping on retrigger
    double m_tailDamping { 0.0 };

    // Noise generation for snare wires
    static constexpr uint32_t RngSeedL { 20261 };
    static constexpr uint32_t RngSeedR { 20262 };
    std::mt19937 m_rngL { RngSeedL };
    std::mt19937 m_rngR { RngSeedR };
    std::uniform_real_distribution<float> m_dist { -1.0f, 1.0f };
    BaseRateSource m_noiseBankL {};
    BaseRateSource m_noiseBankR {};

    double m_lastSampleRate { 0.0 };
};

} // namespace noteahead

#endif // SNARE_PM_ENGINE_HPP
