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

#ifndef LOW_PASS_FILTER_EFFECT_HPP
#define LOW_PASS_FILTER_EFFECT_HPP

#include "effect.hpp"

namespace noteahead {

class LowPassFilterEffect : public Effect
{
public:
    void setCutoff(float cutoff);
    void process(float & left, float & right, uint32_t sampleRate) override;
    void reset() override;

private:
    float m_cutoff { 1.0f };
    double m_s1L { 0.0 }, m_s2L { 0.0 };
    double m_s1R { 0.0 }, m_s2R { 0.0 };
};

} // namespace noteahead

#endif // LOW_PASS_FILTER_EFFECT_HPP
