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

#include "fm_synth_presets.hpp"

#include "../../common/parameter_mapper.hpp"
#include "fm_synth_device.hpp"

#include <array>
#include <random>
#include <string>

namespace noteahead {

namespace {

//! Knob position that lands on @p seconds, for the ranges the device maps its envelopes over.
//! Writing the patches in seconds keeps them readable and keeps the arithmetic out of the table.
float attackKnob(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.001, 10.0));
}

float timeKnob(double seconds)
{
    return static_cast<float>(ParameterMapper::unmapExponential(seconds, 0.01, 10.0));
}

//! Parameter keys, built once per operator so the tables below read as patches rather than as
//! string arithmetic. Operators are numbered from one here, the way the dialog shows them.
std::string opKey(int op, const char * suffix)
{
    return "op" + std::to_string(op) + suffix;
}

} // namespace

const std::vector<SynthPreset> & FmSynthPresets::presets()
{
    static const std::vector<SynthPreset> presetList = [] {
        std::vector<SynthPreset> list;

        list.push_back({ "Init", {} });

        // The one everybody knows: two stacks, one making the body and one the tine. The tine is a
        // very high ratio with a very short decay, which is the whole trick -- it is gone before
        // the note has settled, leaving the ping without the pitch it would otherwise imply.
        list.push_back({ "E.Piano", {
                                      { "algorithm", 4.0f },
                                      { opKey(2, "Ratio"), 1.0f },
                                      { opKey(2, "Level"), 0.55f },
                                      { opKey(2, "Decay"), timeKnob(0.5) },
                                      { opKey(2, "Sustain"), 0.05f },
                                      { opKey(2, "VelocitySensitivity"), 0.75f },
                                      { opKey(3, "Level"), 0.45f },
                                      { opKey(4, "Ratio"), 14.0f },
                                      { opKey(4, "Level"), 0.32f },
                                      { opKey(4, "Decay"), timeKnob(0.12) },
                                      { opKey(4, "Sustain"), 0.0f },
                                      { opKey(4, "VelocitySensitivity"), 1.0f },
                                      { "ampDecay", timeKnob(2.0) },
                                      { "ampSustain", 0.25f },
                                      { "ampRelease", timeKnob(0.3) },
                                      { "ampCurve", 0.55f },
                                    } });

        list.push_back({ "E.Piano Soft", {
                                           { "algorithm", 4.0f },
                                           { opKey(2, "Level"), 0.4f },
                                           { opKey(2, "Decay"), timeKnob(0.7) },
                                           { opKey(2, "Sustain"), 0.05f },
                                           { opKey(2, "VelocitySensitivity"), 0.6f },
                                           { opKey(3, "Level"), 0.5f },
                                           { opKey(4, "Ratio"), 14.0f },
                                           { opKey(4, "Level"), 0.18f },
                                           { opKey(4, "Decay"), timeKnob(0.1) },
                                           { opKey(4, "Sustain"), 0.0f },
                                           { opKey(4, "VelocitySensitivity"), 0.9f },
                                           { "ampDecay", timeKnob(3.5) },
                                           { "ampSustain", 0.3f },
                                           { "ampRelease", timeKnob(0.6) },
                                           { "ampCurve", 0.5f },
                                         } });

        list.push_back({ "FM Bass", {
                                      { opKey(2, "Level"), 0.6f },
                                      { opKey(2, "Decay"), timeKnob(0.12) },
                                      { opKey(2, "Sustain"), 0.15f },
                                      { opKey(2, "VelocitySensitivity"), 0.7f },
                                      { "lpfCutoff", 0.75f },
                                      { "ampDecay", timeKnob(0.45) },
                                      { "ampSustain", 0.35f },
                                      { "ampRelease", timeKnob(0.12) },
                                      { "ampCurve", 0.4f },
                                    } });

        // The one place the half ratio earns its keep: the carrier plays an octave below the note.
        list.push_back({ "Sub Bass", {
                                       { opKey(1, "Ratio"), 0.0f },
                                       { opKey(2, "Level"), 0.22f },
                                       { opKey(2, "Decay"), timeKnob(0.06) },
                                       { opKey(2, "Sustain"), 0.0f },
                                       { "lpfCutoff", 0.5f },
                                       { "ampDecay", timeKnob(0.8) },
                                       { "ampSustain", 0.5f },
                                       { "ampRelease", timeKnob(0.15) },
                                     } });

        list.push_back({ "Slap Bass", {
                                        { opKey(2, "Ratio"), 3.0f },
                                        { opKey(2, "Level"), 0.7f },
                                        { opKey(2, "Decay"), timeKnob(0.03) },
                                        { opKey(2, "Sustain"), 0.0f },
                                        { opKey(2, "VelocitySensitivity"), 1.0f },
                                        { "ampDecay", timeKnob(0.35) },
                                        { "ampSustain", 0.15f },
                                        { "ampRelease", timeKnob(0.1) },
                                        { "ampCurve", 0.6f },
                                      } });

        // A whole-number ratio grid can only make harmonic sidebands, and a bell is not harmonic.
        // The detune is what pulls the modulator off the series and puts the clang in.
        list.push_back({ "Tubular Bell", {
                                           { opKey(2, "Ratio"), 7.0f },
                                           { opKey(2, "Detune"), 0.88f },
                                           { opKey(2, "Level"), 0.5f },
                                           { opKey(2, "Decay"), timeKnob(0.7) },
                                           { opKey(2, "Sustain"), 0.0f },
                                           { "ampDecay", timeKnob(3.5) },
                                           { "ampSustain", 0.0f },
                                           { "ampRelease", timeKnob(1.3) },
                                           { "ampCurve", 0.6f },
                                         } });

        list.push_back({ "Glass Bells", {
                                          { "algorithm", 4.0f },
                                          { opKey(2, "Ratio"), 5.0f },
                                          { opKey(2, "Level"), 0.35f },
                                          { opKey(2, "Decay"), timeKnob(0.8) },
                                          { opKey(2, "Sustain"), 0.0f },
                                          { opKey(3, "Ratio"), 2.0f },
                                          { opKey(3, "Level"), 0.5f },
                                          { opKey(4, "Ratio"), 9.0f },
                                          { opKey(4, "Detune"), 0.85f },
                                          { opKey(4, "Level"), 0.3f },
                                          { opKey(4, "Decay"), timeKnob(0.6) },
                                          { opKey(4, "Sustain"), 0.0f },
                                          { "panSpread", 0.8f },
                                          { "ampDecay", timeKnob(2.5) },
                                          { "ampSustain", 0.0f },
                                          { "ampRelease", timeKnob(1.0) },
                                          { "ampCurve", 0.5f },
                                        } });

        list.push_back({ "Marimba", {
                                      { opKey(2, "Ratio"), 4.0f },
                                      { opKey(2, "Level"), 0.5f },
                                      { opKey(2, "Decay"), timeKnob(0.05) },
                                      { opKey(2, "Sustain"), 0.0f },
                                      { "ampDecay", timeKnob(0.28) },
                                      { "ampSustain", 0.0f },
                                      { "ampRelease", timeKnob(0.1) },
                                      { "ampCurve", 0.7f },
                                    } });

        list.push_back({ "Clav", {
                                   { opKey(2, "Ratio"), 3.0f },
                                   { opKey(2, "Level"), 0.65f },
                                   { opKey(2, "Decay"), timeKnob(0.04) },
                                   { opKey(2, "Sustain"), 0.1f },
                                   { opKey(2, "VelocitySensitivity"), 0.9f },
                                   { "hpfCutoff", 0.2f },
                                   { "ampDecay", timeKnob(0.25) },
                                   { "ampSustain", 0.1f },
                                   { "ampRelease", timeKnob(0.08) },
                                   { "ampCurve", 0.65f },
                                 } });

        // Brass is the modulator's slow attack: the note arrives dull and brightens into itself.
        list.push_back({ "Brass", {
                                    { opKey(2, "Level"), 0.55f },
                                    { opKey(2, "Attack"), attackKnob(0.3) },
                                    { opKey(2, "Decay"), timeKnob(0.7) },
                                    { opKey(2, "Sustain"), 0.55f },
                                    { opKey(2, "VelocitySensitivity"), 0.5f },
                                    { "ampAttack", attackKnob(0.12) },
                                    { "ampSustain", 0.85f },
                                    { "ampRelease", timeKnob(0.25) },
                                  } });

        list.push_back({ "Bright Lead", {
                                          { "algorithm", 1.0f },
                                          { opKey(2, "Level"), 0.5f },
                                          { opKey(2, "Decay"), timeKnob(0.4) },
                                          { opKey(2, "Sustain"), 0.6f },
                                          { opKey(3, "Ratio"), 2.0f },
                                          { opKey(3, "Level"), 0.35f },
                                          { opKey(3, "Sustain"), 0.3f },
                                          { opKey(4, "Ratio"), 3.0f },
                                          { opKey(4, "Level"), 0.2f },
                                          { opKey(4, "Sustain"), 0.2f },
                                          { "voiceMode", 5.0f },
                                          { "portamento", 0.25f },
                                          { "ampAttack", attackKnob(0.01) },
                                          { "ampSustain", 0.9f },
                                          { "ampRelease", timeKnob(0.2) },
                                        } });

        list.push_back({ "Metallic Lead", {
                                            { "algorithm", 2.0f },
                                            { opKey(2, "Ratio"), 5.0f },
                                            { opKey(2, "Detune"), 0.85f },
                                            { opKey(2, "Level"), 0.4f },
                                            { opKey(2, "Sustain"), 0.4f },
                                            { opKey(3, "Ratio"), 7.0f },
                                            { opKey(3, "Level"), 0.35f },
                                            { opKey(3, "Sustain"), 0.3f },
                                            { opKey(4, "Ratio"), 11.0f },
                                            { opKey(4, "Level"), 0.25f },
                                            { opKey(4, "Sustain"), 0.2f },
                                            { "feedback", 0.4f },
                                            { "ampSustain", 0.8f },
                                            { "ampRelease", timeKnob(0.25) },
                                          } });

        // Four sines at the first four ratios. The third is the one that sells it: a quint nobody
        // plays is what makes a stack of octaves read as an organ.
        list.push_back({ "Drawbar Organ", {
                                            { "algorithm", 7.0f },
                                            { opKey(2, "Ratio"), 2.0f },
                                            { opKey(2, "Level"), 0.7f },
                                            { opKey(3, "Ratio"), 3.0f },
                                            { opKey(3, "Level"), 0.45f },
                                            { opKey(4, "Ratio"), 4.0f },
                                            { opKey(4, "Level"), 0.35f },
                                            { "ampRelease", timeKnob(0.02) },
                                          } });

        list.push_back({ "Warm Pad", {
                                       { "algorithm", 5.0f },
                                       { opKey(2, "Level"), 0.35f },
                                       { opKey(2, "Attack"), attackKnob(0.4) },
                                       { opKey(2, "Decay"), timeKnob(1.0) },
                                       { opKey(2, "Sustain"), 0.5f },
                                       { opKey(3, "Ratio"), 2.0f },
                                       { opKey(3, "Level"), 0.4f },
                                       { opKey(4, "Level"), 0.2f },
                                       { opKey(4, "Attack"), attackKnob(0.6) },
                                       { opKey(4, "Sustain"), 0.4f },
                                       { "panSpread", 0.9f },
                                       { "lpfCutoff", 0.7f },
                                       { "ampAttack", attackKnob(0.35) },
                                       { "ampRelease", timeKnob(1.3) },
                                     } });

        // Past about two thirds the feedback loop breaks into noise rather than a waveform, and
        // that noise is the only one this synth has. Here it is the point.
        list.push_back({ "Metal Hit", {
                                        { opKey(2, "Ratio"), 11.0f },
                                        { opKey(2, "Level"), 0.6f },
                                        { opKey(2, "Decay"), timeKnob(0.1) },
                                        { opKey(2, "Sustain"), 0.0f },
                                        { opKey(3, "Ratio"), 7.0f },
                                        { opKey(3, "Level"), 0.5f },
                                        { opKey(3, "Decay"), timeKnob(0.06) },
                                        { opKey(3, "Sustain"), 0.0f },
                                        { opKey(4, "Ratio"), 13.0f },
                                        { opKey(4, "Level"), 0.5f },
                                        { opKey(4, "Decay"), timeKnob(0.06) },
                                        { opKey(4, "Sustain"), 0.0f },
                                        { "feedback", 0.9f },
                                        { "hpfCutoff", 0.25f },
                                        { "ampDecay", timeKnob(0.25) },
                                        { "ampSustain", 0.0f },
                                        { "ampRelease", timeKnob(0.08) },
                                        { "ampCurve", 0.7f },
                                      } });

        return list;
    }();

    return presetList;
}

