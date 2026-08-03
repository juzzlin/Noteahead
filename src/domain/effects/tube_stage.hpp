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

#ifndef TUBE_STAGE_HPP
#define TUBE_STAGE_HPP

#include "../dsp/dc_blocker.hpp"
#include "../dsp/one_pole_filter.hpp"
#include "effect.hpp"

#include <memory>

namespace noteahead {

class Decimator;
class Upsampler;

//! Valve preamp stage, for warming a signal the way a console's tube channel does.
//!
//! What separates this from the Saturator's Tube mode is the operating point. A triode does not sit
//! symmetrically about zero: it idles at a grid bias, so the two halves of a waveform meet different
//! parts of the curve. That asymmetry is what produces even harmonics -- the second-harmonic warmth
//! a valve is actually wanted for -- where a symmetric shaper can only produce odd ones, which read
//! as harshness rather than warmth. Bias exposes that operating point directly, so the same Drive
//! setting runs anywhere from nearly symmetric to hard up against cutoff.
//!
//! Two further consequences of modelling it this way, both handled below: an asymmetric shaper puts
//! a DC offset on its output, which has to be blocked rather than passed on to the rest of the rack,
//! and the harmonics it generates alias badly unless the shaping runs oversampled.
class TubeStage : public Effect
{
public:
    enum class Mode
    {
        //! Soft, gradual knee. The classic warm preamp valve.
        Triode,
        //! Harder knee that holds its shape until it gives way, so it colours less until pushed and
        //! then colours more abruptly, with the harmonics running odd rather than even.
        Pentode
    };

    TubeStage();
    ~TubeStage() override;

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void process(double & left, double & right) override;
    void reset() override;
    void sync() override;

    //! How hard the stage is currently working, in dB, for the dialog's meter. Negative.
    float saturationDb() const;

private:
    void syncParameters();

    //! Plate output for a grid voltage already offset by the bias point.
    double shape(double v) const;

    Mode m_mode { Mode::Triode };
    float m_driveDb { 0.0f };
    float m_bias { 0.5f };
    float m_tone { 0.5f };
    float m_mix { 1.0f };
    float m_outputDb { 0.0f };

    //! Grid offset the bias control maps to, and the plate current it idles at. Subtracting the
    //! latter keeps silence silent, so changing Bias cannot thump the rack.
    double m_biasOffset { 0.0 };
    double m_quiescent { 0.0 };

    OnePoleFilter m_tiltL;
    OnePoleFilter m_tiltR;
    DcBlocker m_dcBlockerL;
    DcBlocker m_dcBlockerR;

    double m_saturationDb { 0.0 };

    struct Oversampling;
    std::unique_ptr<Oversampling> m_oversampling;
};

} // namespace noteahead

#endif // TUBE_STAGE_HPP
