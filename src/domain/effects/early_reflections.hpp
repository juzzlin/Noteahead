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

#ifndef EARLY_REFLECTIONS_HPP
#define EARLY_REFLECTIONS_HPP

#include "../dsp/one_pole_filter.hpp"
#include "../dsp/svf_filter.hpp"
#include "effect.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace noteahead {

//! Puts a sound in a room without putting a tail on it: the handful of distinct reflections that
//! arrive before a reverb has become a wash.
//!
//! Depth and width are separate axes, and a reverb only really addresses one of them. What tells
//! the ear how far away something is, is not how long the tail is but what arrives in the first
//! eighty milliseconds -- the pattern of early reflections off the nearest surfaces, how long the
//! first one takes to come back, and how much of its top end the air and the walls have taken. A
//! long tail turned down low is a poor substitute: it fills the gaps between notes rather than
//! placing the notes themselves, which is why more reverb so often buys atmosphere without buying
//! distance.
//!
//! Sixteen taps per channel, at different times on the left and the right so the pattern itself
//! carries the stereo image rather than a width control having to invent one. The gaps between
//! them vary by nearly ten to one and none is a near-repeat of another, because a tap train at even
//! spacing is a comb filter: the first version of this shipped with gaps within a third of each
//! other, which put a resonance every 180 Hz across the whole spectrum and sang like a mosquito on
//! anything sustained.
//!
//! Irregular taps alone are not enough. Sixteen discrete arrivals is a fraction of what a room
//! returns in its first eighty milliseconds, and too sparse to hear as a space rather than as an
//! effect, so the taps are smeared by a chain of Schroeder all-passes. That is what turns a handful
//! of echoes into something with density, and density is most of what reads as depth.
//!
//! Damping is applied per tap rather than to their sum. With no feedback for it to compound in, a
//! single filter on the output would darken every reflection equally, which is not what a room does
//! and not what the ear listens for: losing the top with distance is one of the cues this effect
//! exists to provide, so the later a reflection arrives the more of its top it has lost.
//!
//! The tap network itself has no feedback: every reflection is heard once and the pattern is over
//! when the last tap has been. The all-passes are recursive, as Schroeder sections must be, so the
//! response does ring on a little past the taps -- but only a little, and that is the price of the
//! density. Anything longer belongs to a reverb, which does tails better than a tap cloud can.
class EarlyReflections : public Effect
{
public:
    EarlyReflections();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void reset() override;
    void sync() override;

    static constexpr size_t NumTaps = 16;
    static constexpr size_t NumDiffusers = 4;

private:
    //! Schroeder all-pass in the Freeverb form, as the reverbs use it: passes everything through
    //! but scatters it in time, which is what gives a sparse tap cloud its density.
    struct Allpass
    {
        std::vector<double> buffer;
        uint32_t writePos { 0 };
        uint32_t size { 0 };
        double coeff { 0.0 };

        double process(double input);
        void reset();
    };

    //! One channel's delay line and the filters its reflections pass on the way out.
    struct Channel
    {
        std::vector<double> buffer;
        uint32_t writePos { 0 };
        //! Absorption, one filter per tap. A reflection arriving late has bounced off more
        //! surfaces and pushed through more air than an early one, so it comes back darker: the
        //! later the tap, the lower its corner.
        std::array<OnePoleFilter, NumTaps> damping {};
        //! Keeps the reflections out of the bottom, where they only muddy what is already there.
        SvfFilter lowCut;

        //! Scatters the taps into a continuous cloud.
        std::array<Allpass, NumDiffusers> diffusers;

        void reset();
    };

    void updateState();
    void syncParameters();
    void updateBuffers();

    //! Sums this channel's taps, each through its own absorption filter. Writes first and reads
    //! behind, so the shortest tap is still a tap.
    double renderTaps(Channel & channel, const std::array<uint32_t, NumTaps> & taps,
                      const std::array<double, NumTaps> & gains, double input) const;

    //! Gives one channel's taps their corners, falling as the arrival time rises.
    static void updateDamping(Channel & channel, const std::array<double, NumTaps> & times,
                              double dampedHz, double sampleRate);

    //! Runs the tap sum through this channel's all-passes. A diffusion of zero skips them, since a
    //! Schroeder section at a coefficient of zero is still a delay and would not be a bypass.
    double diffuse(Channel & channel, double input) const;

    Channel m_left;
    Channel m_right;

    std::array<uint32_t, NumTaps> m_tapSamplesLeft {};
    std::array<uint32_t, NumTaps> m_tapSamplesRight {};

    //! Divides the summed taps back to roughly the level that went in, so that Mix means the same
    //! thing whatever the room is set to.
    double m_tapNormalization { 1.0 };

    float m_size { 0.4f };
    float m_preDelayMs { 12.0f };
    float m_damping { 0.4f };
    float m_width { 1.0f };
    float m_lowCutHz { 150.0f };
    float m_diffusion { 0.7f };

    bool m_shouldSyncParameters { false };
    double m_lastSampleRate { -1.0 };
};

} // namespace noteahead

#endif // EARLY_REFLECTIONS_HPP
