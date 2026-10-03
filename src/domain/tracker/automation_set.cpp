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

#include "automation_set.hpp"

#include <algorithm>

namespace noteahead {

namespace {

template<typename Automation>
void mergeInto(std::vector<Automation> & into, const std::vector<Automation> & from)
{
    for (auto && automation : from) {
        if (std::ranges::find(into, automation) == into.end()) {
            into.push_back(automation);
        }
    }
}

} // namespace

bool AutomationSet::empty() const
{
    return midiCc.empty() && pitchBend.empty() && parameter.empty();
}

void AutomationSet::merge(const AutomationSet & other)
{
    mergeInto(midiCc, other.midiCc);
    mergeInto(pitchBend, other.pitchBend);
    mergeInto(parameter, other.parameter);
}

} // namespace noteahead
