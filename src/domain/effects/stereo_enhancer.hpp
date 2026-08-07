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

#ifndef STEREO_ENHANCER_HPP
#define STEREO_ENHANCER_HPP

#include "../dsp/svf_filter.hpp"
#include "effect.hpp"

namespace noteahead {

//! Psychoacoustic EQ: lifts bass, midrange and top without the level going up with them.
//!
//! Where an equalizer boosts a band by turning it up, this one takes each band out of the signal,
//! works on it, and adds it back. Bass is saturated as it goes, so what returns is the harmonics of
//! the low end rather than more low end: those harmonics let a small speaker imply a fundamental it
//! cannot reproduce, and cost far less headroom than the fundamental itself would. The midrange is
//! taken the other way, dipped rather than lifted, because pulling the middle back is what opens up
//! the two ends. The top is added back as a band of its own.
//!
//! Spread widens what is already off-centre by working on the side signal alone, so a mono source
//! stays mono however far it is turned up.
class StereoEnhancer : public Effect
{
public:
    StereoEnhancer();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void reset() override;
    void sync() override;

private:
    void syncParameters();
    void updateFilters();

    float m_bassGain { 0.0f };
    float m_bassFrequency { 0.5f };
    float m_midGain { 0.0f };
    float m_midQ { 0.5f };
    float m_highGain { 0.0f };
    float m_highFrequency { 0.5f };
    float m_outputDb { 0.0f };
    float m_spread { 0.0f };

    //! Band taps, which carry only their own band and are summed back onto the dry signal.
    SvfFilter m_bassTapL;
    SvfFilter m_bassTapR;
    SvfFilter m_highTapL;
    SvfFilter m_highTapR;

    //! The midrange is shaped in place rather than tapped, since it is a dip.
    SvfFilter m_midL;
    SvfFilter m_midR;

    double m_lastSampleRate { -1.0 };
    bool m_coefficientsDirty { true };
};

} // namespace noteahead

#endif // STEREO_ENHANCER_HPP
