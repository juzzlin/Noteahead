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

#include "wavetable_synth_presets.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "wavetable_synth_device.hpp"

#include <cmath>

namespace noteahead {

namespace {

namespace C = Constants::NahdXml;

// The knob curves the device maps its parameters through, inverted, so that the patches can be
// written in the units they are thought in.

float attack(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.001, 10.0));
}

//! Decay and release.
float time(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.01, 10.0));
}

//! The low pass runs logarithmically over 20 Hz to 20 kHz.
float cutoff(double hz)
{
    return static_cast<float>(std::log2(hz / 20.0) / std::log2(1000.0));
}

//! A bipolar intensity: the share of the target's whole range, in octaves for pitch.
float depth(double amount)
{
    return static_cast<float>((ParameterMapper::unmapCubicCentered(amount, -1.0, 1.0) + 1.0) * 0.5);
}

float lfoRate(double hz)
{
    return static_cast<float>(ParameterMapper::unmapLfoFrequency(hz, 0.05, 20.0));
}

float lfoTime(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapCubic(seconds, 0.0, Constants::maxLfoEnvelopeTimeSeconds()));
}

float cents(double amount)
{
    return static_cast<float>((ParameterMapper::unmapCubicCentered(amount, -1200.0, 1200.0) + 1.0) * 0.5);
}

float portamento(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.01, 2.0));
}

//! A cent, in the octaves pitch modulation is counted in.
constexpr double Cent = 1.0 / 1200.0;

float choice(auto value)
{
    return static_cast<float>(static_cast<int>(value));
}

std::string key(const QString & name)
{
    return name.toStdString();
}

using Mode = WavetableSynthDevice::VoiceMode;
using Mod = WavetableSynthDevice::ModTarget;
using LfoDest = WavetableSynthDevice::LfoTarget;

// The tables, by the ordinal the parameter persists
constexpr float ClassicMorph = 0.0f;
constexpr float SpectralAdditive = 1.0f;
constexpr float PulseWidth = 2.0f;
constexpr float VocalFormant = 3.0f;
constexpr float ResonantSweep = 4.0f;
constexpr float OrganDrawbar = 5.0f;

// Positions on Classic Morph, which runs sine, triangle, saw, square
constexpr float Triangle = 0.33f;
constexpr float Saw = 0.66f;

constexpr float SineWave = 3.0f;

} // namespace

