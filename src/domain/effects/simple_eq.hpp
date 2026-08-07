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

#ifndef SIMPLE_EQ_HPP
#define SIMPLE_EQ_HPP

#include "../dsp/svf_filter.hpp"
#include "effect.hpp"

#include <cstdint>

namespace noteahead {

//! A one-knob "sounds good" EQ. The single Amount control sweeps a fixed loudness-smile contour:
//! a low-shelf body boost, a gentle low-mid mud scoop and a high-shelf air lift. At zero the effect
//! is flat (all stages bypass); turning it up scales all three moves together.
class SimpleEq : public Effect
{
public:
    SimpleEq();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;

private:
    //! A stereo pair of identical filters, one per channel, processed as dual mono.
    struct StereoFilter
    {
        SvfFilter left;
        SvfFilter right;

        void reset()
        {
            left.reset();
            right.reset();
        }
    };

    void syncParameters();
    void updateBuffers();
    void processStereo(double & left, double & right);

    StereoFilter m_body;
    StereoFilter m_mud;
    StereoFilter m_air;

    bool m_shouldSyncParameters { false };
    bool m_shouldUpdateBuffers { false };
    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // SIMPLE_EQ_HPP
