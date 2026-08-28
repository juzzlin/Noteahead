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

#ifndef TEXT_TO_PHONEMES_HPP
#define TEXT_TO_PHONEMES_HPP

#include "phoneme.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace noteahead {

struct PhonemeEvent
{
    const PhonemeSpec * spec;
    //! First phoneme of a syllable. Step mode advances the phrase by these, so where they fall is
    //! what decides how a phrase divides over the notes of a pattern.
    bool syllableStart;
    //! First phoneme of a word.
    bool wordStart;
    //! Whether this phoneme belongs to the word's stressed syllable. English is stress-timed, so
    //! this is what the rhythm is built on: the strong syllable runs longer and takes a pitch accent.
    bool stressed { false };
    //! Multiplier on the phoneme's natural duration, carrying what this knows about prosody: the
    //! stressed syllable is long, unstressed ones are short, function words are weak, and the last
    //! syllable of a phrase runs long.
    //!
    //! English is stress-timed, and giving every syllable the same weight and the same length is
    //! most of what makes a synthesizer sound like it is reading a list of syllables rather than
    //! saying a sentence.
    double lengthScale { 1.0 };
};

using PhonemeEventList = std::vector<PhonemeEvent>;

//! Converts a written phrase into the phonemes to speak.
//!
//! English spelling is not a phonetic transcription, so this is a rule set rather than a lookup: a
//! few hundred context-sensitive letter-to-sound rules, tried longest-context first, in the manner
//! of the Naval Research Laboratory set that most small speech synthesizers descend from. It gets
//! ordinary words right and will get some proper nouns and loanwords wrong, which is why anything
//! between slashes is taken as literal phoneme names and passed through untouched:
//!
//!     hello /w er l d/   ->   HH EH L OW  W ER L D
//!
//! Stress is placed by rule, and the rules are right about four times in five. An apostrophe before
//! a syllable overrides them, which is the notation a dictionary uses:
//!
//!     A'merica           ->   AH  M EH R IH K AH   with the stress on "me"
//!
//! It is still an apostrophe in a contraction. The two are told apart by what follows: a contraction
//! ends the word right after it, in one of a closed set of endings, and a stress mark is followed by
//! a syllable.
//!
//! That escape is not a fallback for a weak rule set, it is the point. No rule set gets every
//! English word right, and a user who can hear that a word came out wrong needs a way to fix that
//! word without the fix leaking into every other word that shares its spelling.
PhonemeEventList textToPhonemes(std::string_view text);

//! Which syllable of @p word carries the primary stress, given that it has @p syllableCount of them.
//!
//! English stress is famously irregular and the honest way to get it right is a pronunciation
//! dictionary. This is the rule-based approximation: suffixes that pull the stress to a fixed
//! distance from the end, suffixes that hang off the end without moving it, and prefixes that push
//! it onto the root. Measured against espeak over 5160 dictionary words it agrees 80% of the time,
//! against 66% for always choosing the first syllable, which is what it replaces.
//!
//! Exposed so the rules can be tested against that measurement directly, independently of how this
//! file happens to divide a word into syllables.
size_t speechStressedSyllable(std::string_view word, size_t syllableCount);

//! Whether every rule in the letter-to-sound table names phonemes the synthesizer actually has.
//!
//! Exists for the test that guards the table. A typo in a rule's output would otherwise show up only
//! as one word quietly losing a sound, which is close to impossible to notice by ear among a few
//! hundred rules.
bool speechRulesAreWellFormed();

//! The phoneme names of @p events, space separated, with a double space between words. What the
//! dialog shows the user so they can see what the rules made of what they typed.
std::string phonemeNames(const PhonemeEventList & events);

} // namespace noteahead

#endif // TEXT_TO_PHONEMES_HPP
