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

#ifndef HIGH_PASS_FILTER_EFFECT_HPP
#define HIGH_PASS_FILTER_EFFECT_HPP

#include "effect.hpp"

namespace noteahead {

class HighPassFilterEffect : public Effect
{
public:
    void setCutoff(float cutoff);
    void process(float & left, float & right, uint32_t sampleRate) override;
    void reset() override;

private:
    float m_cutoff { 0.0f };
    float m_lpL { 0.0f }, m_hpL { 0.0f }, m_bpL { 0.0f };
    float m_lpR { 0.0f }, m_hpR { 0.0f }, m_bpR { 0.0f };
};

} // namespace noteahead

#endif // HIGH_PASS_FILTER_EFFECT_HPP
