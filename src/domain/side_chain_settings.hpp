// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef SIDE_CHAIN_SETTINGS_HPP
#define SIDE_CHAIN_SETTINGS_HPP

#include <chrono>
#include <cstdint>
#include <vector>

namespace noteahead {

struct SideChainSettings
{
    //! Enable/disable the side-chain functionality.
    bool enabled { false };

    //! Side-chain source track.
    uint8_t sourceTrackIndex { 0 };
    //! Side-chain source column.
    uint8_t sourceColumnIndex { 0 };

    //! Time before the source event.
    std::chrono::milliseconds lookahead { 0 };
    //! Time after the source event.
    std::chrono::milliseconds release { 0 };

    //! MIDI CC controller target definitions.
    struct Target
    {
        bool enabled { false };
        uint8_t controller { 0 };
        uint8_t targetValue { 0 };
        uint8_t releaseValue { 0 };
    };
    std::vector<Target> targets;
};

} // namespace noteahead

#endif // SIDE_CHAIN_SETTINGS_HPP
