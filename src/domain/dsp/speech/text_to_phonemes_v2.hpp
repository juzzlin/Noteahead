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

#ifndef TEXT_TO_PHONEMES_V2_HPP
#define TEXT_TO_PHONEMES_V2_HPP

#include "phoneme.hpp"
#include "text_to_phonemes.hpp"

#include <string_view>

namespace noteahead {

//! Alveolar flap (/DX/) used for intervocalic T/D before unstressed vowels.
const PhonemeSpec & speechFlapPhoneme();

//! Converts a written phrase into Speech V2 phoneme events with number expansion, contractions,
//! an irregular-word exception lexicon, allophones (flap DX, context-aware "the"), and
//! pre-fortis vowel clipping, while leaving V1 textToPhonemes() untouched.
PhonemeEventList textToPhonemesV2(std::string_view text);

//! Verifies that every phoneme referenced in the V2 contractions and exception lexicon exists.
bool speechV2LexiconIsWellFormed();

} // namespace noteahead

#endif // TEXT_TO_PHONEMES_V2_HPP
