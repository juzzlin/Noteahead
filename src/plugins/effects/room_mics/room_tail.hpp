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

#ifndef ROOM_TAIL_HPP
#define ROOM_TAIL_HPP

#include "../../../domain/dsp/one_pole_filter.hpp"
#include "room_acoustics.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace noteahead {

//! The late reverberation of a room: what follows the early reflections once they arrive too
//! densely to tell apart.
//!
//! An eight-line feedback network mixed by a Hadamard matrix, which loses no energy of its own, so
//! that only the absorption in each line decides the decay. The absorption is a gain and two
//! shelves, set per line from the room's reverberation time in the low, middle and high bands.
//!
//! Its level is not a setting but follows from the room: the diffuse field a unit impulse leaves
//! has a known energy at every moment (RoomAcoustics::diffuseEnergyRate), the network stores what
//! it is given and hands it out at a rate set by its mean line length, and the input is scaled so
//! that the two agree from where the tail takes over. The input is delayed to that point, and
//! tilted by what each band has lost to the room by then.
//!
//! A diffuse field is the same everywhere in a room, so every mic hears the same tail, scaled only
//! by how much of a diffuse field its pattern picks up.
class RoomTail
{
public:
    static constexpr size_t LineCount { 8 };

    //! Sizes everything for @p sampleRate, which allocates.
    void configure(double sampleRate);
    bool isConfigured() const;
    void reset();

    //! Sets the tail up for the room @p settings describes, taking over at @p startSeconds.
    void set(const RoomAcoustics::RoomSettings & settings, const RoomAcoustics::Bands & reverbTimes, double startSeconds);

    //! One frame of the two sources in, the left and right of the diffuse field out.
    void process(double left, double right, double & outLeft, double & outRight);

    //! Whether there is still signal in the network.
    bool isRinging() const;

private:
    struct Line
    {
        std::vector<double> buffer;
        size_t length { 1 };
        size_t writePos { 0 };
        double gain { 1.0 };
        double lowShelf { 1.0 };
        double highShelf { 1.0 };
        OnePoleFilter low;
        OnePoleFilter high;
    };

    struct Input
    {
        std::vector<double> buffer;
        size_t writePos { 0 };
        OnePoleFilter low;
        OnePoleFilter high;
    };

    double m_sampleRate { 0.0 };
    std::array<Line, LineCount> m_lines;
    std::array<Input, 2> m_inputs;
    size_t m_preDelay { 0 };
    double m_inputGain { 0.0 };
    double m_inputLowShelf { 1.0 };
    double m_inputHighShelf { 1.0 };
    //! The loudest output lately, falling away, and how long the input has been silent: what is in
    //! the pre-delay has not reached the lines yet.
    double m_peak { 0.0 };
    size_t m_silentFrames { static_cast<size_t>(-1) / 2 };
};

} // namespace noteahead

#endif // ROOM_TAIL_HPP
