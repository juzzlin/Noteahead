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

#ifndef TREMOLO_HPP
#define TREMOLO_HPP

#include "../dsp/lfo.hpp"
#include "effect.hpp"

namespace noteahead {

//! Level modulated by an LFO. The Auto Panner's mechanism pointed at loudness instead of position.
//!
//! Worth being its own effect rather than a mode of that one because the two do genuinely different
//! things to a mix: panning moves a sound without changing how loud it is, and a panner set to a
//! square wave still sums to the same level in mono. This takes level away and puts it back, so it
//! survives a fold to mono and it changes the track's relationship to everything compressing behind
//! it.
//!
//! Stereo Phase is what makes the difference audible in the other direction. At zero both channels
//! duck together, which is the tremolo on an amplifier. At a hundred and eighty they duck in
//! opposition, and the sound swings between the speakers while its mono sum stays put -- a
//! different effect again from the panner, which moves the sound itself.
class Tremolo : public Effect
{
public:
    Tremolo();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void sync() override;
    void setBpm(float bpm) override;
    void reset() override;

    //! Largest factor the Rate Divider offers. As the Phaser's and the Auto Panner's.
    static int maxRateDivider();

private:
    void updateLfoFrequency();

    Lfo m_lfoLeft;
    Lfo m_lfoRight;

    double m_sampleRate { 0.0 };

    double m_rate { 0.5 };
    double m_syncDivision { 0.25 };
    int m_rateDivider { 1 };
    bool m_sync { false };
    //! How much level the deepest point takes away, 0 to 1. At one the signal reaches silence.
    double m_intensity { 0.5 };
    double m_stereoPhase { 0.0 };
};

} // namespace noteahead

#endif // TREMOLO_HPP
