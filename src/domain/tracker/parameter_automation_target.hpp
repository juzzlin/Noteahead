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

#include "modulation_source_id.hpp"
#include "parameter_automation.hpp"
#include "parameter_modulation.hpp"

#include <string>
#include <vector>

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

    //! Modulates the named parameter around its unmodulated value for @p sourceId: see
    //! ParameterModulation. Sources on the same parameter add up. Called on the audio thread. False,
    //! and nothing done, when there is no such continuous parameter or it takes no more sources.
    virtual bool applyParameterModulation(const std::string & key, ModulationSourceId sourceId, ParameterModulation::Mode mode, double source, double depth) = 0;
    //! Takes @p sourceId's modulation off the named parameter, leaving any other source's.
    virtual void clearParameterModulation(const std::string & key, ModulationSourceId sourceId) = 0;

    //! The parameters most worth riding live, best first, for a modulation source aimed here to take
    //! in turn. Neighbours make a pair, as an XY pad's X and Y take them: cutoff, then resonance.
    //! An entry also names the parameter of that name on every pad or voice of a device that has
    //! them: "cutoff" is "Kick_cutoff" as well. Parameters not listed follow in the order the
    //! dialogs list them. None by default.
    virtual std::vector<std::string> modulationPriority() const;
};

} // namespace noteahead

#endif // PARAMETER_AUTOMATION_TARGET_HPP
