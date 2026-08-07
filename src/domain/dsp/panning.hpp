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

#ifndef PANNING_HPP
#define PANNING_HPP

#include "../effects/effect.hpp"
#include "true_stereo_panner.hpp"

namespace noteahead {

class Panning : public Effect
{
public:
    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void setPan(float pan);
    void processSample(double & left, double & right) override;

private:
    TrueStereoPanner m_panner;
};

} // namespace noteahead

#endif // PANNING_HPP
