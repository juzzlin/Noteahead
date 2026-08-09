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

#ifndef BASS_GRINDER_HPP
#define BASS_GRINDER_HPP

#include "../dsp/dc_blocker.hpp"
#include "../dsp/svf_filter.hpp"
#include "effect.hpp"

#include <memory>

namespace noteahead {

class Decimator;
class Upsampler;

//! Bass preamp distortion, for making a kick or a bass beefy and mean at once.
//!
//! The other nonlinear effects in the rack all shape the full band, which is why none of them can do
//! this: pushing one hard enough to get grind on top also shapes the fundamental, and the weight
//! that made the sound worth distorting goes with it. Split takes the low band out of the clipper
//! entirely -- the fundamental comes through untouched while everything above it is hammered -- and
//! Blend sets how much of the clipped band replaces the clean one, the way a bass preamp's blend
//! control does. At the bottom of its travel Split sits below the audio band, so the effect
//! degenerates to a plain full-band blend and behaves like the pedal it is modelled on.
//!
//! The split is applied twice, once on each side of the clipper. Going in it decides what gets
//! distorted; coming out it stops the clipper putting anything back below the corner, which matters
//! because what leaks past the split returns at nearly the opposite phase and would otherwise cancel
//! the fundamental the split exists to protect.
//!
//! The clipper is an asymmetric diode pair rather than the tanh the Drive and Saturator modes use.
//! Its exponential knee is softer at low level and harder at the ceiling, the two halves clip at
//! different thresholds, and a standing bias ahead of the drive gain keeps the duty cycle off half
//! however hard it is pushed, so it generates the even harmonics that read as weight instead of fizz
//! across the whole range of the control. Two consequences, both handled below: the asymmetry leaves
//! a DC offset that has to be blocked rather than passed on, and the harmonics alias badly unless the
//! shaping runs oversampled.
//!
//! Behind the clipper sits the preamp's tone stack: Color, a fixed scooped-smile voicing, and a
//! three-band EQ with a sweepable mid.
class BassGrinder : public Effect
{
public:
    BassGrinder();
    ~BassGrinder() override;

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void reset() override;
    void sync() override;

    //! How hard the clipper is currently working, in dB, for the dialog's meter. Negative.
    float saturationDb() const;

private:
    void syncParameters();

    //! Asymmetric diode pair. Asymptotes to the forward threshold of whichever half the input is on,
    //! so it needs no clamp behind it.
    double clip(double x) const;

    //! A stereo pair of identical filters, one per channel, processed as dual mono.
    struct StereoFilter
    {
        SvfFilter left;
        SvfFilter right;

        void reset();
    };

    //! Recomputes the base-rate tone stack. The split filter is left out: it runs at the oversampled
    //! rate and is recalculated per frame against the current factor.
    void updateToneStack();

    //! Recalculates the split, which runs at the oversampled rate and so has to follow the factor as
    //! well as the corner. Does nothing while neither has moved.
    void updateSplit(double splitSampleRate);

    //! One channel through the split, the clipper and the blend, at the oversampled rate.
    double grind(SvfFilter & split, SvfFilter & postSplit, double sample, double driveLin, double quiescent);

    float m_drive { 0.0f };
    float m_blend { 1.0f };
    float m_splitFreq { 20.0f };
    bool m_color { false };
    float m_bassGainDb { 0.0f };
    float m_midGainDb { 0.0f };
    float m_midFreq { 500.0f };
    float m_highGainDb { 0.0f };
    float m_mix { 1.0f };
    float m_outputDb { 0.0f };

    SvfFilter m_splitL;
    SvfFilter m_splitR;

    //! The same corner again behind the clipper, so nothing it generates below the split gets out.
    SvfFilter m_postSplitL;
    SvfFilter m_postSplitR;

    DcBlocker m_dcBlockerL;
    DcBlocker m_dcBlockerR;

    StereoFilter m_colorScoop;
    StereoFilter m_colorLow;
    StereoFilter m_colorHigh;
    StereoFilter m_bass;
    StereoFilter m_mid;
    StereoFilter m_high;

    //! Peak in and out of the clipper for the frame being processed, and the smoothed ratio the meter
    //! reads. Reset per frame by processSample().
    double m_peakPre { 0.0 };
    double m_peakPost { 0.0 };
    double m_saturationDb { 0.0 };

    bool m_shouldUpdateToneStack { true };
    double m_lastSampleRate { -1.0 };

    //! What the split's coefficients were last calculated for.
    double m_splitCoeffFreq { -1.0 };
    double m_splitCoeffSampleRate { -1.0 };

    struct Oversampling;
    std::unique_ptr<Oversampling> m_oversampling;
};

} // namespace noteahead

#endif // BASS_GRINDER_HPP
