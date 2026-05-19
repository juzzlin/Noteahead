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

#ifndef PANNING_EFFECT_HPP
#define PANNING_EFFECT_HPP

#include "effect.hpp"

namespace noteahead {

class PanningEffect : public Effect
{
public:
    static std::string typeIdString() { return "c3d4e5f6-a7b8-4c9d-0e1f-2a3b4c5d6e7f"; }
    std::string type() const override { return "panning"; }
    std::string typeId() const override { return typeIdString(); }
    void setPan(float pan);
    void process(float & left, float & right) override;

private:
    float m_pan { 0.5f };
};

} // namespace noteahead

#endif // PANNING_EFFECT_HPP
