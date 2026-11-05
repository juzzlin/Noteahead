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

#ifndef OPTO_COMPRESSOR_CORE_HPP
#define OPTO_COMPRESSOR_CORE_HPP

#include "dsp_component.hpp"

namespace noteahead {

//! The side chain of an optical leveling amplifier after the LA-2A: a lamp lit by the signal and a
//! light-dependent resistor that follows it, built from the unit's published behaviour.
//!
//! There is no threshold or ratio to set. Peak Reduction is the side chain's gain into a fixed
//! threshold, so turning it up both lowers where compression starts and deepens it; Limit makes
//! the curve steeper and its knee narrower.
//!
//! What sets it apart is the cell. It pulls down in about 10 ms, and lets go in two stages: about
//! half of the reduction within some 60 ms, the rest over seconds. How much is left for the slow
//! stage depends on how long the cell has been lit, so a short peak is released at once and a long
//! loud passage is let go of slowly -- the program dependence the unit is known for.
class OptoCompressorCore : public DspComponent
{
public:
    //! 0..1. Zero takes the side chain out altogether.
    void setPeakReduction(double position);
    void setLimit(bool limit);

    //! Advances the lamp and the cell by one frame and returns the gain to apply, in dB. Never
    //! positive.
    double processGainDb(double left, double right);

    //! Gain currently applied, in dB, for metering. Zero when nothing is being held down.
    double reductionDb() const;

    //! The static curve: the reduction, in positive dB, the cell settles to for a side chain at
    //! @p levelDb before Peak Reduction.
    double targetReductionDb(double levelDb) const;

    //! Whether the cell has let go entirely.
    bool isSettled() const;

    void reset();

private:
    void updateCoefficients();

    double m_peakReduction { 0.0 };
    bool m_limit { false };

    double m_lamp { 0.0 };
    //! The cell's two stages, as reduction in positive dB.
    double m_fast { 0.0 };
    double m_slow { 0.0 };
    double m_reductionDb { 0.0 };

    double m_lampRelease { 0.0 };
    double m_fastAttack { 0.0 };
    double m_fastRelease { 0.0 };
    double m_slowCharge { 0.0 };
    double m_slowRelease { 0.0 };
    double m_lastSampleRate { -1.0 };
};

} // namespace noteahead

#endif // OPTO_COMPRESSOR_CORE_HPP
