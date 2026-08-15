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

#ifndef PHASER_HPP
#define PHASER_HPP

#include <array>
#include <cstdint>

#include "../dsp/lfo.hpp"
#include "effect.hpp"

namespace noteahead {

//! A swept all-pass cascade, summed with the dry signal by the Mix control.
//!
//! An all-pass section passes every frequency at full level and only shifts its phase, by an amount
//! that depends on the frequency. Summed back with the dry signal, the frequencies that come out
//! half a cycle late cancel and the rest do not, so the cascade writes a comb of notches into the
//! spectrum. Sweeping the sections moves the notches, and that movement is the effect: nothing is
//! being filtered out so much as repeatedly cancelled and released.
//!
//! Notches are what distinguishes this from the Chorus and the Flanger it is often confused with.
//! A delay-based effect combs at harmonically spaced notches, hundreds of them, because the comb
//! comes from a fixed time offset. Here each notch belongs to one pair of all-pass sections, so
//! Stages is literally how many notches there are, they are not harmonically related, and the result
//! is the hollow sweep of a phaser rather than the jet whoosh of a flanger.
//!
//! Feedback returns the cascade's output to its input, which sharpens the peaks between the notches.
//! Its sign matters as much as its amount: the two polarities cancel at different frequencies and
//! voice the effect quite differently.
class Phaser : public Effect
{
public:
    Phaser();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void sync() override;
    void setBpm(float bpm) override;
    void reset() override;

    //! Most all-pass sections the cascade can run. Each pair of sections puts one notch into the
    //! spectrum, so this is also the notch count at its highest setting, doubled.
    static int maxStages();

    //! Sweep either side of the centre frequency, in octaves, at full Depth.
    static double maxSweepOctaves();

    //! How much of the cascade's output may return to its input. Short of unity, where the ringing
    //! between the notches would take over from the signal.
    static double maxFeedback();

    //! Divisions the Rate can be slowed by, so that a sweep can take minutes without the rate
    //! control having to leave the range every other LFO in the application uses.
    static int maxRateDivider();

protected:
    void processSample(double & left, double & right) override;

private:
    //! One channel's cascade. The sections share a coefficient, which is what makes the sweep a
    //! single multiply per section rather than a tan() per section.
    struct Cascade
    {
        std::array<double, 12> x1 {};
        std::array<double, 12> y1 {};
        double feedbackSample { 0.0 };

        double process(double input, int stages, double coefficient, double feedback);
        void reset();
    };

    void applyParameters();
    void updateLfoFrequency();
    void updateSampleRateDependents();
    void updateModulation();

    //! All-pass coefficient placing the section's corner at the given frequency.
    double coefficientForFrequency(double frequency) const;

    //! As in the Auto Filter: the sweep reaches the cascade this many samples apart, because a moved
    //! corner costs a tan() and a divide, and no LFO capped at 20 Hz can outrun the rate that gives.
    static constexpr uint32_t controlRateStride = 16;

    Cascade m_left;
    Cascade m_right;

    Lfo m_lfoLeft;
    Lfo m_lfoRight;

    int m_stages { 6 };
    double m_centreFrequency { 700.0 };
    double m_depth { 0.7 };
    double m_feedback { 0.0 };
    double m_rate { 0.223 };
    int m_rateDivider { 1 };
    Lfo::Mode m_lfoMode { Lfo::Mode::Normal };
    //! Right-channel LFO offset in cycles, 0 to 0.5. A quarter cycle is the classic stereo phaser.
    double m_stereoPhase { 0.25 };
    double m_gain { 1.0 };

    double m_coefficientLeft { 0.0 };
    double m_coefficientRight { 0.0 };
    double m_appliedSampleRate { 0.0 };
    uint32_t m_controlCounter { 0 };
    bool m_shouldApplyParameters { true };
};

} // namespace noteahead

#endif // PHASER_HPP
