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

#ifndef BIT_CRUSHER_HPP
#define BIT_CRUSHER_HPP

#include "effect.hpp"

namespace noteahead {

//! Word length and sample rate, both thrown away on purpose.
//!
//! Two separate degradations that are usually confused for one. Reducing the word length quantises
//! the amplitude, and the error that leaves is correlated with the signal, which is heard as the
//! grit riding on top of it. Reducing the sample rate holds each sample for several frames, which
//! folds everything above the new Nyquist back down as aliases -- and those are not harmonically
//! related to what produced them, which is why a rate reduction sounds metallic where a word length
//! reduction merely sounds dirty.
//!
//! Neither is oversampled, and that is the whole point. Every other nonlinear effect here runs at a
//! multiple of the sample rate precisely so that its aliases fall where they can be filtered away;
//! here the aliases are the effect, so oversampling would remove what the user asked for. The rate
//! is held in Hz rather than as a divisor for the same reason it is not oversampled: what it does
//! must not change when the engine's own oversampling does.
class BitCrusher : public Effect
{
public:
    BitCrusher();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void sync() override;
    void reset() override;

    //! Largest word length offered, which is where the effect is transparent.
    static int maxBits();

private:
    double quantize(double sample) const;

    double m_sampleRate { 0.0 };

    int m_bits { 16 };
    //! Rate the samples are held at, in Hz.
    double m_targetRate { 44100.0 };

    //! Frames each held sample lasts, and how far through one we are. Fractional, so a target rate
    //! that is not a whole division of the real one still averages out to the rate asked for
    //! instead of snapping to the nearest divisor.
    double m_holdFrames { 1.0 };
    double m_holdPosition { 0.0 };
    double m_heldLeft { 0.0 };
    double m_heldRight { 0.0 };
};

} // namespace noteahead

#endif // BIT_CRUSHER_HPP
