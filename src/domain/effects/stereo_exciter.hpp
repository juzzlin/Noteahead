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

#ifndef STEREO_EXCITER_HPP
#define STEREO_EXCITER_HPP

#include "../dsp/svf_filter.hpp"
#include "effect.hpp"

#include <memory>

namespace noteahead {

class Decimator;
class Upsampler;

//! Aural exciter: generates harmonics of the upper band and adds them back, for presence and
//! intelligibility that an equalizer cannot give.
//!
//! An equalizer can only turn up what is already there. Where a recording has no top end to lift --
//! a dull room, a muffled source -- there is nothing for a shelf to find. This takes the band above
//! Tune, distorts it, and adds the harmonics that produces. Those harmonics are higher than
//! anything in the band that made them, so the result reads as air and detail that was never in the
//! signal, and it survives a small speaker better than a shelf does because the ear infers the
//! missing fundamental from them.
//!
//! Timbre decides the character of what is generated: an odd-symmetric curve returns odd harmonics,
//! which read as edge and bite, and an asymmetric one returns even harmonics, which read as warmth.
//! The control blends between the two rather than switching.
//!
//! The shaping runs oversampled. Harmonics of a band that already reaches several kilohertz land
//! above Nyquist otherwise, and fold back down as inharmonic tones -- the opposite of the clarity
//! the effect is for.
class StereoExciter : public Effect
{
public:
    StereoExciter();
    ~StereoExciter() override;

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void reset() override;
    void sync() override;

    //! Level of the harmonics currently being generated, in dB, for the dialog's meter.
    float harmonicsDb() const;

protected:
    void processSample(double & left, double & right) override;

private:
    void syncParameters();
    void updateFilters();

    //! The side chain: the band the harmonics are generated from.
    double sideChain(SvfFilter & steep, SvfFilter & gentle, double input) const;

    //! Odd and even shaping, blended by Timbre.
    double shape(double value) const;

    float m_tune { 0.5f };
    float m_peak { 0.0f };
    float m_zeroFill { 0.0f };
    float m_timbre { 0.5f };
    float m_harmonics { 0.0f };

    SvfFilter m_steepL;
    SvfFilter m_steepR;
    SvfFilter m_gentleL;
    SvfFilter m_gentleR;

    double m_harmonicsDb { 0.0 };
    double m_lastSampleRate { -1.0 };
    bool m_coefficientsDirty { true };

    struct Oversampling;
    std::unique_ptr<Oversampling> m_oversampling;
};

} // namespace noteahead

#endif // STEREO_EXCITER_HPP
