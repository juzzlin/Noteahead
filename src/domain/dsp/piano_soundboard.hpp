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

#ifndef PIANO_SOUNDBOARD_HPP
#define PIANO_SOUNDBOARD_HPP

#include <array>
#include <bitset>
#include <cstdint>
#include <vector>

namespace noteahead {

//! What a grand does besides the strings that are struck.
//!
//! The hammer knocks the whole instrument: the soundboard and the rim ring for a fraction of a second
//! at their own low resonances, which is the thump under every note and most of what a treble note
//! has before its strings speak.
//!
//! And every string whose damper is up answers the ones that are sounding: lifting the sustain pedal
//! lifts them all, and the top keys have no dampers at all. Each such string is a comb filter tuned
//! to it, fed with what the soundboard carries.
class PianoSoundboard
{
public:
    static constexpr int LowestNote = 21;
    static constexpr int HighestNote = 108;
    static constexpr int StringCount = HighestNote - LowestNote + 1;

    void setSampleRate(double sampleRate);
    //! Tunes the sympathetic strings to the keys' own tuning: @p stretch is how much of the
    //! recording's stretch to take, and @p decayScale scales how long they ring
    void setTuning(double stretch, double decayScale, double panSpread);

    //! The hammer strikes @p note with @p strength
    void knock(uint8_t note, double strength);

    //! Which strings are free to ring: the dampers that are up, less the keys that are sounding, which
    //! are strings of their own
    void setOpenStrings(const std::bitset<128> & open);

    //! Adds the soundboard to @p left and @p right. @p strings is what the struck strings put into
    //! it, @p knockLevel and @p resonance how much of each is heard
    void process(double strings, double knockLevel, double resonance, double & left, double & right);

    bool isActive() const;
    void reset();

private:
    static constexpr int BodyModes = 12;

    struct Comb
    {
        std::vector<double> line;
        int write { 0 };
        double delay { 0.0 };
        double openGain { 0.0 };
        double closedGain { 0.0 };
        double lowpass { 0.0 };
        double energy { 0.0 };
        double pan { 0.5 };
    };

    void tune();

    std::array<double, BodyModes> m_bodyA1 {};
    std::array<double, BodyModes> m_bodyA2 {};
    std::array<double, BodyModes> m_bodyY1 {};
    std::array<double, BodyModes> m_bodyY2 {};

    std::array<Comb, StringCount> m_combs {};
    std::bitset<128> m_open;

    double m_sampleRate { 0.0 };
    double m_stretch { 1.0 };
    double m_decayScale { 1.0 };
    double m_panSpread { 0.6 };
    double m_bodyEnergy { 0.0 };
    bool m_combsActive { false };
};

} // namespace noteahead

#endif // PIANO_SOUNDBOARD_HPP
