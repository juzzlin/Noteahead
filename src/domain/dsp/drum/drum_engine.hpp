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

#ifndef DRUM_ENGINE_HPP
#define DRUM_ENGINE_HPP

#include "../dsp_component.hpp"

namespace noteahead {

class DrumEngine : public DspComponent
{
public:
    static constexpr float AmplitudeThreshold { 0.0001f };
    static constexpr float ChokeFadeSeconds { 0.015f };

    virtual ~DrumEngine() override = default;
    virtual void trigger(float velocity) = 0;
    virtual float nextSample() = 0;
    virtual bool isActive() const = 0;
    virtual void reset() = 0;

    virtual void stop()
    {
    }
};

} // namespace noteahead

#endif // DRUM_ENGINE_HPP
