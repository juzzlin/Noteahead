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

#ifndef DOUBLER_HPP
#define DOUBLER_HPP

#include "../dsp/delay_line.hpp"
#include "../dsp/svf_filter.hpp"
#include "effect.hpp"

namespace noteahead {

//! Doubles a part with a copy of itself a few milliseconds late, the way a second take or tape ADT
//! does, so that one source sounds like two played together and spreads across the field.
//!
//! Unlike Dimension, nothing is detuned: the copy is the same sound, only later. That keeps a plain
//! tone such as a saw lead sounding exactly like itself, just wider.
//!
//! Two ways to place the copy:
//!
//! Mono Safe adds it to the side signal alone, as the difference between the channels. The left
//! hears the source plus the copy and the right the source minus it, so each side combs the other
//! way and the ear hears width. Summed to mono the copy cancels, and what comes back is exactly the
//! mid that went in.
//!
//! Haas moves the right channel to the copy, which is the classic double: the source on the left,
//! the copy on the right. It is the more obvious of the two, and it pays for it in mono, where the
//! two sum into a comb filter.
//!
//! Low Cut keeps the bottom out of either: below it the part stays where it was. It matters more
//! than on a widener. A copied harmonic is louder on one side than the other by where it falls in
//! the comb, which depends on the note, so a copied fundamental throws whole notes to one side.
//! Above the fundamentals the copy only reaches harmonics many to a comb tooth, which even out.
//! Drift lets the copy's delay wander slowly, as a tape machine's would, so the double never locks
//! into one fixed comb.
class Doubler : public Effect
{
public:
    //! Persisted as an ordinal: append only.
    enum class Mode
    {
        MonoSafe,
        Haas
    };

    Doubler();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;
    std::vector<std::string> modulationPriority() const override;

    void processSample(double & left, double & right) override;
    void reset() override;
    void sync() override;
    bool isSettled() const override;

    //! Delay before drift, in milliseconds, for @p value on the Time control.
    static double timeMs(double value);

private:
    void updateState();
    void syncParameters();
    void prepareBuffer(double sampleRate);

    DelayLine m_delay;
    //! Applied to what is delayed, so the copy has no bottom in it. Two sections, 24 dB/oct
    SvfFilter m_lowCut;
    SvfFilter m_lowCut2;
    bool m_lowCutActive { true };

    Mode m_mode { Mode::MonoSafe };
    double m_targetDelayFrames { 0.0 };
    double m_delayFrames { 0.0 };
    double m_amount { 0.0 };
    double m_driftFrames { 0.0 };
    double m_driftPhaseA { 0.0 };
    double m_driftPhaseB { 0.0 };
    double m_smoothing { 0.0 };
    size_t m_silentFrames { 0 };

    bool m_shouldSyncParameters { false };
    double m_lastSampleRate { -1.0 };
};

} // namespace noteahead

#endif // DOUBLER_HPP
