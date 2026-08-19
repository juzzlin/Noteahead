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

#ifndef DIMENSION_HPP
#define DIMENSION_HPP

#include "../dsp/micro_pitch_shifter.hpp"
#include "../dsp/svf_filter.hpp"
#include "effect.hpp"

namespace noteahead {

//! Makes width where there is none, by building a side signal out of the middle of the mix.
//!
//! A width control can only spread what is already off-centre. Given a mono synth, a close-miked
//! part, or anything else that arrives down the middle, it has nothing to work with and does
//! nothing. This takes the centre itself, detunes one copy a few cents up and another the same
//! few cents down, and adds the difference between them as side.
//!
//! Working on the difference is what makes the result free. The two copies appear in the side
//! signal with opposite signs and nowhere else, so summing the output back to mono returns exactly
//! the mid that came in: the widening cancels itself perfectly, and there is no setting of any
//! control here that can cost a mono listener anything. That is the opposite of how width is
//! usually bought.
//!
//! Detune sets how far apart the two copies are. A few cents reads as one source made larger; past
//! about fifteen it starts to read as two sources, which is a chorus rather than a widening.
//!
//! Low Cut keeps the bottom out of it. Spreading low frequencies spends headroom on something the
//! ear cannot place anyway, and it is the first thing to muddy a mix.
class Dimension : public Effect
{
public:
    Dimension();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void reset() override;
    void sync() override;

private:
    void updateState();
    void syncParameters();

    MicroPitchShifter m_shifterUp;
    MicroPitchShifter m_shifterDown;

    //! Applied to the mid before the shifters, so what they spread has no bottom in it.
    SvfFilter m_lowCut;

    float m_detuneCents { 7.0f };
    float m_amount { 0.0f };
    float m_lowCutHz { 120.0f };

    bool m_shouldSyncParameters { false };
    double m_lastSampleRate { -1.0 };
};

} // namespace noteahead

#endif // DIMENSION_HPP
