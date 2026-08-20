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

#ifndef SATURATING_SVF_HPP
#define SATURATING_SVF_HPP

#include "dsp_component.hpp"

namespace noteahead {

//! Two-pole state-variable filter whose integrators saturate, the way an overdriven analog VCF does.
//!
//! The linear filters in the rack are driven or filtered, never both at once: shape first and filter
//! after, and the harmonics are made outside the loop that is supposed to tame them. Here the
//! nonlinearity is inside the loop. Two things follow from that, and they are the whole reason this
//! class exists:
//!
//! - The resonant peak compresses as the input grows. The integrator states cannot exceed the
//!   saturator's ceiling, so the harder the filter is driven the less of the peak survives, and the
//!   resonance setting stops behaving like a fixed number. On a real synth that is what keeps a
//!   screaming filter from tearing the sound apart; chaining a distortion into a linear filter
//!   cannot reproduce it, because there the peak keeps its height however hard the input is pushed.
//! - Whatever the loop generates is filtered by the same two poles on the way round again, so the
//!   harmonics come out rounded rather than as the fizz a static shaper leaves on top.
//!
//! The solve stays the Topology-Preserving Transform one and only the integrator states pass through
//! tanh, so there is no delay-free loop to iterate and no setting that can make it diverge: the
//! states are bounded by the saturator whatever the input does.
class SaturatingSvf : public DspComponent
{
public:
    //! \param frequency Corner in Hz. Clamped to something the bilinear transform can still resolve.
    void setCutoff(double frequency);

    //! \param resonance 0 for the flattest response, approaching 1 for a peak on the edge of
    //! self-oscillation. What the peak actually reaches also depends on how hard the filter is being
    //! driven, which is the point of this class.
    void setResonance(double resonance);

    //! \param drive How hard the integrators are pushed into their ceiling, 1 for the level a
    //! nominal 0 dBFS signal saturates at. Below roughly 0.1 the filter is linear for any usable
    //! signal and this is an ordinary SVF.
    void setSaturation(double drive);

    //! How much of the squash to apply on each step, as a fraction.
    //!
    //! tanh is not idempotent: applying it twice is not applying it once, so a filter running at
    //! four times the rate squashes its states four times as often and compresses that much harder.
    //! That made an effect sound different at one oversampling factor than at another -- a render
    //! at 4x came out over a decibel below the same song playing at 2x. Callers that oversample
    //! pass the reciprocal of their factor here, which keeps the saturation per unit of time, and
    //! therefore the sound, the same at any rate.
    void setSaturationPerStep(double fraction);

    void setSampleRate(double sampleRate) override;

    //! One sample through the filter. Returns the low-pass tap, which is the one an overdriven VCF
    //! is played through.
    double process(double input);

    //! Taps from the most recent process() call, for callers that want another slope or a mix.
    double lowPass() const;
    double bandPass() const;
    double highPass() const;

    void reset();

private:
    void updateCoefficients();

    double m_cutoff { 1000.0 };
    double m_resonance { 0.0 };
    double m_saturation { 1.0 };
    double m_saturationPerStep { 1.0 };

    double m_lastCutoff { -1.0 };
    double m_lastResonance { -1.0 };
    double m_lastSampleRate { -1.0 };

    double m_g { 0.0 };
    double m_k { 2.0 };
    double m_a1 { 0.0 };
    double m_a2 { 0.0 };
    double m_a3 { 0.0 };

    double m_ic1eq { 0.0 };
    double m_ic2eq { 0.0 };

    double m_lowPass { 0.0 };
    double m_bandPass { 0.0 };
    double m_highPass { 0.0 };
};

} // namespace noteahead

#endif // SATURATING_SVF_HPP
