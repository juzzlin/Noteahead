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

#ifndef DRUM_VOICE_PARAMETER_DISPLAY_HPP
#define DRUM_VOICE_PARAMETER_DISPLAY_HPP

#include <string>

namespace noteahead {

class ParameterContainer;

//! Describes the parameters of the drum voice whose keys begin with @p prefix, so that a parameter
//! automation offers them in the units the drum dialogs show. Shared by both drum synths, whose
//! voices are built from the same parameter names; a name the voice does not have is skipped.
void describeDrumVoiceParameters(ParameterContainer & container, const std::string & prefix);

} // namespace noteahead

#endif // DRUM_VOICE_PARAMETER_DISPLAY_HPP
