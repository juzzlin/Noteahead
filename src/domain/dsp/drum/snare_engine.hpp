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

#ifndef SNARE_ENGINE_HPP
#define SNARE_ENGINE_HPP

#include "../base_rate_source.hpp"
#include "../cascaded_svf.hpp"
#include "drum_engine.hpp"
#include <cstdint>
#include <random>

namespace noteahead {

class SnareEngine : public DrumEngine
{
public:
    SnareEngine();
    virtual ~SnareEngine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;
    void stop() override;

    void setTune(float tune);
    void setDecay(float decay);
    void setSnappy(float snappy);
    void setTone(float tone);
    void setInvertPhase(bool invert);

    void updateRates();

private:
    double m_tonalPhase1 { 0.0 };
    double m_tonalPhase2 { 0.0 };
    float m_ampEnv { 0.0f };
    float m_attackEnv { 0.0f };
    float m_tonalEnv { 0.0f };
    float m_pitchEnv { 0.0f };
    bool m_active { false };
    bool m_invertPhase { false };

    float m_tune { 0.5f };
    float m_decay { 0.5f };
    float m_snappy { 0.5f };
    float m_tone { 0.5f };
    float m_velocity { 1.0f };

    float m_pitchEnvDecay { 1.0f };
    float m_tonalDecayRate { 1.0f };
    float m_decayRate { 1.0f };
    double m_lastSampleRate { 0.0 };
    bool m_stopping { false };

    //! Fixed so a render is reproducible: the engine is re-seeded in reset(), which
    //! every render start runs, rather than carrying state over from earlier playback.
    static constexpr uint32_t RngSeed { 1001 };
    std::mt19937 m_rng { RngSeed };
    std::uniform_real_distribution<float> m_dist { -1.0f, 1.0f };
    //! Noise is drawn at the base rate and interpolated up, so what reaches the engine's
    //! saturation and filters is the same waveform at every oversampling factor.
    BaseRateSource m_noiseBank;
    CascadedSvf m_noiseFilter;
};

} // namespace noteahead

#endif // SNARE_ENGINE_HPP
