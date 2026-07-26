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
#include <optional>
#include <string>

#include "../dsp/dsp_component.hpp"
#include "../tracker/parameter_container.hpp"

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

    virtual std::optional<size_t> sidechainSourceDeviceIndex() const
    {
        return std::nullopt;
    }

    bool enabled() const;
    void setEnabled(bool enabled);

    virtual void reset() override;
    virtual void sync();
    virtual void setBpm(float bpm);
    float bpm() const;

    //! Internal oversampling factor (1, 2 or 4) for nonlinear effects that render their shaping stage
    //! at a higher rate to suppress aliasing. Pushed per block from the AudioContext; 1 (no
    //! oversampling) unless set, so linear effects and direct per-sample use are unaffected.
    void setOversampleFactor(uint8_t factor);
    uint8_t oversampleFactor() const;

private:
    bool m_enabled { true };
    float m_bpm = 120;
    uint8_t m_oversampleFactor { 1 };
};

} // namespace noteahead

#endif // EFFECT_HPP
