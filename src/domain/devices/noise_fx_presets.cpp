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

#include "noise_fx_presets.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "noise_fx_device.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

namespace NahdXml = Constants::NahdXml;
using Destination = NoiseFxDevice::Destination;
using Parameters = std::map<std::string, float>;

// Written in the units the patches are thought in and converted here, on the curves the device runs
// its controls on.

float cutoffKnob(double hz)
{
    return static_cast<float>(ParameterMapper::unmapExponential(std::clamp(hz, 20.0, 20000.0), 20.0, 20000.0));
}

float attackKnob(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.001, 10.0));
}

//! Decay and release share their range.
float decayKnob(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.005, 20.0));
}

float sweepSecondsKnob(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.01, 60.0));
}

float lfoRateKnob(double hz)
{
    return static_cast<float>(ParameterMapper::unmapExponential(hz, 0.05, 50.0));
}

//! The noise level knob, which runs on a squared gain curve, for a level in dB below full.
float noiseLevelKnob(double decibels)
{
    return static_cast<float>(std::sqrt(std::pow(10.0, decibels / 20.0)));
}

float holdRateKnob(double hz)
{
    return static_cast<float>(ParameterMapper::unmapExponential(hz, 50.0, 50000.0));
}

//! A modulation depth, -1..1, as the bipolar knob stores it.
float depthKnob(double depth)
{
    return static_cast<float>(0.5 + std::clamp(depth, -1.0, 1.0) * 0.5);
}

//! A pitch offset, in semitones of the ±24 the knob covers.
float pitchKnob(double semitones)
{
    return static_cast<float>(0.5 + std::clamp(semitones / 24.0, -1.0, 1.0) * 0.5);
}

//! The entry of NoiseFxDevice::syncLengthBeats() that is @p bars long.
float syncLength(double bars)
{
    const auto & beats = NoiseFxDevice::syncLengthBeats();
    const auto found = std::ranges::find(beats, bars * 4.0);
    return static_cast<float>(std::distance(beats.begin(), found));
}

//! The entry of NoiseFxDevice::gateDivisionBeats() that is @p beats long.
float gateDivision(double beats)
{
    const auto & divisions = NoiseFxDevice::gateDivisionBeats();
    const auto found = std::ranges::find(divisions, beats);
    return static_cast<float>(std::distance(divisions.begin(), found));
}

float ordinal(auto value)
{
    return static_cast<float>(value);
}

std::string key(const QString & key)
{
    return key.toStdString();
}

std::string sweepA(const QString & key)
{
    return NoiseFxDevice::sweepKey(0, key);
}

std::string sweepB(const QString & key)
{
    return NoiseFxDevice::sweepKey(1, key);
}

//! Destination @p destination, 1 or 2, of the sweep @p sweep.
std::string sweepRoute(size_t sweep, const QString & key, size_t destination)
{
    return NoiseFxDevice::sweepDestinationKey(sweep, key, destination - 1);
}

std::string lfo(const QString & key)
{
    return NoiseFxDevice::lfoKey(key);
}

std::string lfoRoute(const QString & key, size_t destination)
{
    return NoiseFxDevice::lfoDestinationKey(key, destination - 1);
}

std::string gate(const QString & key)
{
    return NoiseFxDevice::gateKey(key);
}

constexpr float On = 1.0f;
constexpr float Off = 0.0f;

} // namespace

