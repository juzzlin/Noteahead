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

#include "eq_8_band_parametric_presets.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../dsp/svf_filter.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace noteahead {

namespace {

// Every patch below is written in the units it is thought in -- hertz, decibels, Q -- and converted
// to knob positions here. Writing them as positions directly is how a table like this goes wrong:
// the frequency knob is logarithmic and the Q knob exponential, so a plausible-looking number lands
// nowhere near the frequency it was meant to name.

//! Knob position for a band centre. The EQ maps this knob logarithmically over 20 Hz to 20 kHz.
float freq(double hz)
{
    return static_cast<float>(std::clamp(ParameterMapper::unmapLogFrequency(hz, 20.0, 20000.0), 0.0, 1.0));
}

//! Knob position for a band gain, which runs linearly from -24 dB to +24 dB about a flat centre.
float gain(double db)
{
    return static_cast<float>(std::clamp((db + 24.0) / 48.0, 0.0, 1.0));
}

//! Knob position for a band Q, which the EQ maps exponentially over 0.1 to 10.
float resonance(double q)
{
    return static_cast<float>(std::clamp(ParameterMapper::unmapExponential(q, 0.1, 10.0), 0.0, 1.0));
}

//! Q of a bell whose -3 dB bandwidth is \a octaves wide.
//!
//! Worth having rather than a literal: "a one-octave bell", which is what these presets are written
//! against throughout, is Q of about 1.41 and not the 1.0 the phrase invites.
double bellQForOctaves(double octaves)
{
    const auto ratio = std::pow(2.0, octaves);
    return std::sqrt(ratio) / (ratio - 1.0);
}

//! Maximally flat: -3 dB at the corner, with no dip below it or peak above it.
//!
//! One EQ band is a second-order section, so a high pass built on one is 12 dB/octave. The source
//! notes ask for 24, which would take a second band at a second Q value -- and a preset that spends
//! two of the eight bands on its high pass both looks like a duplicated band and leaves less room
//! for the moves the preset is a starting point for. One band it is; the second is a knob away for
//! anyone who wants the steeper slope.
constexpr double HighPassQ = 0.70711;

//! Standard flat shelf. Higher would put a dip below the corner and a bump above it.
constexpr double ShelfQ = 0.70711;

struct Band
{
    SvfFilter::Type type { SvfFilter::Type::Bypass };
    double hz { 1000.0 };
    double db { 0.0 };
    double q { 0.70711 };
};

//! A one-octave bell, the shape the source notes prescribe wherever they do not give a Q.
Band bell(double hz, double db)
{
    return { SvfFilter::Type::Bell, hz, db, bellQForOctaves(1.0) };
}

//! A bell at an explicitly given Q.
Band bell(double hz, double db, double q)
{
    return { SvfFilter::Type::Bell, hz, db, q };
}

Band highShelf(double hz, double db)
{
    return { SvfFilter::Type::HighShelf, hz, db, ShelfQ };
}

//! A 12 dB/octave high pass at \a hz.
Band highPass(double hz)
{
    return { SvfFilter::Type::LowCut, hz, 0.0, HighPassQ };
}

//! Bands are filled from band 1 upwards and in ascending frequency, so the dialog reads across like
//! a spectrum. Whatever is left over stays Bypass, which is its default.
EffectPreset preset(std::string name, std::vector<Band> bands)
{
    namespace C = Constants::NahdXml;
    EffectPreset result { std::move(name), {} };
    for (size_t i = 0; i < bands.size(); i++) {
        const auto & band = bands.at(i);
        result.parameters[C::xmlKeyBandType(i).toStdString()] = static_cast<float>(static_cast<int>(band.type));
        result.parameters[C::xmlKeyBandFreq(i).toStdString()] = freq(band.hz);
        result.parameters[C::xmlKeyBandGain(i).toStdString()] = gain(band.db);
        result.parameters[C::xmlKeyBandQ(i).toStdString()] = resonance(band.q);
    }
    return result;
}

} // namespace

