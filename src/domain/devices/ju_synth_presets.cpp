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

#include "ju_synth_presets.hpp"

#include "../../common/parameter_mapper.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

// Written in the units the patches are thought in and converted here, on the curves the device runs
// its controls on.

float cutoffKnob(double hz)
{
    return static_cast<float>(std::log(std::clamp(hz, 20.0, 20000.0) / 20.0) / std::log(1000.0));
}

float attackKnob(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.001, 3.0));
}

float decayKnob(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.002, 12.0));
}

float releaseKnob(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.002, 12.0));
}

float lfoRateKnob(double hz)
{
    return static_cast<float>(ParameterMapper::unmapExponential(hz, 0.1, 20.0));
}

//! LFO delay, as a share of its three seconds.
float lfoDelayKnob(double seconds)
{
    return static_cast<float>(std::clamp(seconds / 3.0, 0.0, 1.0));
}

// DCO range, PWM source, VCA mode and chorus mode, by the ordinals the device persists.
constexpr float Range16 = 0.0f;
constexpr float PwmLfo = 0.0f;
constexpr float PwmManual = 1.0f;
constexpr float VcaGate = 1.0f;
constexpr float ChorusI = 1.0f;
constexpr float ChorusII = 2.0f;
constexpr float ChorusBoth = 3.0f;
constexpr float On = 1.0f;
constexpr float Off = 0.0f;

} // namespace

