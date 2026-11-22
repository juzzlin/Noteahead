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

#ifndef FET_COMPRESSOR_CORE_HPP
#define FET_COMPRESSOR_CORE_HPP

#include "dsp_component.hpp"
#include "svf_filter.hpp"

namespace noteahead {

//! Stereo-linked feedback FET gain-reduction core in the spirit of an 1176 limiting amplifier.
//!
//! Because the detector sits after the gain-reduction element, a naive one-sample-delayed feedback
//! loop would oscillate at high ratios and microsecond attacks. Instead, each frame solves the
//! implicit one-pole feedback equation in closed form in the log domain, preserving both the
//! steady-state slope and the ratio-dependent tightening of feedback attack while remaining
//! unconditionally stable at every sample rate and ratio.
//!
//! All-Buttons-In ("All") engages a steeper threshold with a short pre-clamp detector lag so
//! initial transients punch through before clamping hard, alongside a two-stage program-dependent
//! release that lets go of short peaks quickly while holding longer passages more gently.
class FetCompressorCore : public DspComponent
{
public:
    //! Persisted by index: append only.
    enum class Ratio
    {
        FourToOne,
        EightToOne,
        TwelveToOne,
        TwentyToOne,
        All
    };

    void setRatio(Ratio ratio);
    void setAttackSeconds(double seconds);
    void setReleaseSeconds(double seconds);
    //! Sidechain high-pass corner in Hz; values <= 0 bypass the filter.
    void setSideChainHpfHz(double hz);
    //! Scales the attack time constant (e.g. slightly faster for Blue Stripe).
    void setAttackScale(double scale);

    //! Advances the sidechain filter, detector, and feedback ballistics by one frame and returns the
    //! gain reduction to apply, in dB. Never positive.
    double processGainDb(double left, double right);

    //! Current gain reduction in dB, for metering. Zero when idle.
    double reductionDb() const;

    //! Steady-state closed-loop reduction in positive dB for a pre-FET input at @p levelDb.
    double targetReductionDb(double levelDb) const;

    //! True once both release stages and the All-mode lag have returned to rest.
    bool isSettled() const;

    void reset();

private:
    struct RatioCurve
    {
        double ratio;
        double thresholdDb;
        double kneeDb;
    };

    static RatioCurve curveFor(Ratio ratio);
    void updateCoefficients();

    Ratio m_ratio { Ratio::FourToOne };
    double m_attackSeconds { 0.0002 };
    double m_releaseSeconds { 0.2 };
    double m_hpfHz { 0.0 };
    double m_attackScale { 1.0 };

    SvfFilter m_hpfL;
    SvfFilter m_hpfR;

    double m_fast { 0.0 };
    double m_slow { 0.0 };
    double m_allLag { 0.0 };
    double m_reductionDb { 0.0 };

    double m_attackCoeff { 0.0 };
    double m_fastReleaseCoeff { 0.0 };
    double m_slowChargeCoeff { 0.0 };
    double m_slowReleaseCoeff { 0.0 };
    double m_allLagCoeff { 0.0 };

    double m_lastAttackSeconds { -1.0 };
    double m_lastReleaseSeconds { -1.0 };
    double m_lastAttackScale { -1.0 };
    Ratio m_lastRatio { Ratio::FourToOne };
    double m_lastHpfHz { -1.0 };
    double m_lastSampleRate { -1.0 };
};

} // namespace noteahead

#endif // FET_COMPRESSOR_CORE_HPP