SynthPreset FmSynthPresets::randomPatch(uint32_t seed)
{
    std::mt19937 rng { seed };

    const auto chance = [&rng](double probability) {
        return std::uniform_real_distribution<double> { 0.0, 1.0 }(rng) < probability;
    };
    const auto range = [&rng](double min, double max) {
        return static_cast<float>(std::uniform_real_distribution<double> { min, max }(rng));
    };
    const auto pick = [&rng](int count) {
        return std::uniform_int_distribution<int> { 0, count - 1 }(rng);
    };

    // Weighted towards the small whole numbers, because that is where the harmonic sounds live.
    // The high and the fractional entries are what make bells and metal, so they are in the table
    // but rare.
    static const std::array<float, 16> ratios { 1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 9, 11, 14 };

    std::map<std::string, float> patch;

    patch["algorithm"] = static_cast<float>(pick(static_cast<int>(FmSynthDevice::AlgorithmCount)));

    // The voice mode is deliberately left at Poly. Unison, Supersaw and Drift each spend the whole
    // voice pool on one note, so a patch in any of them cannot play a chord -- which is not
    // something a button offering "a patch" should hand back without being asked.

    // The shape is chosen once and every envelope follows it. Independent envelopes are what makes
    // a random FM patch sound broken rather than merely strange: a percussive modulator under a
    // sustaining carrier is a click, not a sound.
    enum class Shape
    {
        Percussive,
        Plucked,
        Sustained,
        Pad
    };
    const auto shape = static_cast<Shape>(pick(4));

    switch (shape) {
    case Shape::Percussive:
        patch["ampAttack"] = attackKnob(0.001);
        patch["ampDecay"] = timeKnob(range(0.12, 0.5));
        patch["ampSustain"] = 0.0f;
        patch["ampRelease"] = timeKnob(range(0.05, 0.2));
        patch["ampCurve"] = range(0.5, 0.8);
        break;
    case Shape::Plucked:
        patch["ampAttack"] = attackKnob(0.001);
        patch["ampDecay"] = timeKnob(range(0.6, 3.0));
        patch["ampSustain"] = range(0.1, 0.4);
        patch["ampRelease"] = timeKnob(range(0.1, 0.6));
        patch["ampCurve"] = range(0.4, 0.7);
        break;
    case Shape::Sustained:
        patch["ampAttack"] = attackKnob(range(0.005, 0.1));
        patch["ampSustain"] = range(0.7, 1.0);
        patch["ampRelease"] = timeKnob(range(0.1, 0.5));
        break;
    case Shape::Pad:
        patch["ampAttack"] = attackKnob(range(0.2, 0.7));
        patch["ampSustain"] = range(0.8, 1.0);
        patch["ampRelease"] = timeKnob(range(0.8, 3.0));
        patch["panSpread"] = range(0.5, 1.0);
        break;
    }

    const bool percussive = shape == Shape::Percussive || shape == Shape::Plucked;
    const auto & algorithm = FmSynthDevice::algorithms().at(static_cast<size_t>(patch["algorithm"]));

    for (size_t i = 0; i < FmSynthDevice::OperatorCount; i++) {
        const int op = static_cast<int>(i) + 1;
        const bool carrier = algorithm.carriers & (1u << i);

        // Operator 1 carries every algorithm, so it stays at full level: a random patch has to be
        // audible before it can be interesting.
        patch[opKey(op, "Level")] = op == 1 ? 1.0f
          : carrier                         ? range(0.4, 1.0)
                                            : range(0.15, 0.85);

        patch[opKey(op, "Ratio")] = op == 1 ? 1.0f : ratios.at(static_cast<size_t>(pick(static_cast<int>(ratios.size()))));

        // A little detune thickens; a lot is what takes a patch off the harmonic series. Rare,
        // because on most sounds it just reads as out of tune.
        patch[opKey(op, "Detune")] = chance(0.25) ? range(0.3, 0.7) : 0.5f;

        patch[opKey(op, "Attack")] = shape == Shape::Pad && chance(0.5) ? attackKnob(range(0.2, 1.0)) : attackKnob(0.001);

        // A modulator that decays away is the single most FM thing there is: the note starts bright
        // and settles. Carriers follow the voice's shape instead.
        if (carrier) {
            patch[opKey(op, "Decay")] = timeKnob(range(0.5, 4.0));
            patch[opKey(op, "Sustain")] = percussive ? range(0.3, 1.0) : range(0.7, 1.0);
        } else {
            patch[opKey(op, "Decay")] = timeKnob(percussive ? range(0.03, 0.4) : range(0.2, 2.0));
            patch[opKey(op, "Sustain")] = chance(0.6) ? range(0.0, 0.3) : range(0.4, 0.9);
            // Velocity into a modulator is what makes a patch respond to playing rather than just
            // get louder, so most random patches get some.
            patch[opKey(op, "VelocitySensitivity")] = chance(0.7) ? range(0.3, 1.0) : 0.0f;
        }

        // High notes climb towards Nyquist with the modulator's sidebands on top, so a random patch
        // is much likelier to be playable across the keyboard with some scaling on.
        if (!carrier && chance(0.6)) {
            patch[opKey(op, "KeyScale")] = range(0.2, 0.8);
        }
    }

    // Mostly none. A little is character, and the top of the range is noise -- which is worth
    // reaching occasionally, but not one patch in three.
    if (chance(0.35)) {
        patch["feedback"] = chance(0.2) ? range(0.7, 1.0) : range(0.1, 0.5);
    }

    if (chance(0.3)) {
        patch["lpfCutoff"] = range(0.4, 0.9);
    }

    if (chance(0.25)) {
        patch["lfoTarget"] = static_cast<float>(pick(3)); // Pitch, Cutoff or Mod Index
        patch["lfoRate"] = range(0.1, 0.5);
        // The intensity knob is bipolar and its centre is off, so a small offset is a small amount.
        patch["lfoIntensity"] = 0.5f + range(0.05, 0.2) * (chance(0.5) ? 1.0f : -1.0f);
    }

    return { "Random", patch };
}

} // namespace noteahead
