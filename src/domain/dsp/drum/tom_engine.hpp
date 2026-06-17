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

#include "drum_engine.hpp"

namespace noteahead {

class TomEngine : public DrumEngine
{
public:
    TomEngine();
    virtual ~TomEngine() override = default;

    void trigger(float velocity) override;
    float nextSample() override;
    bool isActive() const override;
    void reset() override;
    void stop() override;

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

    float m_tune { 0.5f };
    float m_decay { 0.5f };
    float m_pitchDepth { 0.5f };
    float m_pitchDecay { 0.5f };
    float m_velocity { 1.0f };
    bool m_stopping { false };
};

} // namespace noteahead

#endif // TOM_ENGINE_HPP
