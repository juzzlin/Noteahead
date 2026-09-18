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

#include "bass_synth_presets.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../dsp/poly_blep_oscillator.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace noteahead {

namespace {

// Written in the units the panel is thought in -- hertz, seconds, percent -- and converted here.
// The filter knobs especially: both filters run on 20 Hz * (ceiling / 20 Hz) ^ knob, so a cutoff
// set as though the travel were linear in frequency lands an octave or more from where it was meant.

std::string key(const QString & xmlKey)
{
    return xmlKey.toStdString();
}

//! Knob position for a filter corner. The curve both the diode ladder and the high pass run on.
float cutoffKnob(double hz)
{
    constexpr double lowest = 20.0;
    constexpr double ceiling = 20000.0;
    return static_cast<float>(std::log(std::clamp(hz, lowest, ceiling) / lowest) / std::log(ceiling / lowest));
}

//! Knob position for the filter envelope's decay, which runs exponentially over 0.1 s to 10 s.
//! An accented note halves it, so these are the times the unaccented notes take.
float decayKnob(double seconds)
{
    return static_cast<float>(std::clamp(ParameterMapper::unmapExponential(seconds, 0.1, 10.0), 0.0, 1.0));
}

//! Knob position for the slide, exponential over 10 ms to 2 s. Zero is the one value that means
//! something else: no slide at all.
float slideKnob(double seconds)
{
    return seconds <= 0.0 ? 0.0f : static_cast<float>(std::clamp(ParameterMapper::unmapExponential(seconds, 0.01, 2.0), 0.0, 1.0));
}

//! The plain fractions: resonance, envelope amount, accent, sub level and the distortion controls.
float percent(double value)
{
    return static_cast<float>(std::clamp(value / 100.0, 0.0, 1.0));
}

float waveform(PolyBlepOscillator::Waveform value)
{
    return static_cast<float>(static_cast<int>(value));
}

struct Patch
{
    PolyBlepOscillator::Waveform waveform { PolyBlepOscillator::Waveform::Saw };
    double cutoffHz { 1000.0 };
    double resonance { 0.0 };
    double envMod { 50.0 };
    double decaySeconds { 0.4 };
    double accent { 50.0 };
    double slideSeconds { 0.0 };
    double subLevel { 0.0 };
    double distDrive { 0.0 };
    double distTone { 50.0 };
    double highPassHz { 20.0 };
};

SynthPreset preset(const std::string & name, const Patch & patch)
{
    namespace C = Constants::NahdXml;
    return SynthPreset {
        name,
        { { key(C::xmlKeyWaveform()), waveform(patch.waveform) },
          { key(C::xmlKeyLpfCutoff()), cutoffKnob(patch.cutoffHz) },
          { key(C::xmlKeyLpfResonance()), percent(patch.resonance) },
          { key(C::xmlKeyHpfCutoff()), cutoffKnob(patch.highPassHz) },
          { key(C::xmlKeyEnvMod()), percent(patch.envMod) },
          { key(C::xmlKeyDecay()), decayKnob(patch.decaySeconds) },
          { key(C::xmlKeyAccent()), percent(patch.accent) },
          { key(C::xmlKeySlide()), slideKnob(patch.slideSeconds) },
          { key(C::xmlKeySubLevel()), percent(patch.subLevel) },
          { key(C::xmlKeyDistDrive()), percent(patch.distDrive) },
          { key(C::xmlKeyDistTone()), percent(patch.distTone) } }
    };
}

} // namespace

const std::vector<SynthPreset> & BassSynthPresets::presets()
{
    // Init first because it is the way back, then alphabetical, as in the other lists.
    //
    // A bass line on this synth is half the patch and half how it is played: the accent and slide
    // columns are what the filter envelope and the glide are set up to respond to, so the patches
    // that name a slide time are the ones meant to be played with slides.
    static const std::vector<SynthPreset> presets {
        // The way back, as in the other lists: names nothing, so loading it is the init patch.
        SynthPreset { "Init", {} },

        // The sound the instrument is famous for: a resonant ladder opened by the envelope of every
        // note, and enough drive to keep the peak from thinning out.
        preset("Acid Lead", Patch { PolyBlepOscillator::Waveform::Saw, 600.0, 80.0, 75.0, 0.35, 70.0, 0.06, 0.0, 45.0, 60.0 }),

        // The same idea taken past the point of politeness: nearly self-oscillating, a short decay
        // and a square, which is where the hollow squelch comes from.
        preset("Acid Squelch", Patch { PolyBlepOscillator::Waveform::Square, 350.0, 92.0, 90.0, 0.25, 85.0, 0.05, 0.0, 60.0, 55.0 }),

        // Nothing but the fundamental and the octave below it. No resonance and almost no envelope:
        // a sub is a level, not a movement.
        preset("Deep Sub", Patch { PolyBlepOscillator::Waveform::Triangle, 220.0, 8.0, 12.0, 0.9, 30.0, 0.0, 80.0, 0.0, 50.0 }),

        // Slow, round and slid into: the envelope opens a little and takes its time closing, and
        // the sub carries the weight the filter takes away.
        preset("Dub Bass", Patch { PolyBlepOscillator::Waveform::Triangle, 180.0, 25.0, 30.0, 1.2, 40.0, 0.12, 50.0, 0.0, 45.0 }),

        // A saw kept wide open, thickened underneath rather than by resonance, with just enough
        // drive to give it an edge on a small speaker.
        preset("Fat Saw", Patch { PolyBlepOscillator::Waveform::Saw, 900.0, 30.0, 35.0, 0.6, 50.0, 0.0, 40.0, 25.0, 55.0 }),

        // Driven hard into a dark tone control, so the distortion adds weight rather than fizz.
        preset("Growl", Patch { PolyBlepOscillator::Waveform::Square, 450.0, 70.0, 60.0, 0.5, 60.0, 0.0, 20.0, 70.0, 35.0 }),

        // A short, bright blip: the envelope does nearly all of it, which is what makes the note
        // read as plucked rather than as filtered.
        preset("Plucked", Patch { PolyBlepOscillator::Waveform::Saw, 1200.0, 50.0, 80.0, 0.14, 45.0, 0.0, 0.0, 0.0, 60.0 }),

        // Mid resonance and a long slide: the patch for a line that walks between its notes instead
        // of stepping.
        preset("Rubber", Patch { PolyBlepOscillator::Waveform::Triangle, 500.0, 55.0, 50.0, 0.4, 50.0, 0.25, 25.0, 15.0, 50.0 }),

        // The high pass is the point: everything under 90 Hz is gone, so the part sits over a kick
        // and a sub without fighting either.
        preset("Skinny Stab", Patch { PolyBlepOscillator::Waveform::Square, 1400.0, 40.0, 55.0, 0.2, 55.0, 0.0, 0.0, 20.0, 60.0, 90.0 }),

        // Open, hollow and long enough to hold a note: the square used as a bass line rather than
        // as an acid line.
        preset("Square Lead", Patch { PolyBlepOscillator::Waveform::Square, 1500.0, 35.0, 40.0, 0.8, 50.0, 0.0, 30.0, 20.0, 55.0 })
    };

    return presets;
}

} // namespace noteahead
