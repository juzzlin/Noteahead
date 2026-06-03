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

#ifndef AUTO_PANNER_EFFECT_HPP
#define AUTO_PANNER_EFFECT_HPP

#include "../dsp/lfo.hpp"
#include "effect.hpp"

namespace noteahead {

class AutoPannerEffect : public Effect
{
public:
    AutoPannerEffect();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void process(double & left, double & right) override;
    void process(AudioContext & context) override;
    void sync() override;
    void setBpm(float bpm) override;

private:
    Lfo m_lfo;
    float m_intensity { 1.0f };
    bool m_sync { false };
    float m_rate { 0.5f };
    float m_syncDivision { 0.25f };
    float m_bpm { 120.0f };

    void updateLfoFrequency();
};

} // namespace noteahead

#endif // AUTO_PANNER_EFFECT_HPP
