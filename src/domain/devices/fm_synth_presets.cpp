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

// The algorithms, by the ordinal the parameter persists. Named here because a patch that says
// "Branch" is readable and one that says 2.0f is not.
constexpr float Serial = 0.0f;
constexpr float Branch = 2.0f;
constexpr float TripleMod = 3.0f;
constexpr float ChainAndCarrier = 5.0f;
constexpr float SharedMod = 6.0f;
constexpr float Additive = 7.0f;

// Operator waveforms, as FmOperator orders them. Every patch above this line is pure sine; these
// are the other seven, which change what the sidebands are made of before any of them are placed.
constexpr float HalfSine = 1.0f;
constexpr float AbsSine = 2.0f;
constexpr float Saw = 7.0f;

// Modulation destinations. ModIndex scales every modulation path at once, which is the FM synth's
// brightness control and the destination worth reaching for most often.
constexpr float ModIndex = 2.0f;
constexpr float LfoPitch = 0.0f;
constexpr float LfoCutoff = 1.0f;
constexpr float LfoModIndex = 2.0f;
constexpr float LfoVolume = 3.0f;

// Voice modes, ordered as SynthDevice's are.
constexpr float Unison = 1.0f;
constexpr float Mono = 5.0f;

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

        // --- Added later ---
        //
        // Appended rather than filed in among the patches above, because a preset's position in
        // this list is the MIDI program number that selects it. Moving an existing patch would
        // change the instrument in any song that names one.

        // The other electric piano: a reed rather than a tine, so the modulator is a half sine --
        // which is a sine with its bottom half flattened, and so already full of the even harmonics
        // a reed barks with. Operator 3 runs alongside as a carrier of its own for the body.
        list.push_back({ "Wurly", {
                                    { "algorithm", ChainAndCarrier },
                                    { opKey(2, "Waveform"), HalfSine },
                                    { opKey(2, "Ratio"), 1.0f },
                                    { opKey(2, "Level"), 0.62f },
                                    { opKey(2, "Decay"), timeKnob(0.35) },
                                    { opKey(2, "Sustain"), 0.08f },
                                    { opKey(2, "VelocitySensitivity"), 0.85f },
                                    { opKey(3, "Level"), 0.4f },
                                    { opKey(4, "Ratio"), 3.0f },
                                    { opKey(4, "Level"), 0.25f },
                                    { opKey(4, "Decay"), timeKnob(0.09) },
                                    { opKey(4, "Sustain"), 0.0f },
                                    { opKey(4, "VelocitySensitivity"), 1.0f },
                                    { "ampDecay", timeKnob(2.2) },
                                    { "ampSustain", 0.22f },
                                    { "ampRelease", timeKnob(0.35) },
                                    { "ampCurve", 0.55f },
                                  } });

        // Three modulators into one carrier, all of them opening slowly. Brass is not bright at the
        // start of the note and bright afterwards -- it arrives, and the modulator attack is the
        // only thing here that can make it arrive.
        list.push_back({ "DX Brass", {
                                       { "algorithm", TripleMod },
                                       { opKey(2, "Ratio"), 1.0f },
                                       { opKey(2, "Level"), 0.5f },
                                       { opKey(2, "Attack"), attackKnob(0.09) },
                                       { opKey(2, "Sustain"), 0.7f },
                                       { opKey(3, "Ratio"), 2.0f },
                                       { opKey(3, "Level"), 0.3f },
                                       { opKey(3, "Attack"), attackKnob(0.13) },
                                       { opKey(3, "Sustain"), 0.55f },
                                       { opKey(4, "Ratio"), 3.0f },
                                       { opKey(4, "Level"), 0.16f },
                                       { opKey(4, "Attack"), attackKnob(0.16) },
                                       { opKey(4, "Sustain"), 0.4f },
                                       { "ampAttack", attackKnob(0.04) },
                                       { "ampDecay", timeKnob(1.2) },
                                       { "ampSustain", 0.8f },
                                       { "ampRelease", timeKnob(0.25) },
                                     } });

        // Feedback on the modulator turns its sine towards a saw, and the LFO on the modulation
        // index moves the whole thing rather than any one operator. Together that is the growl.
        list.push_back({ "Growl Bass", {
                                         { opKey(2, "Ratio"), 1.0f },
                                         { opKey(2, "Level"), 0.68f },
                                         { opKey(2, "Decay"), timeKnob(0.5) },
                                         { opKey(2, "Sustain"), 0.45f },
                                         { "feedback", 0.55f },
                                         { "lfoTarget", LfoModIndex },
                                         { "lfoRate", 0.42f },
                                         { "lfoIntensity", 0.35f },
                                         { "lpfCutoff", 0.7f },
                                         // Unison rather than Mono: a stack spends the pool on the
                                         // one note being played, which is what a bass wants and
                                         // what nothing else in this list asks for.
                                         { "voiceMode", Unison },
                                         { "voiceDepth", 0.25f },
                                         { "ampDecay", timeKnob(1.5) },
                                         { "ampSustain", 0.6f },
                                         { "ampRelease", timeKnob(0.12) },
                                       } });

        // One modulator into three carriers at once, which is what gives every partial the same
        // movement instead of three independent voices. The rectified sine is full of even
        // harmonics, and a struck bar is mostly even harmonics.
        list.push_back({ "Vibes", {
                                    { "algorithm", SharedMod },
                                    // Operators 1 to 3 are the carriers under this algorithm, so
                                    // these are the bar's partials rather than modulation depths.
                                    { opKey(1, "Level"), 1.0f },
                                    { opKey(2, "Ratio"), 4.0f },
                                    { opKey(2, "Level"), 0.5f },
                                    { opKey(3, "Ratio"), 9.0f },
                                    { opKey(3, "Level"), 0.22f },
                                    { opKey(4, "Waveform"), AbsSine },
                                    { opKey(4, "Ratio"), 1.0f },
                                    { opKey(4, "Level"), 0.28f },
                                    { opKey(4, "Decay"), timeKnob(0.25) },
                                    { opKey(4, "Sustain"), 0.0f },
                                    { "lfoTarget", LfoVolume },
                                    { "lfoRate", 0.55f }, // The vibraphone's fans, near enough
                                    { "lfoIntensity", 0.5f },
                                    { "ampDecay", timeKnob(2.5) },
                                    { "ampSustain", 0.0f },
                                    { "ampRelease", timeKnob(0.8) },
                                    { "ampCurve", 0.5f },
                                  } });

        // A branch rather than a stack: one modulator is itself modulated and another is not, so
        // two sets of sidebands arrive with different decays. A struck metal pan is exactly that.
        list.push_back({ "Steel Drum", {
                                         { "algorithm", Branch },
                                         { opKey(2, "Ratio"), 2.0f },
                                         { opKey(2, "Level"), 0.45f },
                                         { opKey(2, "Decay"), timeKnob(0.18) },
                                         { opKey(2, "Sustain"), 0.1f },
                                         { opKey(3, "Ratio"), 3.0f },
                                         { opKey(3, "Level"), 0.4f },
                                         { opKey(3, "Decay"), timeKnob(0.07) },
                                         { opKey(3, "Sustain"), 0.0f },
                                         { opKey(4, "Ratio"), 5.0f },
                                         { opKey(4, "Level"), 0.3f },
                                         { opKey(4, "Decay"), timeKnob(0.04) },
                                         { opKey(4, "Sustain"), 0.0f },
                                         { "ampDecay", timeKnob(1.1) },
                                         { "ampSustain", 0.0f },
                                         { "ampRelease", timeKnob(0.4) },
                                         { "ampCurve", 0.6f },
                                       } });

        // A saw modulator is the bluntest instrument here: every harmonic at once, straight into
        // the carrier. Gone in a tenth of a second it is a plucked string; held, it is unusable.
        list.push_back({ "Koto", {
                                   { "algorithm", Serial },
                                   { opKey(2, "Waveform"), Saw },
                                   { opKey(2, "Ratio"), 1.0f },
                                   { opKey(2, "Level"), 0.4f },
                                   { opKey(2, "Decay"), timeKnob(0.08) },
                                   { opKey(2, "Sustain"), 0.0f },
                                   { opKey(2, "VelocitySensitivity"), 0.8f },
                                   { opKey(3, "Ratio"), 2.0f },
                                   { opKey(3, "Level"), 0.3f },
                                   { opKey(3, "Decay"), timeKnob(0.05) },
                                   { opKey(3, "Sustain"), 0.0f },
                                   { "ampDecay", timeKnob(1.3) },
                                   { "ampSustain", 0.0f },
                                   { "ampRelease", timeKnob(0.5) },
                                   { "ampCurve", 0.65f },
                                 } });

        // No modulation at all: four carriers detuned against each other, which is the one
        // algorithm where this synth is an additive one. Levels are kept well down because four
        // carriers at once is also the one place it can run out of headroom.
        list.push_back({ "Ice Pad", {
                                      { "algorithm", Additive },
                                      { opKey(1, "Level"), 1.0f },
                                      { opKey(2, "Ratio"), 2.0f },
                                      { opKey(2, "Detune"), 0.53f },
                                      { opKey(2, "Level"), 0.4f },
                                      { opKey(3, "Ratio"), 4.0f },
                                      { opKey(3, "Detune"), 0.47f },
                                      { opKey(3, "Level"), 0.2f },
                                      { opKey(4, "Ratio"), 8.0f },
                                      { opKey(4, "Detune"), 0.55f },
                                      { opKey(4, "Level"), 0.1f },
                                      // Left polyphonic on purpose: a stacked voice mode spends the
                                      // whole pool on one note, and a pad is played as a chord. The
                                      // detune that makes this move is between the carriers
                                      // themselves rather than between voices.
                                      { "panSpread", 0.5f },
                                      { "lfoTarget", LfoPitch },
                                      { "lfoRate", 0.25f },
                                      { "lfoIntensity", 0.08f },
                                      { "ampAttack", attackKnob(0.2) },
                                      { "ampSustain", 1.0f },
                                      { "ampRelease", timeKnob(2.5) },
                                    } });

        // Feedback taken far enough that the operator is most of the way to noise, held rather than
        // struck, with the index wandering under it. Nothing decays here; the note is the drone.
        list.push_back({ "Bell Drone", {
                                         { opKey(2, "Ratio"), 11.0f },
                                         { opKey(2, "Detune"), 0.62f },
                                         { opKey(2, "Level"), 0.3f },
                                         { opKey(2, "Sustain"), 1.0f },
                                         { "feedback", 0.72f },
                                         { "lfoTarget", LfoModIndex },
                                         { "lfoRate", 0.18f },
                                         { "lfoIntensity", 0.6f },
                                         { "lpfCutoff", 0.8f },
                                         { "ampAttack", attackKnob(0.12) },
                                         { "ampSustain", 1.0f },
                                         { "ampRelease", timeKnob(3.0) },
                                       } });

        // The filter, which the patches above use only to take the top off. Here the second LFO
        // sweeps it with the resonance up, which is a formant moving rather than a tone control.
        list.push_back({ "Talking Lead", {
                                           { "algorithm", Serial },
                                           { opKey(2, "Ratio"), 2.0f },
                                           { opKey(2, "Level"), 0.45f },
                                           { opKey(2, "Sustain"), 0.6f },
                                           { opKey(3, "Ratio"), 1.0f },
                                           { opKey(3, "Level"), 0.25f },
                                           { opKey(3, "Sustain"), 0.5f },
                                           { "modTarget", ModIndex },
                                           { "modIntensity", 0.4f },
                                           { "modDecay", timeKnob(0.3) },
                                           { "lpfCutoff", 0.55f },
                                           { "lpfResonance", 0.6f },
                                           { "lfo2Target", LfoCutoff },
                                           { "lfo2Rate", 0.4f },
                                           { "lfo2Intensity", 0.55f },
                                           { "voiceMode", Mono },
                                           { "portamento", 0.12f },
                                           { "ampAttack", attackKnob(0.01) },
                                           { "ampSustain", 0.9f },
                                           { "ampRelease", timeKnob(0.2) },
                                         } });

        // The delay, which no patch above touches at all. Synced, so the repeats stay with the song
        // whatever tempo it is played at, and darkened in the feedback path so they fall away.
        list.push_back({ "Tape Echo Keys", {
                                             { "algorithm", 4.0f }, // Twin Stacks, as the pianos use
                                             { opKey(2, "Ratio"), 1.0f },
                                             { opKey(2, "Level"), 0.4f },
                                             { opKey(2, "Decay"), timeKnob(0.4) },
                                             { opKey(2, "Sustain"), 0.1f },
                                             { opKey(4, "Ratio"), 7.0f },
                                             { opKey(4, "Level"), 0.2f },
                                             { opKey(4, "Decay"), timeKnob(0.1) },
                                             { opKey(4, "Sustain"), 0.0f },
                                             { "delaySync", 1.0f },
                                             { "delaySyncDivision", 0.375f }, // A dotted eighth
                                             { "delayFeedback", 0.5f },
                                             { "delayFeedbackLpf", 0.4f },
                                             { "delayMix", 0.35f },
                                             { "ampDecay", timeKnob(1.6) },
                                             { "ampSustain", 0.2f },
                                             { "ampRelease", timeKnob(0.3) },
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
