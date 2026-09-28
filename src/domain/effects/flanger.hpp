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

#ifndef FLANGER_HPP
#define FLANGER_HPP

#include "../dsp/delay_line.hpp"
#include "../dsp/lfo.hpp"
#include "effect.hpp"

namespace noteahead {

//! A swept short delay summed with the dry signal: the jet whoosh the Phaser's own comment names as
//! the thing a phaser is not.
//!
//! The delay is what separates the three effects in this family. A fixed time offset combs at
//! harmonically spaced notches, hundreds of them, and sweeping that offset drags the whole harmonic
//! series up and down together -- which the ear follows as a single moving resonance rather than as
//! a filter. The Phaser's notches come from all-pass sections instead, so there are as many as it
//! has stages and they are not harmonically related; the Chorus uses the same delay as this but far
//! longer and with no feedback, so its copies are heard as separate voices rather than as a comb.
//!
//! Feedback sharpens the peaks between the notches and is deliberately bipolar. The two polarities
//! comb at different frequencies -- positive reinforces where negative cancels -- and the negative
//! side is the hollower, more metallic of the two. Reaching one of them only would be missing half
//! the effect.
class Flanger : public Effect
{
public:
    Flanger();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void sync() override;
    void setBpm(float bpm) override;
    void reset() override;

    //! Largest factor the Rate Divider offers. As the Phaser's and the Auto Panner's.
    static int maxRateDivider();

private:
    void updateLfoFrequency();
    void updateSampleRateDependents();

    DelayLine m_delayLeft;
    DelayLine m_delayRight;
    Lfo m_lfoLeft;
    Lfo m_lfoRight;

    double m_sampleRate { 0.0 };

    double m_rate { 0.3 };
    Lfo::Mode m_lfoMode { Lfo::Mode::Normal };
    int m_rateDivider { 1 };
    double m_depth { 0.6 };
    //! Shortest delay the sweep reaches, in milliseconds. The sweep runs from here upwards, so this
    //! is where the comb's first notch sits at its highest.
    double m_delayMs { 1.0 };
    //! Signed: the sign is as much of the voicing as the amount.
    double m_feedback { 0.0 };
    double m_stereoPhase { 0.0 };

    //! What the feedback path is carrying, per channel.
    double m_feedbackLeft { 0.0 };
    double m_feedbackRight { 0.0 };
};

} // namespace noteahead

#endif // FLANGER_HPP
