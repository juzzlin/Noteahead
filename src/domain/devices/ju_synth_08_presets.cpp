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

#include "ju_synth_08_presets.hpp"
#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

namespace C = Constants::NahdXml;

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
    return static_cast<float>(ParameterMapper::unmapExponential(hz, 0.1, 30.0));
}

//! LFO delay, as a share of its three seconds.
float lfoDelayKnob(double seconds)
{
    return static_cast<float>(std::clamp(seconds / 3.0, 0.0, 1.0));
}

//! VCO 2 fine tune over its 50 cents either way, centred at half travel.
float fineTuneKnob(double cents)
{
    return static_cast<float>(std::clamp(0.5 + cents / 100.0, 0.0, 1.0));
}

//! Unison detune, as a share of its 50 cents.
float unisonDetuneKnob(double cents)
{
    return static_cast<float>(std::clamp(cents / 50.0, 0.0, 1.0));
}

//! Pitch envelope depth over its 24 semitones either way, centred at half travel.
float pitchEnvKnob(double semitones)
{
    return static_cast<float>(std::clamp(0.5 + semitones / 48.0, 0.0, 1.0));
}

//! Delay time over its 10 ms to 1 s.
float delayTimeKnob(double seconds)
{
    return static_cast<float>(std::clamp((seconds - 0.01) / 0.99, 0.0, 1.0));
}

// Ranges, waveforms and modes, by the ordinals the device persists.
constexpr float Feet16 = 0.0f;
constexpr float Feet4 = 2.0f;
constexpr float Vco1Square = 2.0f;
constexpr float Vco1Sine = 3.0f;
constexpr float Vco2Square = 1.0f;
constexpr float Vco2Triangle = 2.0f;
constexpr float Slope12dB = 0.0f;
constexpr float Solo = 1.0f;
constexpr float Unison = 2.0f;
constexpr float PwmLfo = 1.0f;
constexpr float TargetVco2 = 1.0f;
constexpr float VcaGate = 1.0f;
constexpr float LfoTriangle = 1.0f;
constexpr float On = 1.0f;

std::string key(const QString & xmlKey)
{
    return xmlKey.toStdString();
}

} // namespace

