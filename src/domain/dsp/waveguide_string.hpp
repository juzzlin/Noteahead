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

#ifndef WAVEGUIDE_STRING_HPP
#define WAVEGUIDE_STRING_HPP

#include "all_pass_chain.hpp"
#include "delay_line.hpp"
#include "dsp_component.hpp"

#include <cstdint>
#include <vector>

namespace noteahead {

// Karplus-Strong digital waveguide string model.
// The excitation pulse is pre-loaded into the delay buffer at trigger time.
// The loop filter controls frequency-dependent decay (brightness).
// An all-pass chain inside the loop adds inharmonicity (string stiffness).
// Damping on note-off multiplies the per-cycle loop gain towards zero.
class WaveguideString : public DspComponent
{
public:
    // Lowest pitch the delay line must be able to hold: MIDI note 0. The buffer is
    // sized from the sample rate so that the bottom of the MIDI range stays in tune
    // at every rate rather than hitting a fixed ceiling.
    static constexpr double LowestFrequency = 8.1757989156437;

    void setSampleRate(double sampleRate) override;

    // brightness: 0=dark, 1=bright. decayTime: 0=short, 1=long. detuneCents: frequency offset in cents.
    void trigger(uint8_t note, float velocity, float brightness, float inharmonicity, float decayTime, double detuneCents = 0.0);
    void release(float releaseTime);

    double nextSample();
    bool isActive() const;
    void reset();

private:
    static constexpr double SilenceThreshold = 1e-9;
    static constexpr int ApStages = 4;
    // Ceiling on the loop gain, so that compensating the loop filter can never let the
    // loop hold on to more than it was given.
    static constexpr double MaxLoopGain = 0.99999;
    // Smallest loss the loop filter may impose, so that it never becomes a pass-through.
    static constexpr double MinLoopFilterCoeff = 0.005;
    // How far apart in time two strings struck together may have their hammers land.
    static constexpr double StrikeSpreadSeconds = 0.0025;

    static double midiNoteToFreq(uint8_t note);
    void buildExcitation(size_t width, float velocity);
    // Grows the delay line if the current sample rate needs more room than it has.
    void ensureBuffer();
    // Sets the delay-line length so that the whole loop resonates at m_frequency.
    // Returns the integer part of that length.
    size_t retune();
    // Sets the loop gain that produces the wanted decay time at the current pitch,
    // allowing for what the loop filter costs on each lap.
    void updateLoopGain();

    DelayLine m_delay;
    AllPassChain m_dispersion;
    // Supplies the fractional part of the loop length without touching the magnitude
    AllPassChain m_tuning;

    double m_frequency { 0.0 };
    double m_decayTime { 0.5 };
    double m_loopGain { 0.0 };
    double m_loopFilterCoeff { 0.25 };
    double m_loopFilterPrev { 0.0 };
    double m_dispersionCoeff { 0.0 };

    double m_damperGain { 1.0 };
    double m_damperDecay { 1.0 };
    bool m_releasing { false };

    double m_energy { 0.0 };
};

} // namespace noteahead

#endif // WAVEGUIDE_STRING_HPP