const std::vector<SynthPreset> & JuSynthPresets::presets()
{
    static const std::vector<SynthPreset> presetList = [] {
        std::vector<SynthPreset> list;

        list.push_back({ "Init", {} });

        // A saw through Chorus II with a delayed vibrato: the string machine every Juno owner has.
        list.push_back({ "Juno Strings", {
                                           { "lpfCutoff", cutoffKnob(3200.0) },
                                           { "keyTrack", 0.5f },
                                           { "lfoRate", lfoRateKnob(5.5) },
                                           { "lfoDelay", lfoDelayKnob(0.6) },
                                           { "pitchLfoDepth", 0.12f },
                                           { "attack", attackKnob(0.25) },
                                           { "decay", decayKnob(1.0) },
                                           { "sustain", 0.85f },
                                           { "release", releaseKnob(0.8) },
                                           { "chorusMode", ChorusII },
                                         } });

        // Saw and a slowly swept pulse under Chorus I.
        list.push_back({ "Juno Pad", {
                                       { "pulseEnabled", On },
                                       { "pwmSource", PwmLfo },
                                       { "pulseWidth", 0.6f },
                                       { "lfoRate", lfoRateKnob(0.6) },
                                       { "lpfCutoff", cutoffKnob(1600.0) },
                                       { "lpfResonance", 0.2f },
                                       { "keyTrack", 0.4f },
                                       { "attack", attackKnob(0.6) },
                                       { "release", releaseKnob(1.5) },
                                       { "chorusMode", ChorusI },
                                     } });

        // The envelope opening a dark saw on every note.
        list.push_back({ "Brass", {
                                    { "lpfCutoff", cutoffKnob(500.0) },
                                    { "envMod", 0.35f },
                                    { "keyTrack", 0.5f },
                                    { "attack", attackKnob(0.06) },
                                    { "decay", decayKnob(0.5) },
                                    { "sustain", 0.6f },
                                    { "release", releaseKnob(0.2) },
                                    { "chorusMode", ChorusI },
                                  } });

        // Saw and the full sub an octave down, dry and short.
        list.push_back({ "Sub Bass", {
                                       { "range", Range16 },
                                       { "subLevel", 1.0f },
                                       { "lpfCutoff", cutoffKnob(350.0) },
                                       { "lpfResonance", 0.3f },
                                       { "envMod", 0.3f },
                                       { "decay", decayKnob(0.3) },
                                       { "sustain", 0.4f },
                                       { "release", releaseKnob(0.08) },
                                     } });

        // A narrow pulse and the sub, the bass the Juno-106 is sampled for.
        list.push_back({ "Juno Bass", {
                                        { "range", Range16 },
                                        { "sawEnabled", Off },
                                        { "pulseEnabled", On },
                                        { "pwmSource", PwmManual },
                                        { "pulseWidth", 0.35f },
                                        { "subLevel", 0.9f },
                                        { "lpfCutoff", cutoffKnob(350.0) },
                                        { "lpfResonance", 0.25f },
                                        { "envMod", 0.45f },
                                        { "decay", decayKnob(0.3) },
                                        { "sustain", 0.35f },
                                        { "release", releaseKnob(0.1) },
                                      } });

        list.push_back({ "Pluck", {
                                    { "lpfCutoff", cutoffKnob(350.0) },
                                    { "envMod", 0.5f },
                                    { "keyTrack", 0.6f },
                                    { "decay", decayKnob(0.35) },
                                    { "sustain", 0.0f },
                                    { "release", releaseKnob(0.35) },
                                    { "chorusMode", ChorusII },
                                  } });

        // Square and sub on the gate, through the fast I+II wobble.
        list.push_back({ "Organ", {
                                    { "sawEnabled", Off },
                                    { "pulseEnabled", On },
                                    { "subLevel", 0.6f },
                                    { "lpfCutoff", cutoffKnob(4000.0) },
                                    { "vcaMode", VcaGate },
                                    { "chorusMode", ChorusBoth },
                                  } });

        list.push_back({ "PWM Lead", {
                                       { "sawEnabled", Off },
                                       { "pulseEnabled", On },
                                       { "pwmSource", PwmLfo },
                                       { "pulseWidth", 0.5f },
                                       { "lfoRate", lfoRateKnob(3.0) },
                                       { "lfoDelay", lfoDelayKnob(0.4) },
                                       { "pitchLfoDepth", 0.1f },
                                       { "lpfCutoff", cutoffKnob(2500.0) },
                                       { "lpfResonance", 0.25f },
                                       { "portamento", 0.3f },
                                       { "release", releaseKnob(0.25) },
                                       { "chorusMode", ChorusI },
                                     } });

        // Everything at once under I+II: thick, wide and moving.
        list.push_back({ "PWM Stack", {
                                        { "pulseEnabled", On },
                                        { "pwmSource", PwmLfo },
                                        { "pulseWidth", 0.8f },
                                        { "lfoRate", lfoRateKnob(1.0) },
                                        { "subLevel", 0.5f },
                                        { "lpfCutoff", cutoffKnob(2000.0) },
                                        { "portamento", 0.2f },
                                        { "release", releaseKnob(0.4) },
                                        { "chorusMode", ChorusBoth },
                                      } });

        // A swept pulse with a little noise, and resonance where a vowel would have its formant.
        list.push_back({ "Choir", {
                                    { "sawEnabled", Off },
                                    { "pulseEnabled", On },
                                    { "pwmSource", PwmLfo },
                                    { "pulseWidth", 0.7f },
                                    { "lfoRate", lfoRateKnob(0.8) },
                                    { "noiseLevel", 0.05f },
                                    { "lpfCutoff", cutoffKnob(1200.0) },
                                    { "lpfResonance", 0.35f },
                                    { "attack", attackKnob(0.4) },
                                    { "release", releaseKnob(1.2) },
                                    { "chorusMode", ChorusII },
                                  } });

        // The filter LFO doing the work: a slow sweep through a resonant saw.
        list.push_back({ "Sweep Pad", {
                                        { "lpfCutoff", cutoffKnob(400.0) },
                                        { "lpfResonance", 0.45f },
                                        { "filterLfoDepth", 0.4f },
                                        { "lfoRate", lfoRateKnob(0.15) },
                                        { "attack", attackKnob(1.0) },
                                        { "release", releaseKnob(2.0) },
                                        { "chorusMode", ChorusI },
                                      } });

        return list;
    }();
    return presetList;
}

} // namespace noteahead
