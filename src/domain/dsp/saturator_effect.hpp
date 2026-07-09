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

#ifndef SATURATOR_EFFECT_HPP
#define SATURATOR_EFFECT_HPP

#include "../effects/effect.hpp"
#include "cascaded_svf.hpp"

namespace noteahead {

class SaturatorEffect : public Effect
{
public:
    enum class Mode
    {
        Tape,
        Tube,
        Diode
    };

    SaturatorEffect();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void process(double & left, double & right) override;
    void reset() override;
    void sync() override;

    float saturationDb() const;

private:
    void syncParameters();
    double shape(double x) const;

    Mode m_mode { Mode::Tape };
    float m_driveDb { 0.0f };
    float m_tone { 1.0f };
    float m_mix { 1.0f };
    float m_outputDb { 0.0f };

    CascadedSvf m_toneFilterL;
    CascadedSvf m_toneFilterR;

    double m_saturationDb { 0.0 };
};

} // namespace noteahead

#endif // SATURATOR_EFFECT_HPP
