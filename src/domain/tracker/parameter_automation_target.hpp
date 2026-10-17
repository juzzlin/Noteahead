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

#ifndef PARAMETER_AUTOMATION_TARGET_HPP
#define PARAMETER_AUTOMATION_TARGET_HPP

#include "parameter_automation.hpp"
#include "parameter_modulation.hpp"

#include <string>

namespace noteahead {

//! Something a parameter automation can drive: an effect, a device.
//!
//! Each applies a step its own way -- an effect resyncs, a device does so under its own lock, the way
//! it takes a MIDI controller -- so the engine can hand every step over without knowing which it is.
class ParameterAutomationTarget
{
public:
    virtual ~ParameterAutomationTarget();

    //! Runs the named parameter at the value an automation in @p mode at @p value comes to. Called
    //! on the audio thread. False, and nothing done, when there is no such continuous parameter: a
    //! position between two choices of a switch means nothing.
    virtual bool applyParameterAutomation(const std::string & key, ParameterAutomation::Mode mode, double value) = 0;

    //! Puts every automated parameter back to its authored value. Modulation stays.
    virtual void clearParameterAutomation() = 0;

    //! Modulates the named parameter around its unmodulated value: see ParameterModulation. Called
    //! on the audio thread. False, and nothing done, when there is no such continuous parameter.
    virtual bool applyParameterModulation(const std::string & key, ParameterModulation::Mode mode, double source, double depth) = 0;
    //! Takes a modulation off the named parameter, leaving whatever it was modulated around.
    virtual void clearParameterModulation(const std::string & key) = 0;
};

} // namespace noteahead

#endif // PARAMETER_AUTOMATION_TARGET_HPP
