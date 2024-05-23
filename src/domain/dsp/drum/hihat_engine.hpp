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

#ifndef HIHAT_ENGINE_HPP
#define HIHAT_ENGINE_HPP

#include "../cascaded_svf.hpp"
#include "drum_engine.hpp"
#include <array>
#include <random>

namespace noteahead {

class HiHatEngine : public DrumEngine
{
public:
    HiHatEngine();
    virtual ~HiHatEngine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;

    void setTune(float tune);
    void setDecay(float decay);
    void setResonance(float resonance);

    void stop(); // For choking
    void updateRates();

private:
    float m_ampEnv { 0.0f };
    bool m_active { false };
    bool m_choking { false };

    float m_tune { 0.5f };
    float m_decay { 0.5f };
    float m_resonance { 0.3f };
    float m_velocity { 1.0f };

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist { -1.0f, 1.0f };
    CascadedSvf m_filter;
    CascadedSvf m_bodyFilter;

    std::array<double, 6> m_phases { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    float m_bodyEnv { 0.0f };

    float m_bodyDecayRate { 1.0f };
    float m_decayRate { 1.0f };
    float m_chokeDecayRate { 1.0f };
    double m_lastSampleRate { 0.0 };
};

} // namespace noteahead

#endif // HIHAT_ENGINE_HPP
