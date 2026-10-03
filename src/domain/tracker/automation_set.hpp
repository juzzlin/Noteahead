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

#ifndef AUTOMATION_SET_HPP
#define AUTOMATION_SET_HPP

#include "../midi/midi_cc_automation.hpp"
#include "../midi/pitch_bend_automation.hpp"
#include "parameter_automation.hpp"

#include <vector>

namespace noteahead {

//! Automations of every kind, held together.
//!
//! What copying, pasting and undoing pass around, so that each of them handles every kind in one
//! go: a kind added later only has to be added here and to forEach() to be carried by all of them,
//! rather than to a parallel list at every call site, where one is easily missed.
struct AutomationSet
{
    std::vector<MidiCcAutomation> midiCc;
    std::vector<PitchBendAutomation> pitchBend;
    std::vector<ParameterAutomation> parameter;

    bool empty() const;

    //! Adds every automation of @p other this set does not already hold.
    void merge(const AutomationSet & other);

    //! Calls @p function on every automation, whatever its kind. The function is called with the
    //! concrete type, so it has to be generic.
    template<typename Function>
    void forEach(Function && function)
    {
        for (auto && automation : midiCc) {
            function(automation);
        }
        for (auto && automation : pitchBend) {
            function(automation);
        }
        for (auto && automation : parameter) {
            function(automation);
        }
    }

    template<typename Function>
    void forEach(Function && function) const
    {
        for (auto && automation : midiCc) {
            function(automation);
        }
        for (auto && automation : pitchBend) {
            function(automation);
        }
        for (auto && automation : parameter) {
            function(automation);
        }
    }
};

} // namespace noteahead

#endif // AUTOMATION_SET_HPP
