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
        // Two saws an octave apart over a third that drones on, off the keyboard; the mod wheel adds the vibrato
        { .name = "Brass",
          .parameters = parameters({ .glide = 2.0, .modMix = 10.0, .lfo = true, .oscMod = true, .lfoRate = 2.0, .osc = { Osc { .range = Range::Feet4, .wave = Wave::Saw, .volume = 5.0, .on = true }, Osc { .range = Range::Feet8, .wave = Wave::Saw, .volume = 2.0, .on = true }, Osc { .range = Range::Feet8, .wave = Wave::Saw, .volume = 4.0, .on = true } }, .osc3Control = false, .cutoff = 1.0, .emphasis = 3.0, .contour = 2.0, .filterAds = { 0.5, 3.0, 3.0 }, .loudnessAds = { 0.5, 6.0, 6.0 }, .decay = true, .gainDb = 6.2 }) },
        // A saw and two squares tuned 3 and 6.5 semitones up, all loud into a ringing filter
        { .name = "Bright Lead",
          .parameters = parameters({ .modMix = 10.0, .lfo = true, .oscMod = true, .lfoRate = 3.0, .osc = { Osc { .range = Range::Feet8, .wave = Wave::Saw, .volume = 8.0, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::Square, .frequency = 3.0, .volume = 8.0, .on = true }, Osc { .range = Range::Feet32, .wave = Wave::Square, .frequency = 6.5, .volume = 9.0, .on = true } }, .cutoff = -3.0, .emphasis = 7.0, .contour = 6.0, .keyboard2 = true, .filterAds = { 0.5, 8.0, 0.0 }, .loudnessAds = { 0.0, 8.0, 10.0 }, .gainDb = 2.1 }) },
        // A narrow and a wide pulse two octaves down, fed back, with OSC 3 bubbling the filter
        { .name = "Bubble Bass",
          .parameters = parameters({ .modDepth = 10.0, .filterMod = true, .osc = { Osc { .range = Range::Feet32, .wave = Wave::NarrowPulse, .volume = 10.0, .on = true }, Osc { .range = Range::Feet32, .wave = Wave::WidePulse, .volume = 5.0, .on = true }, Osc { .range = Range::Feet32, .wave = Wave::Square } }, .ext = 7.0, .extOn = true, .cutoff = -4.0, .emphasis = 6.5, .contour = 8.0, .keyboard1 = false, .filterAds = { 0.0, 6.0, 0.0 }, .loudnessAds = { 0.0, 5.0, 9.0 }, .decay = true, .gainDb = 3.1 }) },
        // The output fed back into the mixer until it growls
        { .name = "Driven Bass",
          .parameters = parameters({ .modMix = 10.0, .lfo = true, .oscMod = true, .lfoRate = 3.0, .osc = { Osc { .range = Range::Feet32, .wave = Wave::Saw, .volume = 10.0, .on = true }, Osc { .range = Range::Feet32, .wave = Wave::Triangle, .volume = 10.0, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::Saw, .volume = 10.0, .on = true } }, .ext = 8.0, .extOn = true, .cutoff = -3.0, .emphasis = 0.0, .contour = 5.0, .keyboard2 = true, .filterAds = { 0.0, 0.3, 0.0 }, .loudnessAds = { 0.0, 3.1, 9.0 }, .decay = true, .gainDb = 7.8 }) },
        // A square and a triangle an octave apart, with OSC 3 shaking the filter at audio rate
        { .name = "Electric Piano",
          .parameters = parameters({ .modDepth = 10.0, .lfo = true, .filterMod = true, .osc = { Osc { .range = Range::Feet4, .wave = Wave::Square, .volume = 3.0, .on = true }, Osc { .range = Range::Feet8, .wave = Wave::Triangle, .volume = 6.0, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::Saw } }, .cutoff = -1.0, .emphasis = 5.0, .contour = 1.0, .keyboard1 = false, .filterAds = { 0.0, 0.5, 0.0 }, .loudnessAds = { 0.0, 5.0, 5.0 }, .decay = true, .gainDb = 9.7 }) },
        // Two narrow pulses and a saw, fed back, through a closed and ringing filter that the contour opens
        { .name = "Funk Bass",
          .parameters = parameters({ .glide = 1.0, .modMix = 10.0, .lfo = true, .oscMod = true, .filterMod = true, .lfoRate = 4.0, .osc = { Osc { .range = Range::Feet2, .wave = Wave::NarrowPulse, .volume = 5.0, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::NarrowPulse, .volume = 9.0, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::Saw, .volume = 10.0, .on = true } }, .ext = 7.0, .extOn = true, .cutoff = -4.0, .emphasis = 7.0, .contour = 6.0, .keyboard1 = false, .keyboard2 = true, .filterAds = { 0.0, 2.0, 4.0 }, .loudnessAds = { 0.0, 5.0, 10.0 }, .gainDb = 17.0 }) },
        // A square, a saw and a wide pulse two octaves down, all three loud: the mixer overdrives the filter
        { .name = "Fuzz Lead",
          .parameters = parameters({ .glide = 2.0, .modMix = 10.0, .lfo = true, .filterMod = true, .lfoRate = 3.0, .osc = { Osc { .range = Range::Feet16, .wave = Wave::Square, .volume = 6.0, .on = true }, Osc { .range = Range::Feet4, .wave = Wave::Saw, .volume = 7.0, .on = true }, Osc { .range = Range::Feet32, .wave = Wave::WidePulse, .volume = 4.0, .on = true } }, .cutoff = 0.0, .emphasis = 2.0, .contour = 2.0, .keyboard2 = true, .filterAds = { 0.5, 3.0, 7.0 }, .loudnessAds = { 0.5, 4.0, 6.0 }, .decay = true, .gainDb = 1.7 }) },
        // White noise and a short loudness contour. The filter follows the whole keyboard, and the sheet
        // has it at 0, which is a hat only three octaves up: at +2 it is one around C3. The sheet's
        // 7 ms decay, which it says to adjust, is a click; 1.3 is a closed hat
        { .name = "Hi-Hat",
          .parameters = parameters({ .filterMod = true, .osc = { Osc { .on = false }, Osc {}, Osc { .range = Range::Feet2, .wave = Wave::NarrowPulse, .volume = 0.0 } }, .noise = 10.0, .noiseOn = true, .cutoff = 2.0, .contour = 5.0, .keyboard2 = true, .filterAds = { 0.0, 0.5, 0.0 }, .loudnessAds = { 0.0, 1.3, 0.0 }, .decay = true, .gainDb = 15.4 }) },
        // A triangle swept down by the filter contour, with a breath of noise for the beater
        { .name = "Kick",
          .parameters = parameters({ .modDepth = 10.0, .filterEg = true, .oscMod = true, .osc = { Osc { .range = Range::Feet32, .wave = Wave::Triangle, .volume = 10.0, .on = true }, Osc {}, Osc {} }, .noise = 2.0, .noiseOn = true, .cutoff = -0.5, .emphasis = 0.0, .contour = 4.0, .filterAds = { 0.0, 0.5, 0.0 }, .loudnessAds = { 0.0, 4.0, 0.0 }, .decay = true, .gainDb = 1.7 }) },
        // One triangle through a singing filter that follows the keyboard
        { .name = "Resonant Bass",
          .parameters = parameters({ .glide = 1.0, .osc = { Osc { .range = Range::Feet16, .wave = Wave::Triangle, .volume = 10.0, .on = true }, Osc {}, Osc {} }, .cutoff = -3.0, .emphasis = 5.0, .contour = 5.0, .keyboard2 = true, .filterAds = { 0.0, 2.0, 2.0 }, .loudnessAds = { 0.0, 2.0, 10.0 }, .decay = true, .gainDb = 6.3 }) },
        // Two saws and a narrow pulse an octave above them, the filter closed: round and deep
        { .name = "Round Bass",
          .parameters = parameters({ .glide = 3.0, .osc = { Osc { .range = Range::Feet32, .wave = Wave::Saw, .volume = 8.0, .on = true }, Osc { .range = Range::Feet32, .wave = Wave::Saw, .volume = 2.0, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::NarrowPulse, .volume = 3.0, .on = true } }, .noise = 1.5, .noiseOn = true, .cutoff = -3.0, .emphasis = 3.0, .contour = 6.0, .keyboard1 = false, .filterAds = { 0.0, 1.3, 0.0 }, .loudnessAds = { 0.0, 3.1, 10.0 }, .decay = true, .gainDb = 6.7 }) },
        // White noise through the filter and a short loudness contour
        { .name = "Snare",
          .parameters = parameters({ .osc = { Osc { .on = false }, Osc {}, Osc {} }, .noise = 10.0, .noiseOn = true, .cutoff = 0.0, .emphasis = 0.0, .contour = 3.0, .filterAds = { 0.0, 2.0, 0.0 }, .loudnessAds = { 0.0, 5.0, 0.0 }, .decay = true, .gainDb = 17.5 }) },
        // Two triangles under a filter swept by OSC 3 off the keyboard; its Frequency sets the rate
        { .name = "Space Wobble",
          .parameters = parameters({ .modDepth = 10.0, .filterMod = true, .osc = { Osc { .range = Range::Feet8, .wave = Wave::Triangle, .volume = 8.0, .on = true }, Osc { .range = Range::Feet32, .wave = Wave::Triangle, .frequency = 4.5, .volume = 10.0, .on = true }, Osc { .range = Range::Lo, .wave = Wave::Square, .frequency = 5.0 } }, .osc3Control = false, .cutoff = -3.0, .emphasis = 5.0, .contour = 7.0, .keyboard1 = false, .filterAds = { 8.0, 6.0, 0.0 }, .loudnessAds = { 0.3, 7.0, 10.0 }, .gainDb = 3.8 }) },
        // Three squares, the second and third a few cents apart, with glide
        { .name = "Square Lead",
          .parameters = parameters({ .glide = 3.0, .modMix = 10.0, .lfo = true, .oscMod = true, .filterMod = true, .lfoRate = 3.0, .osc = { Osc { .range = Range::Feet16, .wave = Wave::Square, .volume = 5.0, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::Square, .frequency = 0.08, .volume = 5.0, .on = true }, Osc { .range = Range::Feet16, .wave = Wave::Square, .frequency = -0.08, .volume = 5.0, .on = true } }, .cutoff = 4.0, .emphasis = 0.0, .contour = 0.0, .keyboard1 = false, .loudnessAds = { 0.0, 5.0, 10.0 }, .gainDb = -4.7 }) },
        // A narrow pulse, a triangle two octaves down and noise, all frequency modulated by OSC 3
        { .name = "Steel Pluck",
          .parameters = parameters({ .modDepth = 10.0, .oscMod = true, .osc = { Osc { .range = Range::Feet8, .wave = Wave::NarrowPulse, .volume = 4.0, .on = true }, Osc { .range = Range::Feet32, .wave = Wave::Triangle, .volume = 5.0, .on = true }, Osc {} }, .noise = 4.0, .noiseOn = true, .cutoff = -2.0, .emphasis = 2.0, .contour = 6.0, .keyboard1 = false, .filterAds = { 0.0, 2.0, 0.0 }, .loudnessAds = { 0.0, 4.0, 0.0 }, .gainDb = 8.9 }) },
        // A single triangle two octaves down
        { .name = "Sub Bass",
          .parameters = parameters({ .modMix = 10.0, .lfo = true, .oscMod = true, .lfoRate = 1.0, .osc = { Osc { .range = Range::Feet32, .wave = Wave::Triangle, .volume = 10.0, .on = true }, Osc {}, Osc {} }, .cutoff = 4.0, .emphasis = 0.0, .contour = 0.0, .keyboard1 = false, .decay = true, .gainDb = -0.5 }) },
        // A low triangle-saw whose pitch the filter contour drops, with the output fed back and a little noise
        { .name = "Toms",
          .parameters = parameters({ .modDepth = 10.0, .filterEg = true, .lfo = true, .oscMod = true, .osc = { Osc { .range = Range::Feet32, .wave = Wave::TriangleSaw, .volume = 5.0, .on = true }, Osc {}, Osc {} }, .ext = 7.0, .extOn = true, .noise = 2.0, .noiseOn = true, .cutoff = -2.0, .contour = 4.0, .filterAds = { 0.0, 0.5, 0.0 }, .loudnessAds = { 0.0, 4.0, 0.0 }, .decay = true, .gainDb = 8.7 }) },
        // One triangle in the 2' range with a long glide; the mod wheel adds the vibrato. Keyboard Control
        // 2 is on as well, where the sheet has only 1: the filter then follows the triangle, which
        // otherwise loses 30 dB between C2 and C5
        { .name = "Triangle Lead",
          .parameters = parameters({ .glide = 4.5, .modMix = 10.0, .lfo = true, .oscMod = true, .lfoRate = 3.0, .osc = { Osc { .range = Range::Feet2, .wave = Wave::Triangle, .volume = 5.0, .on = true }, Osc {}, Osc {} }, .cutoff = 0.0, .contour = 1.0, .keyboard2 = true, .filterAds = { 0.5, 0.5, 0.0 }, .loudnessAds = { 0.6, 3.0, 10.0 }, .decay = true, .gainDb = 18.2 }) },
        // A saw and a narrow pulse in the 4' range, a slow swell and a long release
        { .name = "Violin Lead",
          .parameters = parameters({ .glide = 2.0, .modMix = 10.0, .lfo = true, .oscMod = true, .filterMod = true, .lfoRate = 3.0, .osc = { Osc { .range = Range::Feet4, .wave = Wave::Saw, .volume = 5.0, .on = true }, Osc { .range = Range::Feet4, .wave = Wave::NarrowPulse, .volume = 2.0, .on = true }, Osc {} }, .cutoff = 0.0, .emphasis = 1.5, .contour = 3.0, .filterAds = { 0.5, 3.0, 6.0 }, .loudnessAds = { 0.5, 6.0, 6.0 }, .decay = true, .gainDb = 11.1 }) },
        // Noise through a filter close to singing, which the keyboard tunes
        { .name = "Wind",
          .parameters = parameters({ .filterEg = true, .osc = { Osc { .on = false }, Osc {}, Osc {} }, .noise = 7.0, .noiseOn = true, .cutoff = -2.0, .emphasis = 7.0, .contour = 0.0, .keyboard2 = true, .filterAds = { 0.0, 0.0, 0.0 }, .loudnessAds = { 0.5, 4.0, 3.0 }, .decay = true, .gainDb = 28.5 }) },
    };
    return presets;
}

} // namespace noteahead