const EffectPresetList & Eq8BandParametricPresets::presets()
{
    // Starting points rather than finished curves, and deliberately gentle: every move is a few dB,
    // which is the range in which an EQ decision is still reversible by ear.
    //
    // Flat comes first because it is the way back; the rest are alphabetical, so a source is found
    // by name in a list this long rather than by reading all of it.
    static const EffectPresetList presets {
        // Every band bypassed, which is also the effect's own default.
        preset("Flat", {}),

        // Deep sub plus the punch an 808 is played for, and the mud between them taken out.
        preset("808 Kick", { highPass(30.0), bell(50.0, 3.0, 1.0), bell(120.0, 2.0, 1.2), bell(250.0, -2.5, 1.2) }),

        // A 909 sits higher than an 808: sub at 55 Hz, boxiness at 220 Hz, and the click that
        // carries it on a small speaker at 3.5 kHz.
        preset("909 Kick", { highPass(30.0), bell(55.0, 3.0, 1.0), bell(220.0, -3.0, 1.2), bell(3500.0, 2.0, 1.5) }),

        preset("Bass Guitar", { highPass(30.0), bell(3000.0, 3.0) }),

        preset("Choir", { bell(300.0, -2.0, 1.2), bell(2200.0, 1.5, 1.5), bell(10000.0, 2.0, 1.5) }),

        preset("Clap", { bell(400.0, -2.5, 1.2), bell(2000.0, 2.0, 1.5), bell(8000.0, 2.0, 1.5) }),

        // The notes ask for nothing but the high pass on a guitar, and a preset that invents the
        // rest would be guessing at an instrument that is recorded a dozen different ways.
        preset("Guitar", { highPass(80.0) }),

        // 150 Hz is the lowest of the high passes: there is nothing under it on a cymbal but spill.
        preset("Hi-Hats & Cymbals", { highPass(150.0), bell(700.0, -2.0, 1.2), bell(9500.0, 2.0, 1.5) }),

        preset("Kick Drum", { highPass(30.0), bell(60.0, 3.0), bell(300.0, -3.0), bell(4000.0, 3.0), highShelf(8000.0, 6.0) }),

        preset("Lead Vocal", { highPass(100.0), bell(120.0, 3.0), bell(300.0, -3.0), bell(6000.0, 3.0), highShelf(12000.0, 3.0) }),

        // The air shelf is +3 dB where the source notes offer up to +8 dB, with the warning that
        // +8 dB is often too much. A preset is what gets loaded before anyone listens, so it takes
        // the cautious end of the range; the knob goes the rest of the way.
        preset("Master Bus", { highPass(30.0), bell(80.0, 3.0), bell(500.0, -3.0), highShelf(16000.0, 3.0) }),

        preset("Pads", { bell(250.0, -3.0, 1.0), bell(5000.0, 2.0, 1.5) }),

        preset("Piano", { bell(280.0, -3.0, 1.0), bell(2500.0, 2.0, 1.2), bell(9000.0, 1.5, 1.5) }),

        // The high pass at 80 Hz and the lift at 100 Hz are both in the source notes, and they do
        // pull against each other: the point of the pair is a defined bottom rather than a deeper one.
        preset("Snare Drum", { highPass(80.0), bell(100.0, 3.0), bell(350.0, -3.0, 1.2), bell(2200.0, 3.0, 1.5), bell(6500.0, 2.0, 1.5) }),

        preset("Strings", { bell(300.0, -2.0, 1.2), bell(7000.0, 2.0, 1.5) }),

        preset("Synth Bass", { highPass(30.0), bell(90.0, 2.5, 1.0), bell(300.0, -3.0, 1.2), bell(2000.0, 1.5, 1.5) }),

        preset("Synth Lead", { bell(320.0, -2.5, 1.2), bell(3500.0, 2.0, 1.5), bell(8000.0, 1.5, 1.5) })
    };
    return presets;
}

} // namespace noteahead
