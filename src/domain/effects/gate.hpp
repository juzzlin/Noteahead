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

#ifndef GATE_HPP
#define GATE_HPP

#include "effect.hpp"

namespace noteahead {

//! A noise gate and downward expander: quiet passages are pulled further down, loud ones pass.
//!
//! The opposite operation to AutoDucker, which pulls a signal down *because another one is loud*.
//! Here the signal keys itself: what decides the gain is how loud this signal is right now, and the
//! whole point is that below a threshold it stops being heard at all.
//!
//! Ratio and Range are both needed and are not the same control. Ratio says how steeply the gain
//! falls away once the signal drops below the threshold, which is what makes the difference between
//! a gate that slams and an expander that merely leans on the quiet parts. Range says how far down
//! it is allowed to go at all, so a drum track can be tightened by 12 dB rather than silenced.
//!
//! Hold and hysteresis are what stop it chattering. A signal sitting near the threshold crosses it
//! many times a second, and a gate with neither opens and shuts on every crossing -- which is heard
//! as a rattle rather than as gating. Hold keeps it open for a set time after the signal falls away;
//! the hysteresis closes at a lower level than it opens at, so a wobble around the open threshold
//! cannot reach the close one.
class Gate : public Effect
{
public:
    Gate();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void sync() override;
    void reset() override;

    //! Gain currently applied, in dB, for metering. Zero while the gate is fully open.
    float gainDb() const;

private:
    void updateCoefficients();
    //! What the expander asks for at a given detector level, in dB. Never positive.
    double targetGainDb(double detectorDb) const;

    double m_sampleRate { 0.0 };

    double m_thresholdDb { -40.0 };
    double m_ratio { 8.0 };
    double m_rangeDb { 60.0 };
    double m_attackMs { 1.0 };
    double m_holdMs { 50.0 };
    double m_releaseMs { 100.0 };

    double m_attackCoefficient { 0.0 };
    double m_releaseCoefficient { 0.0 };
    double m_detectorCoefficient { 0.0 };

    //! Peak detector, in linear amplitude. Rises instantly and falls at the detector coefficient:
    //! a gate that averaged its input would open late on a transient, which is the one thing it
    //! must not do on a drum.
    double m_detector { 0.0 };
    double m_gainDb { 0.0 };
    //! Frames the gate is still held open for, counting down.
    size_t m_holdCounter { 0 };
    //! Whether the gate is currently open, which is what makes the hysteresis a hysteresis: the
    //! level it closes at is only consulted while it is open.
    bool m_open { false };
};

} // namespace noteahead

#endif // GATE_HPP
