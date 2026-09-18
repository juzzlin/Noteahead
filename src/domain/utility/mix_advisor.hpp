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

#ifndef MIX_ADVISOR_HPP
#define MIX_ADVISOR_HPP

#include "loudness_analyzer.hpp"
#include "spectrum_analyzer.hpp"

#include <vector>

namespace noteahead {

//! What a measured mix amounts to, as findings rather than numbers.
//!
//! SpectrumAnalyzer says where the energy sits and LoudnessAnalyzer says how loud and how peaky it
//! is. Neither says anything to a reader who does not already know what to compare them against.
//! This draws the comparisons.
//!
//! Everything about balance is judged against **the mix's own tilt**, never against a reference
//! curve. A straight line is fitted through the third-octave levels and each region is read as its
//! departure from that line, because a mix that is legitimately dark is dark in every band at once
//! and has nothing wrong with it: only the departures are audible as mud, boxiness or scoop. An
//! absolute reference would call that same mix five separate faults, which is how an analyzer earns
//! being ignored.
//!
//! Findings carry codes and numbers, never sentences. The wording lives in the application layer,
//! where it can be translated; here it would be neither translatable nor testable without matching
//! strings.
class MixAdvisor
{
public:
    enum class Topic
    {
        Tilt, //!< The overall lean. Always reported, never a fault.
        Balanced, //!< Nothing stood out. Said out loud, because an empty section reads as a bug.
        Sub, //!< Below 50 Hz.
        LowEnd, //!< 50 to 125 Hz.
        LowMid, //!< 125 to 400 Hz.
        Mid, //!< 400 Hz to 1.25 kHz.
        Presence, //!< 1.25 to 4 kHz.
        Top, //!< 4 to 10 kHz.
        Air, //!< Above 10 kHz.
        PresenceAgainstHighs, //!< The number the report already prints, said in words.
        Resonance, //!< One band standing well above both of its neighbours.
        TruePeak //!< What is left between the loudest peak and full scale.
    };

    enum class Severity
    {
        Note, //!< Worth knowing about.
        Caution //!< Worth doing something about.
    };

    //! Which way a region departs from the fitted tilt. Meaningless for topics that have no
    //! direction, such as Tilt and Resonance, where it is always Neutral.
    enum class Direction
    {
        Neutral,
        Above,
        Below
    };

    struct Finding
    {
        Topic topic { Topic::Balanced };
        Severity severity { Severity::Note };
        Direction direction { Direction::Neutral };
        //! What triggered it: dB against the fitted tilt, dBTP for TruePeak, dB/octave for Tilt.
        float valueDb { 0.0f };
        //! The band a finding is about, or zero when it is about a region or the whole mix.
        double frequencyHz { 0.0 };
    };

    struct Summary
    {
        //! Slope of the fitted line, in dB per octave, relative to pink.
        //!
        //! SpectrumAnalyzer reports power per third-octave band, so pink noise is a flat line and
        //! white noise climbs 3 dB per octave. Nearly every mix leans down across the range.
        float tiltDbPerOctave { 0.0f };
        //! Worst first, so a reader who stops after one line has read the one that mattered.
        std::vector<Finding> findings;
        //! False when the spectrum could not be measured; only loudness-side findings are present.
        bool hasBalance { false };
    };

    static Summary advise(const SpectrumAnalyzer::Result & spectrum, const LoudnessAnalyzer::Result & loudness);

    //! Slope in dB per octave of the least-squares line through @p bands, over the fitted range.
    //!
    //! Exposed for the sake of the tests, which have no other way to say what the tilt of a curve
    //! they built should come out as.
    static float fitTiltDbPerOctave(const std::vector<SpectrumAnalyzer::Band> & bands);
};

} // namespace noteahead

#endif // MIX_ADVISOR_HPP