const std::vector<SynthPreset> & NoiseFxPresets::presets()
{
    static const std::vector<SynthPreset> presetList = [] {
        std::vector<SynthPreset> list;

        // The init patch is itself a riser: Sweep A opens the filter over four bars.
        list.push_back({ "Init", {} });

        // White noise opening up and fading in over eight bars, the build into a drop.
        list.push_back({ "Riser 8 Bars", Parameters {
                                           { key(NahdXml::xmlKeyCutoff()), cutoffKnob(150.0) },
                                           { key(NahdXml::xmlKeyResonance()), 0.55f },
                                           { key(NahdXml::xmlKeyAmpRelease()), decayKnob(1.5) },
                                           { sweepA(NahdXml::xmlKeySyncLength()), syncLength(8.0) },
                                           { sweepRoute(0, NahdXml::xmlKeyTarget(), 2), ordinal(Destination::Level) },
                                           { sweepRoute(0, NahdXml::xmlKeyDepth(), 2), depthKnob(0.9) },
                                         } });

        // A band of noise chopped on the beat, the chops speeding up to sixteenths as it rises.
        list.push_back({ "Snare Roll Build", Parameters {
                                               { key(NahdXml::xmlKeyColor()), 0.6f },
                                               { key(NahdXml::xmlKeyFilterType()), ordinal(NoiseFxDevice::FilterType::BandPass) },
                                               { key(NahdXml::xmlKeyCutoff()), cutoffKnob(500.0) },
                                               { key(NahdXml::xmlKeyResonance()), 0.4f },
                                               { key(NahdXml::xmlKeyAmpRelease()), decayKnob(0.6) },
                                               { sweepA(NahdXml::xmlKeyCurve()), 0.6f },
                                               { sweepA(NahdXml::xmlKeySyncLength()), syncLength(4.0) },
                                               { sweepRoute(0, NahdXml::xmlKeyDepth(), 1), depthKnob(0.6) },
                                               { sweepRoute(0, NahdXml::xmlKeyTarget(), 2), ordinal(Destination::GateRate) },
                                               { sweepRoute(0, NahdXml::xmlKeyDepth(), 2), depthKnob(0.5) },
                                               { gate(NahdXml::xmlKeyRate()), gateDivision(1.0) },
                                               { gate(NahdXml::xmlKeyDepth()), 1.0f },
                                               { gate(NahdXml::xmlKeyWidth()), 0.35f },
                                               { gate(NahdXml::xmlKeySmoothing()), 0.15f },
                                             } });

        // Bright noise closing down and dying away over two bars, for after the drop.
        list.push_back({ "Downlifter", Parameters {
                                         { key(NahdXml::xmlKeyCutoff()), cutoffKnob(60.0) },
                                         { key(NahdXml::xmlKeyResonance()), 0.45f },
                                         { sweepA(NahdXml::xmlKeyStart()), 1.0f },
                                         { sweepA(NahdXml::xmlKeyEnd()), 0.0f },
                                         { sweepA(NahdXml::xmlKeyCurve()), 0.3f },
                                         { sweepA(NahdXml::xmlKeySyncLength()), syncLength(2.0) },
                                         { sweepRoute(0, NahdXml::xmlKeyTarget(), 2), ordinal(Destination::Level) },
                                         { sweepRoute(0, NahdXml::xmlKeyDepth(), 2), depthKnob(0.8) },
                                       } });

        // A white noise hit that falls in pitch and colour, like a crash played backwards and forwards.
        list.push_back({ "Impact Sweep", Parameters {
                                           { key(NahdXml::xmlKeyCutoff()), cutoffKnob(80.0) },
                                           { key(NahdXml::xmlKeyResonance()), 0.2f },
                                           { key(NahdXml::xmlKeyDrive()), 0.4f },
                                           { key(NahdXml::xmlKeyAmpDecay()), decayKnob(2.5) },
                                           { key(NahdXml::xmlKeyAmpSustain()), 0.0f },
                                           { sweepA(NahdXml::xmlKeyStart()), 1.0f },
                                           { sweepA(NahdXml::xmlKeyEnd()), 0.0f },
                                           { sweepA(NahdXml::xmlKeyCurve()), 0.15f },
                                           { sweepA(NahdXml::xmlKeySync()), Off },
                                           { sweepA(NahdXml::xmlKeyLength()), sweepSecondsKnob(2.5) },
                                           { sweepRoute(0, NahdXml::xmlKeyTarget(), 2), ordinal(Destination::Colour) },
                                           { sweepRoute(0, NahdXml::xmlKeyDepth(), 2), depthKnob(0.3) },
                                         } });

        // Pink noise through a resonant band pass that drifts, gusting in level: held, it blows. The
        // LFO glides, as a stepped one jumps the colour of the wind every couple of seconds.
        list.push_back({ "Wind", Parameters {
                                   { key(NahdXml::xmlKeyColor()), 0.3f },
                                   { key(NahdXml::xmlKeyWidth()), 1.0f },
                                   { key(NahdXml::xmlKeyFilterType()), ordinal(NoiseFxDevice::FilterType::BandPass) },
                                   { key(NahdXml::xmlKeyCutoff()), cutoffKnob(700.0) },
                                   { key(NahdXml::xmlKeyResonance()), 0.75f },
                                   { key(NahdXml::xmlKeyAmpAttack()), attackKnob(1.5) },
                                   { key(NahdXml::xmlKeyAmpRelease()), decayKnob(2.5) },
                                   { key(NahdXml::xmlKeyPlayToEnd()), Off },
                                   { sweepRoute(0, NahdXml::xmlKeyTarget(), 1), ordinal(Destination::None) },
                                   { lfo(NahdXml::xmlKeyWaveform()), static_cast<float>(NoiseFxDevice::SmoothRandomWaveform) },
                                   { lfo(NahdXml::xmlKeyRate()), lfoRateKnob(0.4) },
                                   { lfoRoute(NahdXml::xmlKeyTarget(), 1), ordinal(Destination::Cutoff) },
                                   { lfoRoute(NahdXml::xmlKeyDepth(), 1), depthKnob(0.25) },
                                   { lfoRoute(NahdXml::xmlKeyTarget(), 2), ordinal(Destination::Level) },
                                   { lfoRoute(NahdXml::xmlKeyDepth(), 2), depthKnob(0.5) },
                                 } });

        // Pops and ticks over a faint hiss: a record between the tracks.
        list.push_back({ "Vinyl Crackle", Parameters {
                                            { key(NahdXml::xmlKeyNoiseLevel()), noiseLevelKnob(-30.0) },
                                            { key(NahdXml::xmlKeyColor()), 0.3f },
                                            { key(NahdXml::xmlKeyCrackle()), 0.45f },
                                            { key(NahdXml::xmlKeyWidth()), 0.3f },
                                            { key(NahdXml::xmlKeyCutoff()), cutoffKnob(3000.0) },
                                            { key(NahdXml::xmlKeyResonance()), 0.0f },
                                            { key(NahdXml::xmlKeyPlayToEnd()), Off },
                                            { sweepRoute(0, NahdXml::xmlKeyTarget(), 1), ordinal(Destination::None) },
                                          } });

        // Noise sampled at a falling rate as the filter opens: a riser that turns to grit.
        list.push_back({ "Bitcrush Riser", Parameters {
                                             { key(NahdXml::xmlKeyCutoff()), cutoffKnob(300.0) },
                                             { key(NahdXml::xmlKeyResonance()), 0.35f },
                                             { key(NahdXml::xmlKeySampleHoldRate()), holdRateKnob(16000.0) },
                                             { key(NahdXml::xmlKeyAmpRelease()), decayKnob(0.8) },
                                             { sweepRoute(0, NahdXml::xmlKeyDepth(), 1), depthKnob(0.75) },
                                             { sweepRoute(0, NahdXml::xmlKeyTarget(), 2), ordinal(Destination::SampleHoldRate) },
                                             { sweepRoute(0, NahdXml::xmlKeyDepth(), 2), depthKnob(-0.6) },
                                           } });

        // A saw dropping three octaves in a fifth of a second.
        list.push_back({ "Laser Zap", Parameters {
                                        { key(NahdXml::xmlKeyBlend()), 1.0f },
                                        { key(NahdXml::xmlKeyWaveform()), ordinal(PolyBlepOscillator::Waveform::Saw) },
                                        { key(NahdXml::xmlKeyPitch()), pitchKnob(24.0) },
                                        { key(NahdXml::xmlKeyCutoff()), cutoffKnob(8000.0) },
                                        { key(NahdXml::xmlKeyResonance()), 0.5f },
                                        { key(NahdXml::xmlKeyAmpDecay()), decayKnob(0.25) },
                                        { key(NahdXml::xmlKeyAmpSustain()), 0.0f },
                                        { key(NahdXml::xmlKeyAmpRelease()), decayKnob(0.05) },
                                        { sweepA(NahdXml::xmlKeyCurve()), 0.2f },
                                        { sweepA(NahdXml::xmlKeySync()), Off },
                                        { sweepA(NahdXml::xmlKeyLength()), sweepSecondsKnob(0.2) },
                                        { sweepRoute(0, NahdXml::xmlKeyTarget(), 1), ordinal(Destination::Pitch) },
                                        { sweepRoute(0, NahdXml::xmlKeyDepth(), 1), depthKnob(-0.75) },
                                      } });

        // A triangle wailing up and down a fifth and more, for as long as the note is held.
        list.push_back({ "Siren", Parameters {
                                    { key(NahdXml::xmlKeyBlend()), 1.0f },
                                    { key(NahdXml::xmlKeyWaveform()), ordinal(PolyBlepOscillator::Waveform::Triangle) },
                                    { key(NahdXml::xmlKeyPitch()), pitchKnob(12.0) },
                                    { key(NahdXml::xmlKeyCutoff()), 1.0f },
                                    { key(NahdXml::xmlKeyResonance()), 0.0f },
                                    { key(NahdXml::xmlKeyAmpAttack()), attackKnob(0.05) },
                                    { key(NahdXml::xmlKeyPlayToEnd()), Off },
                                    { sweepRoute(0, NahdXml::xmlKeyTarget(), 1), ordinal(Destination::None) },
                                    { lfo(NahdXml::xmlKeyWaveform()), ordinal(Lfo::Waveform::Triangle) },
                                    { lfo(NahdXml::xmlKeyRate()), lfoRateKnob(0.35) },
                                    { lfoRoute(NahdXml::xmlKeyTarget(), 1), ordinal(Destination::Pitch) },
                                    { lfoRoute(NahdXml::xmlKeyDepth(), 1), depthKnob(0.3) },
                                  } });

        // Noise and a ring modulated square through a filter swinging back and forth on the bar.
        list.push_back({ "Ring Mod Sweep", Parameters {
                                             { key(NahdXml::xmlKeyBlend()), 0.6f },
                                             { key(NahdXml::xmlKeyWaveform()), ordinal(PolyBlepOscillator::Waveform::Square) },
                                             { key(NahdXml::xmlKeyRingMod()), 1.0f },
                                             { key(NahdXml::xmlKeyCutoff()), cutoffKnob(200.0) },
                                             { key(NahdXml::xmlKeyResonance()), 0.7f },
                                             { key(NahdXml::xmlKeyDrive()), 0.3f },
                                             { key(NahdXml::xmlKeyPlayToEnd()), Off },
                                             { sweepA(NahdXml::xmlKeyMode()), ordinal(SweepEnvelope::Mode::PingPong) },
                                             { sweepA(NahdXml::xmlKeyCurve()), 0.5f },
                                             { sweepA(NahdXml::xmlKeySyncLength()), syncLength(1.0) },
                                             { sweepRoute(0, NahdXml::xmlKeyDepth(), 1), depthKnob(0.7) },
                                             { sweepB(NahdXml::xmlKeyMode()), ordinal(SweepEnvelope::Mode::PingPong) },
                                             { sweepB(NahdXml::xmlKeySyncLength()), syncLength(0.5) },
                                             { sweepRoute(1, NahdXml::xmlKeyTarget(), 1), ordinal(Destination::Pan) },
                                             { sweepRoute(1, NahdXml::xmlKeyDepth(), 1), depthKnob(0.6) },
                                           } });

        // Ocean surf: brown noise swelling and ebbing every two bars.
        list.push_back({ "Surf", Parameters {
                                   { key(NahdXml::xmlKeyColor()), 0.1f },
                                   { key(NahdXml::xmlKeyWidth()), 1.0f },
                                   { key(NahdXml::xmlKeyCutoff()), cutoffKnob(400.0) },
                                   { key(NahdXml::xmlKeyResonance()), 0.1f },
                                   { key(NahdXml::xmlKeyAmpAttack()), attackKnob(1.0) },
                                   { key(NahdXml::xmlKeyAmpRelease()), decayKnob(3.0) },
                                   { key(NahdXml::xmlKeyPlayToEnd()), Off },
                                   { sweepA(NahdXml::xmlKeyMode()), ordinal(SweepEnvelope::Mode::PingPong) },
                                   { sweepA(NahdXml::xmlKeyCurve()), 0.35f },
                                   { sweepA(NahdXml::xmlKeySyncLength()), syncLength(1.0) },
                                   { sweepRoute(0, NahdXml::xmlKeyDepth(), 1), depthKnob(0.6) },
                                   { sweepRoute(0, NahdXml::xmlKeyTarget(), 2), ordinal(Destination::Level) },
                                   { sweepRoute(0, NahdXml::xmlKeyDepth(), 2), depthKnob(0.85) },
                                 } });

        // Dark noise driven hard, its filter slamming shut over two seconds while a sine an octave
        // under the note falls with it: the blast and the thump under it. Play low.
        list.push_back({ "Explosion", Parameters {
                                        { key(NahdXml::xmlKeyColor()), 0.35f },
                                        { key(NahdXml::xmlKeyCrackle()), 0.15f },
                                        { key(NahdXml::xmlKeyWidth()), 0.8f },
                                        { key(NahdXml::xmlKeyWaveform()), ordinal(PolyBlepOscillator::Waveform::Sine) },
                                        { key(NahdXml::xmlKeyPitch()), pitchKnob(-12.0) },
                                        { key(NahdXml::xmlKeyBlend()), 0.15f },
                                        { key(NahdXml::xmlKeyCutoff()), cutoffKnob(80.0) },
                                        { key(NahdXml::xmlKeyResonance()), 0.15f },
                                        { key(NahdXml::xmlKeyDrive()), 0.6f },
                                        { key(NahdXml::xmlKeyAmpDecay()), decayKnob(3.0) },
                                        { key(NahdXml::xmlKeyAmpSustain()), 0.0f },
                                        { key(NahdXml::xmlKeyAmpRelease()), decayKnob(1.5) },
                                        { sweepA(NahdXml::xmlKeyStart()), 1.0f },
                                        { sweepA(NahdXml::xmlKeyEnd()), 0.0f },
                                        { sweepA(NahdXml::xmlKeyCurve()), 0.3f },
                                        { sweepA(NahdXml::xmlKeySync()), Off },
                                        { sweepA(NahdXml::xmlKeyLength()), sweepSecondsKnob(2.0) },
                                        { sweepRoute(0, NahdXml::xmlKeyDepth(), 1), depthKnob(0.8) },
                                        { sweepRoute(0, NahdXml::xmlKeyTarget(), 2), ordinal(Destination::Pitch) },
                                        { sweepRoute(0, NahdXml::xmlKeyDepth(), 2), depthKnob(0.25) },
                                      } });

        // The same blast heard from a few kilometres off: the top gone, a long low roll left.
        list.push_back({ "Distant Explosion", Parameters {
                                                { key(NahdXml::xmlKeyColor()), 0.05f },
                                                { key(NahdXml::xmlKeyWidth()), 1.0f },
                                                { key(NahdXml::xmlKeyCutoff()), cutoffKnob(100.0) },
                                                { key(NahdXml::xmlKeyResonance()), 0.1f },
                                                { key(NahdXml::xmlKeyDrive()), 0.3f },
                                                { key(NahdXml::xmlKeyAmpAttack()), attackKnob(0.03) },
                                                { key(NahdXml::xmlKeyAmpDecay()), decayKnob(5.0) },
                                                { key(NahdXml::xmlKeyAmpSustain()), 0.0f },
                                                { key(NahdXml::xmlKeyAmpRelease()), decayKnob(3.0) },
                                                { sweepA(NahdXml::xmlKeyStart()), 1.0f },
                                                { sweepA(NahdXml::xmlKeyEnd()), 0.0f },
                                                { sweepA(NahdXml::xmlKeyCurve()), 0.3f },
                                                { sweepA(NahdXml::xmlKeySync()), Off },
                                                { sweepA(NahdXml::xmlKeyLength()), sweepSecondsKnob(4.0) },
                                                { sweepRoute(0, NahdXml::xmlKeyDepth(), 1), depthKnob(0.4) },
                                                { lfo(NahdXml::xmlKeyWaveform()), static_cast<float>(NoiseFxDevice::SmoothRandomWaveform) },
                                                { lfo(NahdXml::xmlKeyRate()), lfoRateKnob(3.0) },
                                                { lfoRoute(NahdXml::xmlKeyTarget(), 1), ordinal(Destination::Level) },
                                                { lfoRoute(NahdXml::xmlKeyDepth(), 1), depthKnob(0.4) },
                                              } });

        // The sharp bang of late-nineties trance, fitted to the one ending the break of Feel the
        // Beat: a sub boom falling from 260 Hz to an octave under the note in a tenth of a second,
        // under flat white noise, with the mids between scooped out by the notch. Nothing closes
        // down: the noise stays bright until it is gone, a second later, which is what keeps this
        // apart from an explosion.
        list.push_back({ "Trance Bang", Parameters {
                                          { key(NahdXml::xmlKeyColor()), 0.5f },
                                          { key(NahdXml::xmlKeyWidth()), 1.0f },
                                          { key(NahdXml::xmlKeyWaveform()), ordinal(PolyBlepOscillator::Waveform::Sine) },
                                          { key(NahdXml::xmlKeyPitch()), pitchKnob(-12.0) },
                                          { key(NahdXml::xmlKeyBlend()), 0.75f },
                                          { key(NahdXml::xmlKeyFilterType()), ordinal(NoiseFxDevice::FilterType::Notch) },
                                          { key(NahdXml::xmlKeyCutoff()), cutoffKnob(800.0) },
                                          { key(NahdXml::xmlKeyResonance()), 0.0f },
                                          { key(NahdXml::xmlKeyDrive()), 0.1f },
                                          { key(NahdXml::xmlKeyAmpDecay()), decayKnob(1.0) },
                                          { key(NahdXml::xmlKeyAmpSustain()), 0.0f },
                                          { key(NahdXml::xmlKeyAmpRelease()), decayKnob(1.0) },
                                          { sweepRoute(0, NahdXml::xmlKeyTarget(), 1), ordinal(Destination::None) },
                                          { sweepB(NahdXml::xmlKeyStart()), 1.0f },
                                          { sweepB(NahdXml::xmlKeyEnd()), 0.0f },
                                          { sweepB(NahdXml::xmlKeyCurve()), 0.2f },
                                          { sweepB(NahdXml::xmlKeySync()), Off },
                                          { sweepB(NahdXml::xmlKeyLength()), sweepSecondsKnob(0.15) },
                                          { sweepRoute(1, NahdXml::xmlKeyTarget(), 1), ordinal(Destination::Pitch) },
                                          { sweepRoute(1, NahdXml::xmlKeyDepth(), 1), depthKnob(0.5) },
                                        } });

        // The same bang cut short, gone in a third of a second, for hits on the beat.
        list.push_back({ "Short Bang", Parameters {
                                         { key(NahdXml::xmlKeyColor()), 0.5f },
                                         { key(NahdXml::xmlKeyWidth()), 1.0f },
                                         { key(NahdXml::xmlKeyWaveform()), ordinal(PolyBlepOscillator::Waveform::Sine) },
                                         { key(NahdXml::xmlKeyPitch()), pitchKnob(-12.0) },
                                         { key(NahdXml::xmlKeyBlend()), 0.75f },
                                         { key(NahdXml::xmlKeyFilterType()), ordinal(NoiseFxDevice::FilterType::Notch) },
                                         { key(NahdXml::xmlKeyCutoff()), cutoffKnob(800.0) },
                                         { key(NahdXml::xmlKeyResonance()), 0.0f },
                                         { key(NahdXml::xmlKeyDrive()), 0.1f },
                                         { key(NahdXml::xmlKeyAmpDecay()), decayKnob(0.3) },
                                         { key(NahdXml::xmlKeyAmpSustain()), 0.0f },
                                         { key(NahdXml::xmlKeyAmpRelease()), decayKnob(0.3) },
                                         { sweepRoute(0, NahdXml::xmlKeyTarget(), 1), ordinal(Destination::None) },
                                         { sweepB(NahdXml::xmlKeyStart()), 1.0f },
                                         { sweepB(NahdXml::xmlKeyEnd()), 0.0f },
                                         { sweepB(NahdXml::xmlKeyCurve()), 0.2f },
                                         { sweepB(NahdXml::xmlKeySync()), Off },
                                         { sweepB(NahdXml::xmlKeyLength()), sweepSecondsKnob(0.06) },
                                         { sweepRoute(1, NahdXml::xmlKeyTarget(), 1), ordinal(Destination::Pitch) },
                                         { sweepRoute(1, NahdXml::xmlKeyDepth(), 1), depthKnob(0.5) },
                                       } });

        // A short, bright blast full of crackling debris that turns to grit as it dies away.
        list.push_back({ "Debris Blast", Parameters {
                                           { key(NahdXml::xmlKeyColor()), 0.45f },
                                           { key(NahdXml::xmlKeyCrackle()), 0.6f },
                                           { key(NahdXml::xmlKeyWidth()), 1.0f },
                                           { key(NahdXml::xmlKeyCutoff()), cutoffKnob(200.0) },
                                           { key(NahdXml::xmlKeyResonance()), 0.25f },
                                           { key(NahdXml::xmlKeyDrive()), 0.5f },
                                           { key(NahdXml::xmlKeyAmpDecay()), decayKnob(1.2) },
                                           { key(NahdXml::xmlKeyAmpSustain()), 0.0f },
                                           { key(NahdXml::xmlKeyAmpRelease()), decayKnob(0.8) },
                                           { sweepA(NahdXml::xmlKeyStart()), 1.0f },
                                           { sweepA(NahdXml::xmlKeyEnd()), 0.0f },
                                           { sweepA(NahdXml::xmlKeyCurve()), 0.2f },
                                           { sweepA(NahdXml::xmlKeySync()), Off },
                                           { sweepA(NahdXml::xmlKeyLength()), sweepSecondsKnob(1.2) },
                                           { sweepRoute(0, NahdXml::xmlKeyDepth(), 1), depthKnob(0.6) },
                                           { sweepRoute(0, NahdXml::xmlKeyTarget(), 2), ordinal(Destination::SampleHoldRate) },
                                           { sweepRoute(0, NahdXml::xmlKeyDepth(), 2), depthKnob(0.5) },
                                           { key(NahdXml::xmlKeySampleHoldRate()), holdRateKnob(3000.0) },
                                         } });

        // A fire: a roar of dark noise flickering in level and brightness, with wood popping in it.
        // High passed, so the roar stays off the subs and the pops come through bright. Held, it
        // burns.
        list.push_back({ "Fire", Parameters {
                                   { key(NahdXml::xmlKeyColor()), 0.15f },
                                   { key(NahdXml::xmlKeyCrackle()), 0.4f },
                                   { key(NahdXml::xmlKeyWidth()), 0.7f },
                                   { key(NahdXml::xmlKeyFilterType()), ordinal(NoiseFxDevice::FilterType::HighPass) },
                                   { key(NahdXml::xmlKeyCutoff()), cutoffKnob(150.0) },
                                   { key(NahdXml::xmlKeyResonance()), 0.05f },
                                   { key(NahdXml::xmlKeyDrive()), 0.2f },
                                   { key(NahdXml::xmlKeyAmpAttack()), attackKnob(0.8) },
                                   { key(NahdXml::xmlKeyAmpRelease()), decayKnob(1.5) },
                                   { key(NahdXml::xmlKeyPlayToEnd()), Off },
                                   { sweepRoute(0, NahdXml::xmlKeyTarget(), 1), ordinal(Destination::None) },
                                   { lfo(NahdXml::xmlKeyWaveform()), static_cast<float>(NoiseFxDevice::SmoothRandomWaveform) },
                                   { lfo(NahdXml::xmlKeyRate()), lfoRateKnob(1.5) },
                                   { lfoRoute(NahdXml::xmlKeyTarget(), 1), ordinal(Destination::Level) },
                                   { lfoRoute(NahdXml::xmlKeyDepth(), 1), depthKnob(0.35) },
                                   { lfoRoute(NahdXml::xmlKeyTarget(), 2), ordinal(Destination::Cutoff) },
                                   { lfoRoute(NahdXml::xmlKeyDepth(), 2), depthKnob(0.1) },
                                 } });

        return list;
    }();
    return presetList;
}

} // namespace noteahead
