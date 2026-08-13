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

#include "synth_presets.hpp"

#include "../../common/parameter_mapper.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

// Every patch below is written in the units it is thought in -- hertz, seconds -- and converted
// here. The list this replaced was written in raw knob positions, and it is where the two most
// common faults in it came from: cutoffs set as though the knob were linear in frequency, which
// left a quarter of the presets filtered down to a few tens of hertz, and modulation targets given
// by guessed ordinal, which aimed several filter sweeps at the pitch of an oscillator that was not
// even turned up.

//! Knob position for a filter corner. This is the curve the filter itself runs on --
//! 20 Hz * (ceiling / 20 Hz) ^ knob -- and not the one the readout uses, which differs at the very
//! bottom of the travel. The ceiling is 20 kHz at any ordinary sample rate.
float cutoffKnob(double hz)
{
    constexpr double lowest = 20.0;
    constexpr double ceiling = 20000.0;
    return static_cast<float>(std::log(std::clamp(hz, lowest, ceiling) / lowest) / std::log(ceiling / lowest));
}

float attackKnob(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.000001, 20.0));
}

float decayKnob(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.01, 60.0));
}

float releaseKnob(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.001, 60.0));
}

float lfoRateKnob(double hz)
{
    return static_cast<float>(ParameterMapper::unmapLfoFrequency(hz, 0.05, 20.0));
}

//! The intensity knobs are bipolar and their centre is off, so a depth is written as an offset
//! from the middle rather than as an absolute position. Positive opens, negative closes.
float intensity(double amount)
{
    return static_cast<float>(0.5 + amount * 0.5);
}

// Oscillator waveforms, as PolyBlepOscillator orders them.
constexpr float Triangle = 0.0f;
constexpr float Saw = 1.0f;
constexpr float Square = 2.0f;
constexpr float Sine = 3.0f;

// Modulation destinations, by the ordinal each enum actually persists.
constexpr float ModCutoff = 3.0f;
constexpr float LfoPitch = 0.0f;
constexpr float LfoCutoff = 2.0f;

// Voice modes.
constexpr float Mono = 5.0f;

} // namespace

