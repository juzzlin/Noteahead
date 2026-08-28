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

#ifndef PHONEME_HPP
#define PHONEME_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace noteahead {

//! What a phoneme does to the source. FormantVoice renders each of these differently, so the type
//! is what the voice branches on rather than the name.
enum class PhonemeType
{
    Silence,
    Vowel,
    Nasal,
    Liquid,
    Fricative,
    Plosive
};

//! One resonance of the vocal tract.
//!
//! Bandwidth rather than Q, for the reason FormantFilterBank gives: a formant's bandwidth stays
//! roughly put as its frequency moves, so the two are not the same thing to specify.
struct FormantTarget
{
    double frequency;
    double bandwidth;
    //! Relative height of this peak. Published formant amplitudes are measured on radiated speech
    //! and so already contain the glottal rolloff, which is why FormantVoice tilts its source
    //! rather than weighting these to imitate the tilt.
    double amplitude;
};

using FormantTargets = std::array<FormantTarget, 3>;

struct PhonemeSpec
{
    std::string_view name;
    PhonemeType type;
    FormantTargets formants;
    //! Where the formants have arrived by the end of the phoneme. Set only for the diphthongs,
    //! which are a movement rather than a position -- /aɪ/ is not a vowel that can be held.
    std::optional<FormantTargets> glideTo;
    //! 1 fully voiced, 0 pure noise. The voiced fricatives sit in between, which is what separates
    //! them from their unvoiced partners.
    double voicing;
    //! Loudness relative to an open vowel.
    double amplitude;
    //! Natural duration. Free mode scales this by the rate, Fit scales the whole phrase by it, and
    //! Grid uses it only to share a syllable's slot out among its consonants.
    uint16_t nominalMs;
};

//! Every phoneme the synthesizer knows, for callers that need to enumerate rather than look up.
std::span<const PhonemeSpec> speechPhonemes();

//! The phoneme of that name, or nullptr when the table has none. Names are ARPABET.
const PhonemeSpec * speechPhoneme(std::string_view name);

//! The silence between words, and what a lookup falls back to.
const PhonemeSpec & speechSilence();

} // namespace noteahead

#endif // PHONEME_HPP
