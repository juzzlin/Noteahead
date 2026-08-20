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

#ifndef ANALOG_FUZZ_HPP
#define ANALOG_FUZZ_HPP

#include "../dsp/dc_blocker.hpp"
#include "../dsp/saturating_svf.hpp"
#include "effect.hpp"

#include <memory>

namespace noteahead {

class Decimator;
class Upsampler;

//! An analog synth's drive: a fuzz stage played through the filter it is overdriving.
//!
//! Every other nonlinear effect in the rack is a static shaper with filters around it, and that is
//! why none of them sounds like the drive knob on a monosynth. There the distortion is not a box in
//! front of the filter, it is a stage pushing the VCF itself past what it can take, and the filter
//! that rounds the result off is the same one whose resonance is being fed by it. This effect is
//! built that way round: Drive and Fuzz shape the signal, and what comes out is run into a two-pole
//! filter whose integrators saturate (SaturatingSvf).
//!
//! What that arrangement gives, and a distortion chained into Auto Filter does not:
//!
//! - Resonance that gives way under drive. The peak is bounded by the same ceiling the harmonics
//!   are, so pushing Drive squashes it instead of making it scream. Backing off opens it again.
//! - Harmonics that come out filtered rather than laid on top, because they are made inside the
//!   loop and pass the poles on the way round.
//!
//! Fuzz morphs the drive stage from a soft valve-like knee to a hard, nearly square clip, and Bias
//! sets where on the curve the signal sits: centred is odd harmonics and a hollow woody tone, off to
//! either side brings in the even ones, and far enough out the stage starves and gates the way a
//! fuzz on a dying battery does. Two consequences of the asymmetry, both handled here: it leaves DC
//! on the output, which is blocked rather than passed on, and it generates harmonics that alias
//! badly unless the shaping and the filter both run oversampled.
class AnalogFuzz : public Effect
{
public:
    AnalogFuzz();
    ~AnalogFuzz() override;

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void reset() override;
    void sync() override;

    //! How hard the drive stage is currently working, in dB, for the dialog's meter. Negative.
    float saturationDb() const;

private:
    void syncParameters();

    //! The drive stage. Morphs between a tanh knee and a hard clip by Fuzz, around the operating
    //! point Bias sets.
    double shape(double x) const;

    //! One channel: shape, then the filter it is overdriving. Runs at the oversampled rate.
    double fuzz(SaturatingSvf & filter, double sample, double driveLin, double filterDrive, double & peakPre, double & peakPost);

    float m_drive { 0.4f };
    float m_fuzz { 0.5f };
    float m_bias { 0.5f };
    float m_cutoff { 4200.0f };
    float m_resonance { 0.25f };
    float m_mix { 1.0f };
    float m_outputDb { 0.0f };

    //! Bias as an offset on the shaper's input, and the output it sits at with no signal. The
    //! quiescent point is subtracted so the stage idles at zero however far the bias is pushed.
    double m_biasOffset { 0.0 };
    double m_quiescent { 0.0 };

    SaturatingSvf m_filterL;
    SaturatingSvf m_filterR;

    DcBlocker m_dcBlockerL;
    DcBlocker m_dcBlockerR;

    double m_saturationDb { 0.0 };

    struct Oversampling;
    std::unique_ptr<Oversampling> m_oversampling;
};

} // namespace noteahead

#endif // ANALOG_FUZZ_HPP
