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

#ifndef PARAMETER_DATA_HPP
#define PARAMETER_DATA_HPP

#include "event_data.hpp"
#include "parameter_automation.hpp"

#include <memory>
#include <string>

namespace noteahead {

class ParameterAutomationTarget;

//! One step of a ParameterAutomation, as the player hands it on.
//!
//! Carries the effect or device itself rather than its id: the song is rendered afresh for every
//! play, so the target is looked up once there instead of once per event on the way to the audio
//! thread. Held weakly, so one removed while the song plays is simply no longer driven.
class ParameterData : public EventData
{
public:
    using TargetW = std::weak_ptr<ParameterAutomationTarget>;
    ParameterData(size_t track, size_t column, TargetW target, std::string parameter, ParameterAutomation::Mode mode, double value);

    const TargetW & target() const;
    const std::string & parameter() const;
    ParameterAutomation::Mode mode() const;
    //! A knob position for an absolute automation; how far from rest towards the knob for a
    //! relative one. 0 to 1 either way.
    double value() const;

private:
    TargetW m_target;
    std::string m_parameter;
    ParameterAutomation::Mode m_mode = ParameterAutomation::Mode::Absolute;
    double m_value = 0.0;
};

} // namespace noteahead

#endif // PARAMETER_DATA_HPP