const std::vector<SynthPreset> & WavetableSynthPresets::presets()
{
    // Init first, as the way back; the rest alphabetical
    static const std::vector<SynthPreset> presetList {
        { "Init", {} },

        // A harmonic series made to ring like a bell: the second oscillator a twelfth up, and the
        // spectrum closing from bright to plain as the note dies away.
        { "Digital Bell", {
                            { key(C::xmlKeyWavetableIndex()), SpectralAdditive },
                            { key(C::xmlKeyOsc1Pos()), 0.25f },
                            { key(C::xmlKeyOsc2Level()), 0.5f },
                            { key(C::xmlKeyOsc2Octave()), 1.0f },
                            { key(C::xmlKeyOsc2Pitch()), cents(702.0) },
                            { key(C::xmlKeyOsc2Pos()), 0.1f },
                            { key(C::xmlKeyAmpAttack()), attack(0.001) },
                            { key(C::xmlKeyAmpDecay()), time(2.0) },
                            { key(C::xmlKeyAmpSustain()), 0.0f },
                            { key(C::xmlKeyAmpRelease()), time(1.5) },
                            { key(C::xmlKeyModTarget()), choice(Mod::Osc1Pos) },
                            { key(C::xmlKeyModAttack()), attack(0.001) },
                            { key(C::xmlKeyModDecay()), time(0.6) },
                            { key(C::xmlKeyModIntensity()), depth(0.5) },
                          } },

        // The Jazz registration, and a slow rotary sway in the pan and level.
        { "Drawbar Organ", {
                             { key(C::xmlKeyWavetableIndex()), OrganDrawbar },
                             { key(C::xmlKeyOsc1Pos()), 0.5f },
                             { key(C::xmlKeyAmpAttack()), attack(0.003) },
                             { key(C::xmlKeyAmpDecay()), time(0.1) },
                             { key(C::xmlKeyAmpSustain()), 1.0f },
                             { key(C::xmlKeyAmpRelease()), time(0.08) },
                             { key(C::xmlKeyLfoWaveform()), SineWave },
                             { key(C::xmlKeyLfoTarget()), choice(LfoDest::Pan) },
                             { key(C::xmlKeyLfoRate()), lfoRate(0.8) },
                             { key(C::xmlKeyLfoIntensity()), depth(0.35) },
                             { key(C::xmlKeyLfo2Waveform()), SineWave },
                             { key(C::xmlKeyLfo2Target()), choice(LfoDest::Volume) },
                             { key(C::xmlKeyLfo2Rate()), lfoRate(0.8) },
                             { key(C::xmlKeyLfo2Intensity()), depth(0.08) },
                           } },

        // The position drifting from triangle towards saw as the note is held, with a second
        // oscillator wandering on its own over several seconds.
        { "Morph Pad", {
                         { key(C::xmlKeyWavetableIndex()), ClassicMorph },
                         { key(C::xmlKeyOsc1Pos()), Triangle },
                         { key(C::xmlKeyOsc2Level()), 0.6f },
                         { key(C::xmlKeyOsc2Pos()), Saw },
                         { key(C::xmlKeyOsc2Pitch()), cents(7.0) },
                         { key(C::xmlKeyLpfCutoff()), cutoff(6000.0) },
                         { key(C::xmlKeyAmpAttack()), attack(0.8) },
                         { key(C::xmlKeyAmpDecay()), time(1.5) },
                         { key(C::xmlKeyAmpSustain()), 0.85f },
                         { key(C::xmlKeyAmpRelease()), time(1.8) },
                         { key(C::xmlKeyModTarget()), choice(Mod::Osc1Pos) },
                         { key(C::xmlKeyModAttack()), attack(2.5) },
                         { key(C::xmlKeyModDecay()), time(4.0) },
                         { key(C::xmlKeyModSustain()), 0.6f },
                         { key(C::xmlKeyModIntensity()), depth(0.4) },
                         { key(C::xmlKeyLfo2Waveform()), SineWave },
                         { key(C::xmlKeyLfo2Target()), choice(LfoDest::Osc2Pos) },
                         { key(C::xmlKeyLfo2Rate()), lfoRate(0.2) },
                         { key(C::xmlKeyLfo2RateDivider()), 4.0f },
                         { key(C::xmlKeyLfo2Intensity()), depth(0.15) },
                         { key(C::xmlKeyVoiceMode()), choice(Mode::Unison) },
                         { key(C::xmlKeyVoiceDepth()), 0.25f },
                         { key(C::xmlKeyPanSpread()), 0.7f },
                       } },

        // Mono and an octave down, the filter snapping open on every note.
        { "Pulse Bass", {
                          { key(C::xmlKeyWavetableIndex()), PulseWidth },
                          { key(C::xmlKeyOsc1Pos()), 0.2f },
                          { key(C::xmlKeyOsc1Octave()), -1.0f },
                          { key(C::xmlKeyOsc2Level()), 0.5f },
                          { key(C::xmlKeyOsc2Octave()), -1.0f },
                          { key(C::xmlKeyOsc2Pos()), 0.0f },
                          { key(C::xmlKeyLpfCutoff()), cutoff(350.0) },
                          { key(C::xmlKeyLpfResonance()), 0.35f },
                          { key(C::xmlKeyAmpAttack()), attack(0.002) },
                          { key(C::xmlKeyAmpDecay()), time(0.4) },
                          { key(C::xmlKeyAmpSustain()), 0.7f },
                          { key(C::xmlKeyAmpRelease()), time(0.12) },
                          { key(C::xmlKeyModTarget()), choice(Mod::Cutoff) },
                          { key(C::xmlKeyModAttack()), attack(0.001) },
                          { key(C::xmlKeyModDecay()), time(0.25) },
                          { key(C::xmlKeyModIntensity()), depth(0.45) },
                          { key(C::xmlKeyVoiceMode()), choice(Mode::Mono) },
                          { key(C::xmlKeyPortamento()), portamento(0.03) },
                        } },

        // Two pulses whose widths are swept by LFOs at different rates, the string machine trick.
        { "PWM Strings", {
                           { key(C::xmlKeyWavetableIndex()), PulseWidth },
                           { key(C::xmlKeyOsc1Pos()), 0.3f },
                           { key(C::xmlKeyOsc2Level()), 0.7f },
                           { key(C::xmlKeyOsc2Pos()), 0.5f },
                           { key(C::xmlKeyOsc2Pitch()), cents(-8.0) },
                           { key(C::xmlKeyLpfCutoff()), cutoff(5000.0) },
                           { key(C::xmlKeyAmpAttack()), attack(0.4) },
                           { key(C::xmlKeyAmpDecay()), time(1.0) },
                           { key(C::xmlKeyAmpSustain()), 0.9f },
                           { key(C::xmlKeyAmpRelease()), time(0.9) },
                           { key(C::xmlKeyLfoTarget()), choice(LfoDest::Osc1Pos) },
                           { key(C::xmlKeyLfoRate()), lfoRate(0.6) },
                           { key(C::xmlKeyLfoIntensity()), depth(0.25) },
                           { key(C::xmlKeyLfo2Target()), choice(LfoDest::Osc2Pos) },
                           { key(C::xmlKeyLfo2Rate()), lfoRate(0.45) },
                           { key(C::xmlKeyLfo2Intensity()), depth(0.25) },
                         } },

        // The resonant peak climbing the saw as the mod envelope opens, and settling part way.
        { "Resonant Sweep", {
                              { key(C::xmlKeyWavetableIndex()), ResonantSweep },
                              { key(C::xmlKeyOsc1Pos()), 0.0f },
                              { key(C::xmlKeyAmpAttack()), attack(0.005) },
                              { key(C::xmlKeyAmpDecay()), time(1.0) },
                              { key(C::xmlKeyAmpSustain()), 0.7f },
                              { key(C::xmlKeyAmpRelease()), time(0.5) },
                              { key(C::xmlKeyModTarget()), choice(Mod::Osc1Pos) },
                              { key(C::xmlKeyModAttack()), attack(0.8) },
                              { key(C::xmlKeyModDecay()), time(2.5) },
                              { key(C::xmlKeyModSustain()), 0.3f },
                              { key(C::xmlKeyModIntensity()), depth(0.9) },
                            } },

        // Seven saws, a little glide, and a vibrato that arrives once the note has been held.
        { "Supersaw Lead", {
                             { key(C::xmlKeyWavetableIndex()), ClassicMorph },
                             { key(C::xmlKeyOsc1Pos()), Saw },
                             { key(C::xmlKeyLpfCutoff()), cutoff(12000.0) },
                             { key(C::xmlKeyAmpAttack()), attack(0.005) },
                             { key(C::xmlKeyAmpDecay()), time(0.4) },
                             { key(C::xmlKeyAmpSustain()), 0.85f },
                             { key(C::xmlKeyAmpRelease()), time(0.3) },
                             { key(C::xmlKeyLfoWaveform()), SineWave },
                             { key(C::xmlKeyLfoTarget()), choice(LfoDest::Pitch) },
                             { key(C::xmlKeyLfoRate()), lfoRate(5.5) },
                             { key(C::xmlKeyLfoIntensity()), depth(15.0 * Cent) },
                             { key(C::xmlKeyLfoDelay()), lfoTime(0.35) },
                             { key(C::xmlKeyLfoFade()), lfoTime(0.6) },
                             { key(C::xmlKeyVoiceMode()), choice(Mode::Supersaw) },
                             { key(C::xmlKeyVoiceDepth()), 0.45f },
                             { key(C::xmlKeyPortamento()), portamento(0.06) },
                           } },

        // The vowels drifting by slowly, a stack of voices, and a late vibrato.
        { "Vocal Choir", {
                           { key(C::xmlKeyWavetableIndex()), VocalFormant },
                           { key(C::xmlKeyOsc1Pos()), 0.5f },
                           { key(C::xmlKeyLpfCutoff()), cutoff(7000.0) },
                           { key(C::xmlKeyAmpAttack()), attack(0.6) },
                           { key(C::xmlKeyAmpDecay()), time(1.0) },
                           { key(C::xmlKeyAmpSustain()), 0.9f },
                           { key(C::xmlKeyAmpRelease()), time(1.2) },
                           { key(C::xmlKeyLfoWaveform()), SineWave },
                           { key(C::xmlKeyLfoTarget()), choice(LfoDest::Osc1Pos) },
                           { key(C::xmlKeyLfoRate()), lfoRate(0.15) },
                           { key(C::xmlKeyLfoRateDivider()), 2.0f },
                           { key(C::xmlKeyLfoIntensity()), depth(0.5) },
                           { key(C::xmlKeyLfo2Waveform()), SineWave },
                           { key(C::xmlKeyLfo2Target()), choice(LfoDest::Pitch) },
                           { key(C::xmlKeyLfo2Rate()), lfoRate(5.0) },
                           { key(C::xmlKeyLfo2Intensity()), depth(10.0 * Cent) },
                           { key(C::xmlKeyLfo2Delay()), lfoTime(0.5) },
                           { key(C::xmlKeyLfo2Fade()), lfoTime(1.0) },
                           { key(C::xmlKeyVoiceMode()), choice(Mode::Unison) },
                           { key(C::xmlKeyVoiceDepth()), 0.2f },
                           { key(C::xmlKeyPanSpread()), 0.8f },
                         } },

        // A short sweep of the position down from between saw and square, gone before the note is.
        { "Wavetable Pluck", {
                               { key(C::xmlKeyWavetableIndex()), ClassicMorph },
                               { key(C::xmlKeyOsc1Pos()), 0.4f },
                               { key(C::xmlKeyLpfCutoff()), cutoff(9000.0) },
                               { key(C::xmlKeyLpfResonance()), 0.15f },
                               { key(C::xmlKeyAmpAttack()), attack(0.001) },
                               { key(C::xmlKeyAmpDecay()), time(0.35) },
                               { key(C::xmlKeyAmpSustain()), 0.0f },
                               { key(C::xmlKeyAmpRelease()), time(0.25) },
                               { key(C::xmlKeyModTarget()), choice(Mod::Osc1Pos) },
                               { key(C::xmlKeyModAttack()), attack(0.001) },
                               { key(C::xmlKeyModDecay()), time(0.15) },
                               { key(C::xmlKeyModIntensity()), depth(0.45) },
                             } },
    };
    return presetList;
}

} // namespace noteahead