const std::vector<SynthPreset> & SynthPresets::presets()
{
    static const std::vector<SynthPreset> presetList = [] {
        std::vector<SynthPreset> list;

        list.push_back({ "Init", {} });

        // --- Bass ---

        list.push_back({ "Fat Saw Bass", {
                                           { "vco2Waveform", Saw },
                                           { "vco2Pitch", 0.52f },
                                           { "mixLevel2", 0.8f },
                                           { "lpfCutoff", cutoffKnob(900.0) },
                                           { "lpfResonance", 0.2f },
                                           { "modTarget", ModCutoff },
                                           { "modIntensity", intensity(0.45) },
                                           { "modAttack", attackKnob(0.001) },
                                           { "modDecay", decayKnob(0.35) },
                                           { "ampAttack", attackKnob(0.002) },
                                           { "ampDecay", decayKnob(0.8) },
                                           { "ampSustain", 0.6f },
                                           { "ampRelease", releaseKnob(0.15) },
                                         } });

        list.push_back({ "Sub Bass", {
                                       { "vco1Waveform", Sine },
                                       { "vco2Waveform", Triangle },
                                       { "vco2Octave", -1.0f },
                                       { "mixLevel2", 0.6f },
                                       { "lpfCutoff", cutoffKnob(1200.0) },
                                       { "ampAttack", attackKnob(0.004) },
                                       { "ampSustain", 1.0f },
                                       { "ampRelease", releaseKnob(0.12) },
                                     } });

        // The sweep this is named for: the mod envelope on the cutoff, no sustain, so every note
        // opens and closes again. The old preset of this name pointed it at VCO3's pitch instead,
        // with VCO3 turned down, so it did nothing whatsoever.
        list.push_back({ "Acid Line", {
                                        { "vco1Waveform", Saw },
                                        { "lpfCutoff", cutoffKnob(220.0) },
                                        { "lpfResonance", 0.8f },
                                        { "modTarget", ModCutoff },
                                        { "modIntensity", intensity(0.75) },
                                        { "modAttack", attackKnob(0.001) },
                                        { "modDecay", decayKnob(0.28) },
                                        { "modSustain", 0.0f },
                                        { "voiceMode", Mono },
                                        { "portamento", 0.12f },
                                        { "ampAttack", attackKnob(0.001) },
                                        { "ampDecay", decayKnob(0.6) },
                                        { "ampSustain", 0.35f },
                                        { "ampRelease", releaseKnob(0.1) },
                                      } });

        list.push_back({ "Reese", {
                                    { "vco1Waveform", Saw },
                                    { "vco2Waveform", Saw },
                                    { "vco2Pitch", 0.53f },
                                    { "mixLevel2", 1.0f },
                                    { "lpfCutoff", cutoffKnob(1100.0) },
                                    { "lfoTarget", LfoCutoff },
                                    { "lfoRate", lfoRateKnob(0.35) },
                                    { "lfoIntensity", intensity(0.25) },
                                    { "ampAttack", attackKnob(0.01) },
                                    { "ampSustain", 1.0f },
                                    { "ampRelease", releaseKnob(0.2) },
                                  } });

        // --- Leads ---

        list.push_back({ "Saw Lead", {
                                       { "vco1Waveform", Saw },
                                       { "vco2Waveform", Saw },
                                       { "vco2Pitch", 0.51f },
                                       { "mixLevel2", 0.7f },
                                       { "lpfCutoff", cutoffKnob(6000.0) },
                                       { "lpfResonance", 0.15f },
                                       { "voiceMode", Mono },
                                       { "portamento", 0.1f },
                                       { "ampAttack", attackKnob(0.005) },
                                       { "ampSustain", 0.9f },
                                       { "ampRelease", releaseKnob(0.15) },
                                     } });

        list.push_back({ "Square Lead", {
                                          { "vco1Waveform", Square },
                                          { "vco1Shape", 0.35f },
                                          { "lpfCutoff", cutoffKnob(5000.0) },
                                          { "voiceMode", Mono },
                                          { "portamento", 0.15f },
                                          { "ampAttack", attackKnob(0.004) },
                                          { "ampSustain", 0.9f },
                                          { "ampRelease", releaseKnob(0.2) },
                                          { "delayMix", 0.3f },
                                          { "delayTime", 0.5f },
                                        } });

        list.push_back({ "Sync Lead", {
                                        { "vco1Waveform", Saw },
                                        { "vco2Waveform", Saw },
                                        { "vco2Sync", 1.0f },
                                        { "vco2Pitch", 0.72f },
                                        { "mixLevel1", 0.4f },
                                        { "mixLevel2", 1.0f },
                                        { "lpfCutoff", cutoffKnob(8000.0) },
                                        { "modTarget", 1.0f }, // VCO2 pitch: the sweep a sync lead is made of
                                        { "modIntensity", intensity(0.5) },
                                        { "modAttack", attackKnob(0.001) },
                                        { "modDecay", decayKnob(0.5) },
                                        { "voiceMode", Mono },
                                        { "ampAttack", attackKnob(0.003) },
                                        { "ampSustain", 0.85f },
                                        { "ampRelease", releaseKnob(0.15) },
                                      } });

        // --- Pads ---

        list.push_back({ "Warm Pad", {
                                       { "vco1Waveform", Saw },
                                       { "vco2Waveform", Saw },
                                       { "vco2Pitch", 0.54f },
                                       { "mixLevel2", 0.8f },
                                       { "lpfCutoff", cutoffKnob(1800.0) },
                                       { "panSpread", 0.8f },
                                       { "oscillatorDrift", 0.3f },
                                       { "ampAttack", attackKnob(0.35) },
                                       { "ampSustain", 1.0f },
                                       { "ampRelease", releaseKnob(1.5) },
                                     } });

        list.push_back({ "String Pad", {
                                         { "vco1Waveform", Saw },
                                         { "vco2Waveform", Saw },
                                         { "vco2Pitch", 0.55f },
                                         { "vco3Waveform", Saw },
                                         { "vco3Octave", 1.0f },
                                         { "mixLevel2", 0.8f },
                                         { "mixLevel3", 0.4f },
                                         { "lpfCutoff", cutoffKnob(3500.0) },
                                         { "panSpread", 0.9f },
                                         { "oscillatorDrift", 0.3f },
                                         { "ampAttack", attackKnob(0.35) },
                                         { "ampSustain", 1.0f },
                                         { "ampRelease", releaseKnob(1.0) },
                                       } });

        // A pad that moves: the LFO walks the corner slowly up and down, which is the one thing a
        // sustained patch needs so that a held chord does not stand still.
        list.push_back({ "Sweep Pad", {
                                        { "vco1Waveform", Saw },
                                        { "vco2Waveform", Square },
                                        { "vco2Pitch", 0.53f },
                                        { "mixLevel2", 0.6f },
                                        { "lpfCutoff", cutoffKnob(1200.0) },
                                        { "lpfResonance", 0.35f },
                                        { "lfoTarget", LfoCutoff },
                                        { "lfoRate", lfoRateKnob(0.12) },
                                        { "lfoIntensity", intensity(0.5) },
                                        { "panSpread", 0.6f },
                                        { "ampAttack", attackKnob(0.5) },
                                        { "ampSustain", 1.0f },
                                        { "ampRelease", releaseKnob(1.2) },
                                      } });

        list.push_back({ "Glass Pad", {
                                        { "vco1Waveform", Sine },
                                        { "vco2Waveform", Triangle },
                                        { "vco2Octave", 1.0f },
                                        { "mixLevel2", 0.7f },
                                        // Above a middle C's fundamental this stops thinning the sound and starts removing it.
                                        { "hpfCutoff", cutoffKnob(150.0) },
                                        { "lpfCutoff", cutoffKnob(9000.0) },
                                        { "panSpread", 0.8f },
                                        { "ampAttack", attackKnob(0.45) },
                                        { "ampSustain", 1.0f },
                                        { "ampRelease", releaseKnob(1.4) },
                                        { "delayMix", 0.25f },
                                      } });

        // --- Keys and plucks ---

        list.push_back({ "Pluck", {
                                    { "vco1Waveform", Saw },
                                    { "lpfCutoff", cutoffKnob(500.0) },
                                    { "lpfResonance", 0.3f },
                                    { "modTarget", ModCutoff },
                                    { "modIntensity", intensity(0.7) },
                                    { "modAttack", attackKnob(0.001) },
                                    { "modDecay", decayKnob(0.12) },
                                    { "modSustain", 0.0f },
                                    { "ampAttack", attackKnob(0.001) },
                                    { "ampDecay", decayKnob(0.45) },
                                    { "ampSustain", 0.0f },
                                    { "ampRelease", releaseKnob(0.25) },
                                    { "ampCurve", 0.6f },
                                  } });

        list.push_back({ "Clav", {
                                   { "vco1Waveform", Square },
                                   { "vco1Shape", 0.7f },
                                   { "lpfCutoff", cutoffKnob(2500.0) },
                                   { "lpfResonance", 0.25f },
                                   { "hpfCutoff", cutoffKnob(200.0) },
                                   { "modTarget", ModCutoff },
                                   { "modIntensity", intensity(0.5) },
                                   { "modAttack", attackKnob(0.001) },
                                   { "modDecay", decayKnob(0.08) },
                                   { "ampAttack", attackKnob(0.001) },
                                   { "ampDecay", decayKnob(0.3) },
                                   { "ampSustain", 0.1f },
                                   { "ampRelease", releaseKnob(0.12) },
                                   { "ampCurve", 0.6f },
                                 } });

        list.push_back({ "Organ", {
                                    { "vco1Waveform", Sine },
                                    { "vco2Waveform", Sine },
                                    { "vco2Octave", 1.0f },
                                    { "vco3Waveform", Sine },
                                    { "vco3Octave", 2.0f },
                                    { "mixLevel2", 0.7f },
                                    { "mixLevel3", 0.4f },
                                    { "ampAttack", attackKnob(0.005) },
                                    { "ampSustain", 1.0f },
                                    { "ampRelease", releaseKnob(0.03) },
                                  } });

        list.push_back({ "Bell", {
                                   { "vco1Waveform", Sine },
                                   { "vco2Waveform", Sine },
                                   { "vco2Octave", 1.0f },
                                   { "vco2Pitch", 0.62f },
                                   { "mixLevel2", 0.5f },
                                   { "lpfCutoff", cutoffKnob(7000.0) },
                                   { "ampAttack", attackKnob(0.002) },
                                   { "ampDecay", decayKnob(2.5) },
                                   { "ampSustain", 0.0f },
                                   { "ampRelease", releaseKnob(1.2) },
                                   { "ampCurve", 0.6f },
                                 } });

        // --- Texture ---

        list.push_back({ "Wobble", {
                                     { "vco1Waveform", Saw },
                                     { "vco2Waveform", Saw },
                                     { "vco2Pitch", 0.53f },
                                     { "mixLevel2", 0.9f },
                                     { "lpfCutoff", cutoffKnob(400.0) },
                                     { "lpfResonance", 0.6f },
                                     { "lfoTarget", LfoCutoff },
                                     { "lfoRate", lfoRateKnob(5.0) },
                                     { "lfoIntensity", intensity(0.6) },
                                     { "ampAttack", attackKnob(0.005) },
                                     { "ampSustain", 1.0f },
                                     { "ampRelease", releaseKnob(0.2) },
                                   } });

        list.push_back({ "Vibrato Flute", {
                                            { "vco1Waveform", Sine },
                                            { "vco2Waveform", Triangle },
                                            { "mixLevel2", 0.35f },
                                            { "lpfCutoff", cutoffKnob(4000.0) },
                                            { "lfoTarget", LfoPitch },
                                            { "lfoRate", lfoRateKnob(5.5) },
                                            { "lfoIntensity", intensity(0.05) },
                                            { "voiceMode", Mono },
                                            { "portamento", 0.08f },
                                            { "ampAttack", attackKnob(0.08) },
                                            { "ampSustain", 1.0f },
                                            { "ampRelease", releaseKnob(0.25) },
                                          } });

        return list;
    }();

    return presetList;
}

} // namespace noteahead
