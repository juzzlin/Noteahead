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

#ifndef SPEECH_PROSODY_HPP
#define SPEECH_PROSODY_HPP

#include "phoneme.hpp"
#include "text_to_phonemes.hpp"

#include <string_view>
#include <vector>

namespace noteahead {

enum class BoundaryTone
{
    None = 0,
    Continuation,
    Statement,
    Question,
    Exclamation
};

struct ProsodyEvent
{
    BoundaryTone boundaryTone { BoundaryTone::None };
    double clausePosition { 0.0 };
    double accentSemitones { 0.0 };
    double loudnessGain { 1.0 };
    double microprosodySemitones { 0.0 };
    bool carriesBoundaryTone { false };
    bool inBoundaryRegion { false };
};

//! Analyzes punctuation, clause structure, stress downstep, and obstruent context on a phoneme stream
//! and evaluates natural F0 contours and loudness accents without altering V1's text-to-phoneme stream.
class SpeechProsody
{
public:
    //! Computes per-phoneme prosody cues and adjusts pause durations for punctuation marks.
    static std::vector<ProsodyEvent> analyze(std::string_view text, PhonemeEventList & events);

    //! Evaluates the instantaneous pitch offset in semitones for @p spec and @p cue.
    static double pitchOffsetSemitones(
      const ProsodyEvent & cue,
      const PhonemeSpec & spec,
      double phonemeProgress,
      double phonemeElapsedSeconds,
      double intonation);

    //! Evaluates the loudness multiplier scaled by @p intonation.
    static double loudnessGain(const ProsodyEvent & cue, double intonation);
};

} // namespace noteahead

#endif // SPEECH_PROSODY_HPP
