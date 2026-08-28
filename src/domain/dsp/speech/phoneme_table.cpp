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

#include "phoneme.hpp"

#include <algorithm>

namespace noteahead {

// The one place vowel quality is tuned. Vowel formant frequencies are the Peterson & Barney male
// averages; the bandwidths and the relative peak heights are the conventional synthesis values.
//
// Vowel amplitudes are not all one. A vowel's intrinsic intensity follows how open it is: /a/ is the
// loudest sound in the language and /i/ and /u/ sit some 5 dB below it. Left equal they came out the
// wrong way round -- the close vowels measured 5 dB *above* /a/, because their first formant sits
// low where the glottal source is strongest and the bank passes more of it -- so "two" and "tick"
// jumped out of every phrase they appeared in.
//
// Two things about this table are load-bearing rather than cosmetic:
//
// The diphthongs are given as a movement, from formants to glideTo, because that is what they are:
// a listener identifies /aɪ/ by the direction F2 travels, not by any point along the way. Freezing
// one at its midpoint yields a vowel that is not in the language.
//
// The fricatives' "formants" are noise band centres, not resonances of a tract, which is why they
// run far above the vowels -- /s/ lives around 5 kHz -- and why their bandwidths are several hundred
// hertz. Narrow them to vowel widths and the sibilants whistle at a pitch instead of hissing.
//
// How wide is not a detail. At a Q of five to eight these read as separate tuned peaks rather than
// as one broad rush of air: /s/ came out thin and piercing, with its energy in two spikes at 5.5 and
// 7.5 kHz and a hole beneath them where a real /s/ has a shelf running from 3.5 kHz up. Where the
// shelf sits matters as much as how broad it is: with the peak at 5.6 kHz and a band reaching past
// 9.6 kHz it still read as piercing, and what fixed that was moving the energy down rather than
// turning it down -- a sibilant that is merely quiet is a sibilant you cannot hear. The stop
// bursts had the same Q, and a short noise burst through a resonant band with an exponential decay
// is not an approximation of a hihat -- it is exactly how one is synthesised, which is what /t/ and
// /k/ sounded like. Everything that shapes noise is now Q 2 or below.
//
// The voiced fricatives are the exception: their lowest band is not frication at all but the voice
// bar, the glottal buzz heard through the constriction, and it is most of what makes one audible.
// Given three frication bands like their unvoiced partners, /ð/ measured 23 dB under the vowel
// beside it with nothing below 1.2 kHz, so "the" came out as a bare schwa -- the consonant was
// simply not there. The band it replaces carried an amplitude of 0.15 and was doing nothing.
//
// Levels are quoted relative to an open vowel, which is how speech levels are measured. Nasals sit
// some 8 dB down and the liquids 4 to 5, and getting that wrong in the other direction is what made
// them read as vowels of their own rather than as consonants.

namespace {

constexpr FormantTargets makeVowel(double f1, double f2, double f3, double a2 = 0.8, double a3 = 0.5)
{
    return { { { f1, 70.0, 1.0 }, { f2, 100.0, a2 }, { f3, 160.0, a3 } } };
}

// Frequencies only, for naming a diphthong's destination as the vowel it lands on.
constexpr FormantTargets Iy = makeVowel(270.0, 2290.0, 3010.0, 0.9, 0.7);
constexpr FormantTargets Ih = makeVowel(390.0, 1990.0, 2550.0, 0.85, 0.6);
constexpr FormantTargets Eh = makeVowel(530.0, 1840.0, 2480.0, 0.85, 0.55);
constexpr FormantTargets Ae = makeVowel(660.0, 1720.0, 2410.0, 0.8, 0.5);
constexpr FormantTargets Aa = makeVowel(730.0, 1090.0, 2440.0, 0.75, 0.45);
constexpr FormantTargets Ao = makeVowel(570.0, 840.0, 2410.0, 0.7, 0.4);
constexpr FormantTargets Uh = makeVowel(440.0, 1020.0, 2240.0, 0.7, 0.4);
constexpr FormantTargets Uw = makeVowel(300.0, 870.0, 2240.0, 0.65, 0.35);
constexpr FormantTargets Ah = makeVowel(640.0, 1190.0, 2390.0, 0.75, 0.45);
constexpr FormantTargets Er = makeVowel(490.0, 1350.0, 1690.0, 0.8, 0.7);
constexpr FormantTargets Ax = makeVowel(500.0, 1500.0, 2500.0, 0.7, 0.45);

constexpr PhonemeSpec Table[] {
    { "_", PhonemeType::Silence, makeVowel(500.0, 1500.0, 2500.0), std::nullopt, 0.0, 0.0, 90 },

    // Monophthongs.
    { "IY", PhonemeType::Vowel, Iy, std::nullopt, 1.0, 0.33, 155 },
    { "IH", PhonemeType::Vowel, Ih, std::nullopt, 1.0, 0.48, 115 },
    { "EH", PhonemeType::Vowel, Eh, std::nullopt, 1.0, 0.61, 125 },
    { "AE", PhonemeType::Vowel, Ae, std::nullopt, 1.0, 0.85, 175 },
    { "AA", PhonemeType::Vowel, Aa, std::nullopt, 1.0, 1.0, 170 },
    { "AO", PhonemeType::Vowel, Ao, std::nullopt, 1.0, 0.81, 170 },
    { "UH", PhonemeType::Vowel, Uh, std::nullopt, 1.0, 0.48, 115 },
    { "UW", PhonemeType::Vowel, Uw, std::nullopt, 1.0, 0.3, 155 },
    { "AH", PhonemeType::Vowel, Ah, std::nullopt, 1.0, 0.74, 125 },
    { "ER", PhonemeType::Vowel, Er, std::nullopt, 1.0, 0.61, 165 },
    { "AX", PhonemeType::Vowel, Ax, std::nullopt, 1.0, 0.4, 85 },

    // Diphthongs.
    { "EY", PhonemeType::Vowel, Eh, Iy, 1.0, 0.52, 200 },
    { "AY", PhonemeType::Vowel, Aa, Iy, 1.0, 0.67, 215 },
    { "OY", PhonemeType::Vowel, Ao, Iy, 1.0, 0.52, 225 },
    { "AW", PhonemeType::Vowel, Aa, Uw, 1.0, 0.67, 215 },
    { "OW", PhonemeType::Vowel, Ao, Uw, 1.0, 0.54, 200 },

    // Nasals. The murmur is the low F1; the upper peaks are held down because the nasal cavity puts
    // an antiresonance where the mouth would have had a formant, and a bank of three all-positive
    // resonances has no way to spell a zero.
    { "M", PhonemeType::Nasal, { { { 250.0, 80.0, 1.0 }, { 1100.0, 120.0, 0.35 }, { 2300.0, 180.0, 0.2 } } }, std::nullopt, 1.0, 0.22, 70 },
    { "N", PhonemeType::Nasal, { { { 250.0, 80.0, 1.0 }, { 1700.0, 130.0, 0.35 }, { 2600.0, 180.0, 0.2 } } }, std::nullopt, 1.0, 0.22, 70 },
    { "NG", PhonemeType::Nasal, { { { 250.0, 80.0, 1.0 }, { 2300.0, 150.0, 0.3 }, { 2900.0, 200.0, 0.2 } } }, std::nullopt, 1.0, 0.22, 75 },

    // Liquids and glides.
    { "L", PhonemeType::Liquid, { { { 400.0, 70.0, 1.0 }, { 1200.0, 100.0, 0.6 }, { 2600.0, 160.0, 0.35 } } }, std::nullopt, 1.0, 0.45, 65 },
    { "R", PhonemeType::Liquid, { { { 490.0, 70.0, 1.0 }, { 1350.0, 100.0, 0.7 }, { 1690.0, 140.0, 0.6 } } }, std::nullopt, 1.0, 0.45, 65 },
    { "W", PhonemeType::Liquid, { { { 300.0, 70.0, 1.0 }, { 610.0, 90.0, 0.7 }, { 2200.0, 160.0, 0.3 } } }, std::nullopt, 1.0, 0.45, 60 },
    { "Y", PhonemeType::Liquid, { { { 260.0, 60.0, 1.0 }, { 2070.0, 110.0, 0.9 }, { 3020.0, 180.0, 0.6 } } }, std::nullopt, 1.0, 0.45, 55 },

    // Fricatives. HH is aspiration through a neutral tract, which is why it is the schwa's shape
    // with the voicing taken out.
    { "HH", PhonemeType::Fricative, { { { 500.0, 400.0, 0.5 }, { 1500.0, 900.0, 0.7 }, { 2600.0, 1400.0, 0.5 } } }, std::nullopt, 0.0, 0.22, 65 },
    { "F", PhonemeType::Fricative, { { { 1000.0, 600.0, 0.15 }, { 2900.0, 1600.0, 0.5 }, { 6200.0, 2600.0, 0.5 } } }, std::nullopt, 0.0, 0.16, 95 },
    { "V", PhonemeType::Fricative, { { { 280.0, 120.0, 0.35 }, { 2900.0, 1600.0, 0.5 }, { 6200.0, 2600.0, 0.5 } } }, std::nullopt, 0.55, 0.2, 65 },
    { "TH", PhonemeType::Fricative, { { { 1200.0, 700.0, 0.15 }, { 3100.0, 1800.0, 0.4 }, { 6600.0, 2800.0, 0.4 } } }, std::nullopt, 0.0, 0.20, 90 },
    { "DH", PhonemeType::Fricative, { { { 280.0, 120.0, 0.35 }, { 3100.0, 1800.0, 0.4 }, { 6600.0, 2800.0, 0.4 } } }, std::nullopt, 0.55, 0.28, 60 },
    { "S", PhonemeType::Fricative, { { { 3400.0, 1600.0, 0.55 }, { 5000.0, 2200.0, 1.0 }, { 7000.0, 3000.0, 0.10 } } }, std::nullopt, 0.0, 0.26, 100 },
    { "Z", PhonemeType::Fricative, { { { 280.0, 120.0, 0.30 }, { 5000.0, 2200.0, 1.0 }, { 7000.0, 3000.0, 0.10 } } }, std::nullopt, 0.5, 0.30, 75 },
    { "SH", PhonemeType::Fricative, { { { 1900.0, 900.0, 0.4 }, { 2700.0, 1400.0, 1.0 }, { 4300.0, 2000.0, 0.5 } } }, std::nullopt, 0.0, 0.34, 105 },
    { "ZH", PhonemeType::Fricative, { { { 280.0, 120.0, 0.30 }, { 2700.0, 1400.0, 1.0 }, { 4300.0, 2000.0, 0.5 } } }, std::nullopt, 0.5, 0.40, 75 },

    // Plosives. The formants here are the spectrum of the release burst, and the place of
    // articulation is entirely in where that burst sits: labial low, velar mid, alveolar high.
    // The voiced ones differ from their partners in the voice bar heard during the closure, and in that
    // the unvoiced ones are aspirated: their length allows for a burst plus the 40 to 80 ms of breath
    // that English puts between an unvoiced stop and the voice that follows it. A voiced stop has almost
    // none, which is what "voice onset time" measures, so it stays short.
    { "P", PhonemeType::Plosive, { { { 800.0, 700.0, 1.0 }, { 1600.0, 1100.0, 0.5 }, { 2700.0, 1500.0, 0.3 } } }, std::nullopt, 0.0, 0.55, 115 },
    { "B", PhonemeType::Plosive, { { { 800.0, 700.0, 1.0 }, { 1600.0, 1100.0, 0.5 }, { 2700.0, 1500.0, 0.3 } } }, std::nullopt, 0.35, 0.42, 65 },
    { "T", PhonemeType::Plosive, { { { 3000.0, 2000.0, 0.6 }, { 4500.0, 2600.0, 1.0 }, { 6600.0, 3000.0, 0.5 } } }, std::nullopt, 0.0, 0.70, 115 },
    { "D", PhonemeType::Plosive, { { { 3000.0, 2000.0, 0.6 }, { 4500.0, 2600.0, 1.0 }, { 6600.0, 3000.0, 0.5 } } }, std::nullopt, 0.35, 0.55, 65 },
    { "K", PhonemeType::Plosive, { { { 1400.0, 1000.0, 0.7 }, { 2300.0, 1500.0, 1.0 }, { 3700.0, 2000.0, 0.5 } } }, std::nullopt, 0.0, 0.85, 120 },
    { "G", PhonemeType::Plosive, { { { 1400.0, 1000.0, 0.7 }, { 2300.0, 1500.0, 1.0 }, { 3700.0, 2000.0, 0.5 } } }, std::nullopt, 0.35, 0.62, 65 },
};

} // namespace

std::span<const PhonemeSpec> speechPhonemes()
{
    return Table;
}

const PhonemeSpec * speechPhoneme(std::string_view name)
{
    const auto it = std::ranges::find(Table, name, &PhonemeSpec::name);
    return it != std::ranges::end(Table) ? &*it : nullptr;
}

const PhonemeSpec & speechSilence()
{
    return Table[0];
}

} // namespace noteahead
