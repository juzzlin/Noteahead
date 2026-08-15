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

#ifndef ENSEMBLE_PHASER_HPP
#define ENSEMBLE_PHASER_HPP

#include "dsp_component.hpp"
#include "lfo.hpp"

#include <array>

namespace noteahead {

//! Six-stage swept all-pass phaser with feedback, in the string machine tradition.
//!
//! A cascade of first-order all-pass sections is swept by an LFO and summed with the dry signal, so
//! the phase cancellations between the two form notches that move with the sweep. Feedback deepens
//! them. Left and right run the same cascade in quadrature, which is what turns a mono ensemble into
//! a wide, moving stereo image.
//!
//! Two controls, matching the hardware panel: Rate is the sweep speed, Color trades notch depth and
//! feedback together (a single "how phasey" knob rather than separate depth and resonance).
class EnsemblePhaser : public DspComponent
{
public:
    static constexpr int StageCount { 6 };

    EnsemblePhaser();

    void setSampleRate(double sampleRate) override;

    //! Bypasses the output while the cascade keeps running, so toggling it does not click.
    void setEnabled(bool enabled);
    bool enabled() const;

    //! 0..1, mapped to a slow-to-fast sweep.
    void setRate(double rate);
    double rate() const;

    //! 0..1, mapped to sweep depth and feedback together.
    void setColor(double color);
    double color() const;

    void process(double & left, double & right);

    void reset();

private:
    struct Channel
    {
        std::array<double, StageCount> x1 {};
        std::array<double, StageCount> y1 {};
        double feedbackSample { 0.0 };

        double process(double input, double coefficient, double feedback);
        void reset();
    };

    //! All-pass coefficient placing the pole at the given corner frequency.
    double coefficientForFrequency(double frequency) const;

    bool m_enabled { false };
    double m_rate { 0.3 };
    double m_color { 0.5 };

    Lfo m_lfoLeft;
    Lfo m_lfoRight;

    Channel m_left;
    Channel m_right;
};

} // namespace noteahead

#endif // ENSEMBLE_PHASER_HPP
