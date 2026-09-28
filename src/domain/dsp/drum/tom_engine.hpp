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

#ifndef TOM_ENGINE_HPP
#define TOM_ENGINE_HPP

#include "../base_rate_source.hpp"
#include "../cascaded_svf.hpp"
#include "drum_engine.hpp"

#include <array>
#include <random>

namespace noteahead {

class TomEngine : public DrumEngine
{
public:
    //! Which drum this is voiced as.
    //!
    //! Classic is the single sine the original Drum Synth has always played and is not free to
    //! change: every kit written against it has to keep sounding the way it did. Rd9 is fitted to a
    //! recording of the hardware and is what Drum Synth V2 selects.
    enum class Voicing
    {
        Classic,
        Rd9
    };

    TomEngine();
    virtual ~TomEngine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;
    void stop() override;

    void setVoicing(Voicing voicing);


    void setTune(float tune);
    void setDecay(float decay);
    void setPitchDepth(float depth);
    void setPitchDecay(float decay);

private:
    double m_phase { 0.0 };
    float m_ampEnv { 0.0f };
    float m_attackEnv { 0.0f };
    float m_retriggerOffset { 0.0f };
    float m_lastOut { 0.0f };
    float m_pitchEnv { 0.0f };
    bool m_active { false };

    Voicing m_voicing { Voicing::Classic };
    //! The two inharmonic modes above the fundamental, for the Rd9 voicing only: a drum head is
    //! not a sine, and the recordings carry partials at about twice and three times the pitch.
    std::array<double, 2> m_partialPhases {};
    //! The stick, which the Classic voicing has none of: about an eighth of the recordings' energy
    //! is in their first fifteen milliseconds, and a sine alone cannot put it there.
    float m_noiseEnv { 0.0f };
    CascadedSvf m_noiseFilter;
    BaseRateSource m_noiseBank;
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist { -1.0f, 1.0f };

    float m_tune { 0.5f };
    float m_decay { 0.5f };
    float m_pitchDepth { 0.5f };
    float m_pitchDecay { 0.5f };
    float m_velocity { 1.0f };
    bool m_stopping { false };
};

} // namespace noteahead

#endif // TOM_ENGINE_HPP
