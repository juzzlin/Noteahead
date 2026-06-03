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

#ifndef EFFECT_HPP
#define EFFECT_HPP

#include <cstdint>
#include <string>

#include "../dsp/dsp_component.hpp"
#include "../parameter_container.hpp"

namespace noteahead {

struct AudioContext;

class Effect : public DspComponent, public ParameterContainer
{
public:
    Effect() = default;
    virtual ~Effect() override;

    Effect(const Effect &) = default;
    Effect & operator=(const Effect &) = default;
    Effect(Effect &&) = default;
    Effect & operator=(Effect &&) = default;

    virtual std::string type() const = 0;
    virtual std::string typeId() const = 0;
    virtual void process(double & left, double & right) = 0;
    virtual void process(AudioContext & context);
    using StringList = std::vector<std::string>;
    virtual StringList parameterNames() const;

    bool enabled() const;
    void setEnabled(bool enabled);

    virtual void reset() override;
    virtual void sync();
    virtual void setBpm(float bpm);

private:
    bool m_enabled { true };
};

} // namespace noteahead

#endif // EFFECT_HPP
