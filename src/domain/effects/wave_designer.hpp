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

#ifndef WAVE_DESIGNER_HPP
#define WAVE_DESIGNER_HPP

#include "effect.hpp"

namespace noteahead {

//! Transient shaper: Attack for the leading edge of a hit, Sustain for the tail behind it.
//!
//! Level-independent by construction, which is what separates it from a compressor. Each control
//! runs two envelope followers over the same signal, one quick and one slow, and acts on the
//! difference between them. That difference only departs from zero while the signal is changing, so
//! a hit is shaped by how sharply it rises and how long it takes to fall, not by how loud it is: the
//! same setting works on a snare at any level, and a steady tone comes out untouched because its two
//! followers agree.
//!
//! Attack compares a fast follower against a medium one, which differ only during the leading edge.
//! Sustain compares a medium one against a slow one, which stay apart through the decay. Both
//! differences drive gain in dB, so turning a control down attenuates that part of the hit by as
//! much as turning it up lifts it.
class WaveDesigner : public Effect
{
public:
    WaveDesigner();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void reset() override;
    void sync() override;

    //! Gain the shaper is currently applying, in dB, for the dialog's meter.
    float shapingDb() const;

private:
    void syncParameters();

    //! One follower's coefficients, recomputed when the sample rate moves.
    struct Follower
    {
        double attackCoefficient { 0.0 };
        double releaseCoefficient { 0.0 };
        double level { 0.0 };

        double process(double rectified);
        void reset();
    };

    void updateCoefficients();

    float m_attack { 0.0f };
    float m_sustain { 0.0f };
    float m_gainDb { 0.0f };

    //! Smoothing on the rectified signal, ahead of the followers.
    double m_detector { 0.0 };
    double m_detectorCoefficient { 0.0 };

    Follower m_fast;
    Follower m_medium;
    Follower m_slow;

    double m_shapingDb { 0.0 };
    double m_lastSampleRate { -1.0 };
};

} // namespace noteahead

#endif // WAVE_DESIGNER_HPP
