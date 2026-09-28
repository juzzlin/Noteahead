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

#ifndef RIDE_ENGINE_HPP
#define RIDE_ENGINE_HPP

#include "../base_rate_source.hpp"
#include "../cascaded_svf.hpp"
#include "drum_engine.hpp"

#include <array>
#include <cstdint>
#include <random>

namespace noteahead {

class RideEngine : public DrumEngine
{
public:
    //! Which cymbal this is voiced as.
    //!
    //! Classic is what the original Drum Synth has always played and is not free to change: every
    //! kit written against it has to keep sounding the way it did. Rd9 is fitted to a recording of
    //! the hardware and is what Drum Synth V2 selects.
    enum class Voicing
    {
        Classic,
        Rd9
    };

    RideEngine();
    virtual ~RideEngine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;
    void stop() override;

    void setVoicing(Voicing voicing);



    void setTune(float tune);
    void setDecay(float decay);
    void setResonance(float resonance);

private:
    float m_ampEnv { 0.0f };
    float m_attackEnv { 0.0f };
    bool m_active { false };

    Voicing m_voicing { Voicing::Classic };
    //! Falls from the strike, closing m_damping with it: see the darkening in the .cpp.
    float m_darkenEnv { 1.0f };
    float m_tune { 0.5f };
    float m_decay { 0.5f };
    float m_resonance { 0.3f };
    float m_velocity { 1.0f };

    //! Fixed so a render is reproducible: the engine is re-seeded in reset(), which
    //! every render start runs, rather than carrying state over from earlier playback.
    static constexpr uint32_t RngSeed { 1004 };
    std::mt19937 m_rng { RngSeed };
    std::uniform_real_distribution<float> m_dist { -1.0f, 1.0f };
    CascadedSvf m_filter;
    //! Frequency-dependent damping of the struck metal, for the Rd9 voicing only.
    CascadedSvf m_damping;
    //! The shimmer's own path: see why it is not damped with the metal, in the .cpp.
    CascadedSvf m_noiseFilter;

    //! Bank of square oscillators, run at the base rate so it is oversampling-independent.
    BaseRateSource m_metallicBank;
    BaseRateSource m_noiseBank;
    float nextMetallicBaseSample();

    //! Sixteen, of which the Classic voicing sounds the first six. The hardware's ride carries most of
    //! its weight between four and sixteen kilohertz, and six partials over a 550 Hz base reach
    //! only 2.3: everything above that was coming from the noise, which is why taking the noise
    //! away took the brightness with it.
    static constexpr size_t ClassicPartials { 6 };
    std::array<double, 16> m_phases {};
    bool m_stopping { false };
};

} // namespace noteahead

#endif // RIDE_ENGINE_HPP
