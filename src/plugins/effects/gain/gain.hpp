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

#ifndef GAIN_HPP
#define GAIN_HPP

#include "../utility/clip_detector.hpp"
#include "effect.hpp"

namespace noteahead {

//! A plain level trim, +/-24 dB, and nothing else.
//!
//! Every other way of changing level in this rack also changes the sound: a limiter is dynamics, a
//! drive or a saturator is distortion. This is the one that only makes it louder or quieter, which
//! is what gain staging into the rest of the chain needs.
//!
//! Carries a clip indicator because a boost is the thing most likely to push the bus past full
//! scale, and the master bus has no meter of its own watching for it.
class Gain : public Effect
{
public:
    Gain();

    static std::string typeIdString();

    std::string type() const override;
    std::string typeId() const override;

    void sync() override;

    //! Trim in dB, which is what the control reads.
    float gainDb() const;

    ClipDetector & clipDetector();
    const ClipDetector & clipDetector() const;

protected:
    void processSample(double & left, double & right) override;

    //! Runs the trim and then looks at what came out of it.
    void processBlock(AudioContext & context) override;

private:
    float m_gainDb { 0.0f };
    double m_gain { 1.0 };
    ClipDetector m_clipDetector;
};

} // namespace noteahead

#endif // GAIN_HPP