const std::vector<JuSynth08Presets::Preset> & JuSynth08Presets::presets()
{
    static const std::vector<Preset> Presets {
        {
          .name = "Init",
          .parameters = {},
        },
        // Two saws ten cents apart on the 12 dB slope, swelling in over most of a second under a
        // delayed vibrato. Fitted to the factory MID STRINGS: its swell, its 4.6 Hz vibrato and its
        // balance of harmonics above 1 and 3 kHz.
        {
          .name = "Jupiter Strings",
          .parameters = {
            { key(C::xmlKeyVco1Level()), 0.7f },
            { key(C::xmlKeyVco2FineTune()), fineTuneKnob(10.0) },
            { key(C::xmlKeyVco2Level()), 0.7f },
            { key(C::xmlKeyVcfSlope()), Slope12dB },
            { key(C::xmlKeyLpfCutoff()), cutoffKnob(800.0) },
            { key(C::xmlKeyLpfResonance()), 0.1f },
            { key(C::xmlKeyEnvMod()), 0.08f },
            { key(C::xmlKeyKeyTrack()), 0.5f },
            { key(C::xmlKeyEnv1Attack()), attackKnob(1.1) },
            { key(C::xmlKeyEnv1Decay()), decayKnob(1.5) },
            { key(C::xmlKeyEnv1Sustain()), 0.8f },
            { key(C::xmlKeyEnv1Release()), releaseKnob(1.0) },
            { key(C::xmlKeyEnv2Attack()), attackKnob(1.1) },
            { key(C::xmlKeyEnv2Decay()), decayKnob(1.0) },
            { key(C::xmlKeyEnv2Sustain()), 0.9f },
            { key(C::xmlKeyEnv2Release()), releaseKnob(1.0) },
            { key(C::xmlKeyWaveform()), LfoTriangle },
            { key(C::xmlKeyLfoRate()), lfoRateKnob(4.6) },
            { key(C::xmlKeyLfoDelay()), lfoDelayKnob(0.8) },
            { key(C::xmlKeyPitchLfoDepth()), 0.25f },
            { key(C::xmlKeyPanSpread()), 0.7f },
            { key(C::xmlKeyDelayLevel()), 0.15f },
            { key(C::xmlKeyDelayTime()), delayTimeKnob(0.38) },
            { key(C::xmlKeyDelayFeedback()), 0.3f },
          },
        },
        // ENV-1 flares the filter open over the first 60 ms of every note and closes it again by
        // 150 ms, as on the factory LO BRASS, whose stabs this is fitted to.
        {
          .name = "Jupiter Brass",
          .parameters = {
            { key(C::xmlKeyVco1Level()), 0.8f },
            { key(C::xmlKeyVco2FineTune()), fineTuneKnob(5.0) },
            { key(C::xmlKeyVco2Level()), 0.8f },
            { key(C::xmlKeyLpfCutoff()), cutoffKnob(450.0) },
            { key(C::xmlKeyLpfResonance()), 0.15f },
            { key(C::xmlKeyEnvMod()), 0.4f },
            { key(C::xmlKeyKeyTrack()), 0.6f },
            { key(C::xmlKeyEnv1Attack()), attackKnob(0.09) },
            { key(C::xmlKeyEnv1Decay()), decayKnob(0.35) },
            { key(C::xmlKeyEnv1Sustain()), 0.45f },
            { key(C::xmlKeyEnv1Release()), releaseKnob(0.8) },
            { key(C::xmlKeyEnv2Attack()), attackKnob(0.03) },
            { key(C::xmlKeyEnv2Decay()), decayKnob(0.5) },
            { key(C::xmlKeyEnv2Sustain()), 0.85f },
            { key(C::xmlKeyEnv2Release()), releaseKnob(0.8) },
            { key(C::xmlKeyPanSpread()), 0.5f },
          },
        },
        // VCO 2 synced to VCO 1 and swept down an octave and more by ENV-1: the classic tearing lead.
        {
          .name = "Sync Lead",
          .parameters = {
            { key(C::xmlKeyVoiceMode()), Solo },
            { key(C::xmlKeyPortamento()), 0.04f },
            { key(C::xmlKeyVco1Level()), 0.6f },
            { key(C::xmlKeyVco2Sync()), On },
            { key(C::xmlKeyVco2Level()), 1.0f },
            { key(C::xmlKeyPitchEnvDepth()), pitchEnvKnob(14.0) },
            { key(C::xmlKeyPitchEnvTarget()), TargetVco2 },
            { key(C::xmlKeyLpfCutoff()), cutoffKnob(5000.0) },
            { key(C::xmlKeyLpfResonance()), 0.2f },
            { key(C::xmlKeyEnvMod()), 0.1f },
            { key(C::xmlKeyKeyTrack()), 0.5f },
            { key(C::xmlKeyEnv1Attack()), attackKnob(0.001) },
            { key(C::xmlKeyEnv1Decay()), decayKnob(0.6) },
            { key(C::xmlKeyEnv1Sustain()), 0.25f },
            { key(C::xmlKeyEnv1Release()), releaseKnob(0.3) },
            { key(C::xmlKeyEnv2Attack()), attackKnob(0.005) },
            { key(C::xmlKeyEnv2Decay()), decayKnob(0.5) },
            { key(C::xmlKeyEnv2Sustain()), 0.9f },
            { key(C::xmlKeyEnv2Release()), releaseKnob(0.25) },
            { key(C::xmlKeyLfoRate()), lfoRateKnob(5.5) },
            { key(C::xmlKeyLfoDelay()), lfoDelayKnob(0.5) },
            { key(C::xmlKeyPitchLfoDepth()), 0.25f },
            { key(C::xmlKeyDelayLevel()), 0.25f },
            { key(C::xmlKeyDelayTime()), delayTimeKnob(0.32) },
            { key(C::xmlKeyDelayFeedback()), 0.35f },
          },
        },
        // VCO 1 heard only through the cross mod, an octave and an inharmonic few cents under VCO 2.
        {
          .name = "Cross Mod Bell",
          .parameters = {
            { key(C::xmlKeyVco1Waveform()), Vco1Sine },
            { key(C::xmlKeyVco1Level()), 0.0f },
            { key(C::xmlKeyVco2Range()), Feet4 },
            { key(C::xmlKeyVco2Waveform()), Vco2Triangle },
            { key(C::xmlKeyVco2FineTune()), fineTuneKnob(41.0) },
            { key(C::xmlKeyVco2Level()), 1.0f },
            { key(C::xmlKeyCrossMod()), 0.7f },
            { key(C::xmlKeyVcfSlope()), Slope12dB },
            { key(C::xmlKeyLpfCutoff()), cutoffKnob(7000.0) },
            { key(C::xmlKeyKeyTrack()), 0.5f },
            { key(C::xmlKeyEnv2Attack()), attackKnob(0.001) },
            { key(C::xmlKeyEnv2Decay()), decayKnob(2.5) },
            { key(C::xmlKeyEnv2Sustain()), 0.0f },
            { key(C::xmlKeyEnv2Release()), releaseKnob(2.0) },
            { key(C::xmlKeyEnv2KeyFollow()), On },
            { key(C::xmlKeyPanSpread()), 0.6f },
            { key(C::xmlKeyDelayLevel()), 0.25f },
            { key(C::xmlKeyDelayTime()), delayTimeKnob(0.42) },
            { key(C::xmlKeyDelayFeedback()), 0.4f },
          },
        },
        // All eight voices on one note, a saw and a square an octave down, spread across the field.
        {
          .name = "Unison Bass",
          .parameters = {
            { key(C::xmlKeyVoiceMode()), Unison },
            { key(C::xmlKeyUnisonDetune()), unisonDetuneKnob(10.0) },
            { key(C::xmlKeyVco1Range()), Feet16 },
            { key(C::xmlKeyVco2Range()), Feet16 },
            { key(C::xmlKeyVco2Waveform()), Vco2Square },
            { key(C::xmlKeyVco2Level()), 0.6f },
            { key(C::xmlKeyLpfCutoff()), cutoffKnob(220.0) },
            { key(C::xmlKeyLpfResonance()), 0.3f },
            { key(C::xmlKeyEnvMod()), 0.32f },
            { key(C::xmlKeyKeyTrack()), 0.5f },
            { key(C::xmlKeyEnv1Attack()), attackKnob(0.001) },
            { key(C::xmlKeyEnv1Decay()), decayKnob(0.28) },
            { key(C::xmlKeyEnv1Sustain()), 0.15f },
            { key(C::xmlKeyEnv1Release()), releaseKnob(0.12) },
            { key(C::xmlKeyEnv2Attack()), attackKnob(0.001) },
            { key(C::xmlKeyEnv2Decay()), decayKnob(0.6) },
            { key(C::xmlKeyEnv2Sustain()), 0.85f },
            { key(C::xmlKeyEnv2Release()), releaseKnob(0.1) },
            { key(C::xmlKeyPanSpread()), 0.8f },
          },
        },
        // A narrow pulse and a saw in one voice, plucked by a filter that snaps shut.
        {
          .name = "Solo Bass",
          .parameters = {
            { key(C::xmlKeyVoiceMode()), Solo },
            { key(C::xmlKeyVco1Range()), Feet16 },
            { key(C::xmlKeyVco1Waveform()), Vco1Square },
            { key(C::xmlKeyPulseWidth()), 0.3f },
            { key(C::xmlKeyVco2Range()), Feet16 },
            { key(C::xmlKeyVco1Level()), 1.0f },
            { key(C::xmlKeyVco2Level()), 0.8f },
            { key(C::xmlKeyLpfCutoff()), cutoffKnob(200.0) },
            { key(C::xmlKeyLpfResonance()), 0.5f },
            { key(C::xmlKeyEnvMod()), 0.4f },
            { key(C::xmlKeyKeyTrack()), 0.5f },
            { key(C::xmlKeyEnv1Attack()), attackKnob(0.001) },
            { key(C::xmlKeyEnv1Decay()), decayKnob(0.2) },
            { key(C::xmlKeyEnv1Sustain()), 0.0f },
            { key(C::xmlKeyEnv1Release()), releaseKnob(0.1) },
            { key(C::xmlKeyEnv2Attack()), attackKnob(0.001) },
            { key(C::xmlKeyEnv2Decay()), decayKnob(0.4) },
            { key(C::xmlKeyEnv2Sustain()), 0.6f },
            { key(C::xmlKeyEnv2Release()), releaseKnob(0.05) },
            { key(C::xmlKeyVelocitySensitivity()), 0.4f },
          },
        },
        // Short and bright enough to sit on an arpeggio, with a dotted echo behind it.
        {
          .name = "Poly Pluck",
          .parameters = {
            { key(C::xmlKeyVco1Waveform()), Vco1Square },
            { key(C::xmlKeyPulseWidth()), 0.35f },
            { key(C::xmlKeyVco2FineTune()), fineTuneKnob(4.0) },
            { key(C::xmlKeyLpfCutoff()), cutoffKnob(400.0) },
            { key(C::xmlKeyLpfResonance()), 0.35f },
            { key(C::xmlKeyEnvMod()), 0.45f },
            { key(C::xmlKeyKeyTrack()), 0.7f },
            { key(C::xmlKeyEnv1Attack()), attackKnob(0.001) },
            { key(C::xmlKeyEnv1Decay()), decayKnob(0.35) },
            { key(C::xmlKeyEnv1Sustain()), 0.0f },
            { key(C::xmlKeyEnv1Release()), releaseKnob(0.25) },
            { key(C::xmlKeyEnv2Attack()), attackKnob(0.001) },
            { key(C::xmlKeyEnv2Decay()), decayKnob(0.5) },
            { key(C::xmlKeyEnv2Sustain()), 0.0f },
            { key(C::xmlKeyEnv2Release()), releaseKnob(0.3) },
            { key(C::xmlKeyVelocitySensitivity()), 0.5f },
            { key(C::xmlKeyPanSpread()), 0.6f },
            { key(C::xmlKeyDelayLevel()), 0.3f },
            { key(C::xmlKeyDelayTime()), delayTimeKnob(0.375) },
            { key(C::xmlKeyDelayFeedback()), 0.4f },
          },
        },
        // A square and a triangle an octave up on the gate, with a slow tremolo.
        {
          .name = "Hollow Organ",
          .parameters = {
            { key(C::xmlKeyVco1Waveform()), Vco1Square },
            { key(C::xmlKeyVco1Level()), 0.6f },
            { key(C::xmlKeyVco2Range()), Feet4 },
            { key(C::xmlKeyVco2Waveform()), Vco2Triangle },
            { key(C::xmlKeyVco2Level()), 0.7f },
            { key(C::xmlKeyVcfSlope()), Slope12dB },
            { key(C::xmlKeyLpfCutoff()), cutoffKnob(3500.0) },
            { key(C::xmlKeyVcaMode()), VcaGate },
            { key(C::xmlKeyWaveform()), LfoTriangle },
            { key(C::xmlKeyLfoRate()), lfoRateKnob(6.0) },
            { key(C::xmlKeyVcaLfoDepth()), 0.15f },
            { key(C::xmlKeyPanSpread()), 0.4f },
          },
        },
        // An LFO-swept pulse and a saw, the slow LFO moving a singing filter as well.
        {
          .name = "Reso Sweep Pad",
          .parameters = {
            { key(C::xmlKeyVco1Waveform()), Vco1Square },
            { key(C::xmlKeyVco1Level()), 0.7f },
            { key(C::xmlKeyPwmSource()), PwmLfo },
            { key(C::xmlKeyVco2FineTune()), fineTuneKnob(6.0) },
            { key(C::xmlKeyVco2Level()), 0.7f },
            { key(C::xmlKeyLpfCutoff()), cutoffKnob(500.0) },
            { key(C::xmlKeyLpfResonance()), 0.65f },
            { key(C::xmlKeyFilterLfoDepth()), 0.35f },
            { key(C::xmlKeyLfoRate()), lfoRateKnob(0.15) },
            { key(C::xmlKeyEnvMod()), 0.1f },
            { key(C::xmlKeyKeyTrack()), 0.5f },
            { key(C::xmlKeyEnv1Attack()), attackKnob(1.5) },
            { key(C::xmlKeyEnv1Decay()), decayKnob(3.0) },
            { key(C::xmlKeyEnv1Sustain()), 0.7f },
            { key(C::xmlKeyEnv1Release()), releaseKnob(2.0) },
            { key(C::xmlKeyEnv2Attack()), attackKnob(1.0) },
            { key(C::xmlKeyEnv2Decay()), decayKnob(2.0) },
            { key(C::xmlKeyEnv2Sustain()), 0.9f },
            { key(C::xmlKeyEnv2Release()), releaseKnob(2.0) },
            { key(C::xmlKeyPanSpread()), 0.8f },
            { key(C::xmlKeyDelayLevel()), 0.25f },
            { key(C::xmlKeyDelayTime()), delayTimeKnob(0.45) },
            { key(C::xmlKeyDelayFeedback()), 0.4f },
          },
        },
        // Every voice on the lead, a square an octave over the saw, with a vibrato that waits.
        {
          .name = "Unison Lead",
          .parameters = {
            { key(C::xmlKeyVoiceMode()), Unison },
            { key(C::xmlKeyUnisonDetune()), unisonDetuneKnob(12.0) },
            { key(C::xmlKeyPortamento()), 0.05f },
            { key(C::xmlKeyVco2Range()), Feet4 },
            { key(C::xmlKeyVco2Waveform()), Vco2Square },
            { key(C::xmlKeyVco2Level()), 0.5f },
            { key(C::xmlKeyLpfCutoff()), cutoffKnob(2500.0) },
            { key(C::xmlKeyLpfResonance()), 0.25f },
            { key(C::xmlKeyEnvMod()), 0.15f },
            { key(C::xmlKeyKeyTrack()), 0.5f },
            { key(C::xmlKeyEnv1Decay()), decayKnob(0.8) },
            { key(C::xmlKeyEnv1Sustain()), 0.4f },
            { key(C::xmlKeyEnv2Attack()), attackKnob(0.01) },
            { key(C::xmlKeyEnv2Decay()), decayKnob(0.5) },
            { key(C::xmlKeyEnv2Sustain()), 0.9f },
            { key(C::xmlKeyEnv2Release()), releaseKnob(0.35) },
            { key(C::xmlKeyLfoRate()), lfoRateKnob(5.0) },
            { key(C::xmlKeyLfoDelay()), lfoDelayKnob(0.6) },
            { key(C::xmlKeyPitchLfoDepth()), 0.3f },
            { key(C::xmlKeyPanSpread()), 0.8f },
            { key(C::xmlKeyDelayLevel()), 0.3f },
            { key(C::xmlKeyDelayTime()), delayTimeKnob(0.32) },
            { key(C::xmlKeyDelayFeedback()), 0.35f },
          },
        },
    };
    return Presets;
}

} // namespace noteahead
