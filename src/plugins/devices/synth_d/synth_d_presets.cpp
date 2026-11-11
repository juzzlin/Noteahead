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

#include "synth_d_presets.hpp"

#include "../../../common/constants.hpp"
#include "../../../domain/dsp/synth_d_voice.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <string>

namespace noteahead {

namespace {

using Range = SynthDVoice::Range;
using Wave = SynthDVoice::Waveform;

struct Osc
{
    Range range { Range::Feet8 };
    Wave wave { Wave::Saw };
    double frequency { 0.0 }; //!< Semitones, -7..7
    double volume { 10.0 }; //!< 0..10
    bool on { false };
};

//! A patch as the panel reads: knobs 0..10, the cutoff -4..4, tunings in semitones. Every field
//! starts where the device starts, so a patch names only what it sets.
struct Patch
{
    double tune { 0.0 };
    double glide { 0.0 };
    double modMix { 0.0 };
    double modDepth { 0.0 };
    bool filterEg { false }; //!< Mod source 1: OSC 3, or the filter contour
    bool lfo { false }; //!< Mod source 2: noise, or the LFO
    bool oscMod { false };
    bool filterMod { false };
    double lfoRate { 5.0 };
    bool lfoSquare { false };
    std::array<Osc, 3> osc { Osc { .on = true }, Osc {}, Osc {} };
    bool osc3Control { true };
    double ext { 0.0 };
    bool extOn { false };
    double noise { 0.0 };
    bool noiseOn { false };
    bool pink { false };
    double cutoff { 0.8 };
    double emphasis { 0.0 };
    double contour { 3.0 };
    bool keyboard1 { true };
    bool keyboard2 { false };
    std::array<double, 3> filterAds { 0.0, 4.0, 5.0 };
    std::array<double, 3> loudnessAds { 0.0, 4.0, 10.0 };
    //! The patch sheets' Decay switch, which on the Model D is one for each contour
    bool decay { false };
    //! Trims the patch to about Init's level
    double gainDb { 0.0 };
};

std::map<std::string, float> parameters(const Patch & patch)
{
    namespace Xml = Constants::NahdXml;
    const auto key = [](const QString & k) {
        return k.toStdString();
    };
    const auto knob = [](double panel) {
        return static_cast<float>(std::clamp(panel / 10.0, 0.0, 1.0));
    };
    const auto bipolar = [](double value, double range) {
        return static_cast<float>(std::clamp((value / range + 1.0) / 2.0, 0.0, 1.0));
    };
    const auto flag = [](bool on) {
        return on ? 1.0f : 0.0f;
    };
    const std::array<QString, 3> ranges { Xml::xmlKeyVco1Range(), Xml::xmlKeyVco2Range(), Xml::xmlKeyVco3Range() };
    const std::array<QString, 3> waves { Xml::xmlKeyVco1Waveform(), Xml::xmlKeyVco2Waveform(), Xml::xmlKeyVco3Waveform() };
    const std::array<QString, 3> volumes { Xml::xmlKeyVco1Level(), Xml::xmlKeyVco2Level(), Xml::xmlKeyVco3Level() };
    const std::array<QString, 3> switches { Xml::xmlKeyVco1On(), Xml::xmlKeyVco2On(), Xml::xmlKeyVco3On() };

    std::map<std::string, float> values {
        { key(Xml::xmlKeyTune()), bipolar(patch.tune, 2.0) },
        { key(Xml::xmlKeyGlide()), knob(patch.glide) },
        { key(Xml::xmlKeyModMix()), knob(patch.modMix) },
        { key(Xml::xmlKeyModDepth()), knob(patch.modDepth) },
        { key(Xml::xmlKeyModSource1()), flag(patch.filterEg) },
        { key(Xml::xmlKeyModSource2()), flag(patch.lfo) },
        { key(Xml::xmlKeyOscillatorModulation()), flag(patch.oscMod) },
        { key(Xml::xmlKeyFilterModulation()), flag(patch.filterMod) },
        { key(Xml::xmlKeyLfoRate()), knob(patch.lfoRate) },
        { key(Xml::xmlKeyLfoWaveform()), flag(patch.lfoSquare) },
        { key(Xml::xmlKeyVco2Pitch()), bipolar(patch.osc[1].frequency, 7.0) },
        { key(Xml::xmlKeyVco3Pitch()), bipolar(patch.osc[2].frequency, 7.0) },
        { key(Xml::xmlKeyVco3KeyboardControl()), flag(patch.osc3Control) },
        { key(Xml::xmlKeyFeedback()), knob(patch.ext) },
        { key(Xml::xmlKeyFeedbackOn()), flag(patch.extOn) },
        { key(Xml::xmlKeyNoiseLevel()), knob(patch.noise) },
        { key(Xml::xmlKeyNoiseOn()), flag(patch.noiseOn) },
        { key(Xml::xmlKeyColor()), flag(patch.pink) },
        { key(Xml::xmlKeyCutoff()), static_cast<float>(std::clamp((patch.cutoff + 4.0) / 8.0, 0.0, 1.0)) },
        { key(Xml::xmlKeyResonance()), knob(patch.emphasis) },
        { key(Xml::xmlKeyEnvMod()), knob(patch.contour) },
        { key(Xml::xmlKeyKeyboardControl1()), flag(patch.keyboard1) },
        { key(Xml::xmlKeyKeyboardControl2()), flag(patch.keyboard2) },
        { key(Xml::xmlKeyEnv1Attack()), knob(patch.filterAds[0]) },
        { key(Xml::xmlKeyEnv1Decay()), knob(patch.filterAds[1]) },
        { key(Xml::xmlKeyEnv1Sustain()), knob(patch.filterAds[2]) },
        { key(Xml::xmlKeyEnv2Attack()), knob(patch.loudnessAds[0]) },
        { key(Xml::xmlKeyEnv2Decay()), knob(patch.loudnessAds[1]) },
        { key(Xml::xmlKeyEnv2Sustain()), knob(patch.loudnessAds[2]) },
        { key(Xml::xmlKeyEnv1DecaySwitch()), flag(patch.decay) },
        { key(Xml::xmlKeyEnv2DecaySwitch()), flag(patch.decay) },
        { key(Xml::xmlKeyGain()), static_cast<float>(std::clamp((patch.gainDb + 30.0) / 60.0, 0.0, 1.0)) },
    };
    for (size_t i = 0; i < 3; i++) {
        values[key(ranges[i])] = static_cast<float>(patch.osc[i].range);
        values[key(waves[i])] = static_cast<float>(patch.osc[i].wave);
        values[key(volumes[i])] = knob(patch.osc[i].volume);
        values[key(switches[i])] = flag(patch.osc[i].on);
    }
    return values;
}

} // namespace

const std::vector<SynthPreset> & SynthDPresets::presets()
{
    // Init first, as the way back; the rest alphabetical. Most are read off the patch sheets that
    // come with the instrument, renamed: the mod wheel adds the vibrato or the movement where they
    // say so, and the knobs they leave blank are left where Init has them.
    static const std::vector<SynthPreset> presets {
        { .name = "Init", .parameters = {} },
        // Three saws an octave apart, the filter half open, under the mod wheel's vibrato
        { .name = "Brass",
          .parameters = parameters({ .modMix = 7.5, .lfo = true, .oscMod = true, .lfoRate = 5.5, .osc = { Osc { .range = Range::Feet4, .wave = Wave::Saw, .volume = 5.0, .on = true }, Osc { .range = Range::Feet8, .wave = Wave::Saw, .volume = 2.0, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::Saw, .volume = 3.5, .on = true } }, .cutoff = 0.5, .emphasis = 3.5, .contour = 2.0, .filterAds = { 1.0, 3.5, 3.0 }, .loudnessAds = { 1.0, 6.5, 6.5 }, .decay = true, .gainDb = 8.1 }) },
        // A narrow pulse and a square two octaves down, fed back, the filter contour opening wide
        { .name = "Bubble Bass",
          .parameters = parameters({ .modMix = 2.5, .modDepth = 10.0, .filterMod = true, .osc = { Osc { .range = Range::Feet32, .wave = Wave::NarrowPulse, .volume = 8.5, .on = true }, Osc { .range = Range::Feet32, .wave = Wave::Square, .volume = 5.0, .on = true }, Osc { .range = Range::Feet32, .wave = Wave::Square, .volume = 0.0 } }, .ext = 7.0, .extOn = true, .cutoff = -3.0, .emphasis = 6.5, .contour = 8.0, .filterAds = { 0.5, 6.5, 1.0 }, .loudnessAds = { 0.5, 5.0, 8.0 }, .decay = true, .gainDb = 4.5 }) },
        // The output fed back into the mixer until it growls
        { .name = "Driven Bass",
          .parameters = parameters({ .modMix = 7.5, .lfo = true, .oscMod = true, .lfoRate = 5.5, .osc = { Osc { .range = Range::Feet32, .wave = Wave::Saw, .volume = 8.5, .on = true }, Osc { .range = Range::Feet32, .wave = Wave::Triangle, .volume = 8.5, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::Saw, .volume = 8.5, .on = true } }, .ext = 8.0, .extOn = true, .cutoff = -2.0, .emphasis = 1.0, .contour = 5.0, .keyboard2 = true, .filterAds = { 0.5, 1.0, 1.0 }, .loudnessAds = { 0.5, 3.0, 9.0 }, .decay = true, .gainDb = 0.5 }) },
        // Square and pulse a fifth apart, sustained, with the filter answering the keyboard
        { .name = "Electric Piano",
          .parameters = parameters({ .glide = 1.0, .modMix = 3.0, .lfo = true, .filterMod = true, .osc = { Osc { .range = Range::Feet4, .wave = Wave::Square, .volume = 2.5, .on = true }, Osc { .range = Range::Feet8, .wave = Wave::Triangle, .volume = 6.5, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::Saw, .volume = 0.0 } }, .cutoff = -1.0, .emphasis = 5.0, .contour = 2.5, .filterAds = { 0.5, 1.8, 1.0 }, .loudnessAds = { 0.5, 5.0, 5.0 }, .decay = true, .gainDb = 9.5 }) },
        // A square, a saw and a narrow pulse two octaves down, all three loud: the mixer overdrives the filter
        { .name = "Fuzz Lead",
          .parameters = parameters({ .glide = 2.0, .modMix = 7.0, .lfo = true, .filterMod = true, .lfoRate = 3.5, .osc = { Osc { .range = Range::Feet16, .wave = Wave::Square, .volume = 6.5, .on = true }, Osc { .range = Range::Feet4, .wave = Wave::Saw, .volume = 7.5, .on = true }, Osc { .range = Range::Feet32, .wave = Wave::NarrowPulse, .volume = 3.0, .on = true } }, .cutoff = 0.0, .emphasis = 2.0, .contour = 2.0, .keyboard2 = true, .filterAds = { 1.0, 3.0, 7.5 }, .loudnessAds = { 1.0, 4.5, 6.5 }, .decay = true, .gainDb = 1.5 }) },
        // A triangle swept down by the filter contour, with a breath of noise for the beater
        { .name = "Kick",
          .parameters = parameters({ .modMix = 1.0, .modDepth = 10.0, .filterEg = true, .oscMod = true, .osc = { Osc { .range = Range::Feet32, .wave = Wave::Triangle, .volume = 8.5, .on = true }, Osc {}, Osc {} }, .noise = 2.0, .noiseOn = true, .cutoff = -0.5, .emphasis = 1.0, .contour = 4.0, .filterAds = { 0.0, 2.0, 1.0 }, .loudnessAds = { 0.0, 4.3, 1.0 }, .decay = true, .gainDb = 4.6 }) },
        // One triangle through a singing filter that follows the keyboard
        { .name = "Resonant Bass",
          .parameters = parameters({ .glide = 2.0,
                                     .osc = { Osc { .range = Range::Feet16, .wave = Wave::Triangle, .volume = 8.5, .on = true }, Osc {}, Osc {} },
                                     .cutoff = -2.0,
                                     .emphasis = 5.0,
                                     .contour = 5.0,
                                     .keyboard2 = true,
                                     .filterAds = { 0.5, 2.0, 2.0 },
                                     .loudnessAds = { 0.5, 2.0, 9.0 },
                                     .decay = true,
                                     .gainDb = 9.0 }) },
        // Two saws and a narrow pulse two octaves down, the filter closed: round and deep
        { .name = "Round Bass",
          .parameters = parameters({ .glide = 1.5, .modMix = 3.0, .osc = { Osc { .range = Range::Feet32, .wave = Wave::Saw, .volume = 8.0, .on = true }, Osc { .range = Range::Feet32, .wave = Wave::Saw, .volume = 2.0, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::NarrowPulse, .volume = 1.5, .on = true } }, .noise = 1.5, .noiseOn = true, .cutoff = -2.0, .emphasis = 3.0, .contour = 6.5, .keyboard1 = false, .filterAds = { 0.5, 2.5, 9.0 }, .loudnessAds = { 0.5, 3.0, 9.5 }, .gainDb = 5.5 }) },
        // White noise through the filter and a short loudness contour
        { .name = "Snare",
          .parameters = parameters({ .osc = { Osc { .on = false }, Osc {}, Osc {} },
                                     .noise = 9.0,
                                     .noiseOn = true,
                                     .cutoff = 0.0,
                                     .emphasis = 1.0,
                                     .contour = 3.5,
                                     .filterAds = { 0.5, 2.0, 1.0 },
                                     .loudnessAds = { 0.5, 5.0, 0.5 },
                                     .decay = true,
                                     .gainDb = 16.1 }) },
        // Three squares, the second and third a few cents apart, with glide
        { .name = "Square Lead",
          .parameters = parameters({ .glide = 2.0, .modMix = 7.0, .lfo = true, .oscMod = true, .filterMod = true, .lfoRate = 5.5, .osc = { Osc { .range = Range::Feet16, .wave = Wave::Square, .volume = 5.0, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::Square, .frequency = 0.08, .volume = 5.0, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::Square, .frequency = -0.08, .volume = 5.0, .on = true } }, .cutoff = 3.0, .emphasis = 1.5, .contour = 1.0, .loudnessAds = { 0.5, 5.0, 9.0 }, .gainDb = -2.9 }) },
        // A single triangle two octaves down
        { .name = "Sub Bass",
          .parameters = parameters({ .glide = 1.0, .modMix = 7.0, .lfo = true, .oscMod = true, .lfoRate = 5.5, .osc = { Osc { .range = Range::Feet32, .wave = Wave::Triangle, .volume = 8.5, .on = true }, Osc {}, Osc {} }, .cutoff = 3.0, .emphasis = 1.0, .contour = 1.0, .loudnessAds = { 0.0, 4.0, 10.0 }, .decay = true, .gainDb = 2.7 }) },
        // A saw and a narrow pulse in the 4' range, a slow swell and a long release
        { .name = "Violin Lead",
          .parameters = parameters({ .glide = 2.0, .modMix = 7.5, .lfo = true, .oscMod = true, .filterMod = true, .lfoRate = 5.5, .osc = { Osc { .range = Range::Feet4, .wave = Wave::Saw, .volume = 5.0, .on = true }, Osc { .range = Range::Feet4, .wave = Wave::NarrowPulse, .volume = 2.0, .on = true }, Osc {} }, .cutoff = 0.0, .emphasis = 2.0, .contour = 3.0, .filterAds = { 0.5, 3.5, 6.5 }, .loudnessAds = { 1.0, 6.5, 6.5 }, .decay = true, .gainDb = 11.7 }) },
        // Noise through a filter close to singing, which the keyboard tunes
        { .name = "Wind",
          .parameters = parameters({ .glide = 1.0, .modMix = 3.0, .filterEg = true, .osc = { Osc { .on = false }, Osc {}, Osc {} }, .noise = 7.0, .noiseOn = true, .cutoff = -1.2, .emphasis = 7.5, .contour = 1.5, .filterAds = { 0.5, 1.5, 1.0 }, .loudnessAds = { 2.0, 3.5, 3.0 }, .decay = true, .gainDb = 21.9 }) },
    };
    return presets;
}

} // namespace noteahead
