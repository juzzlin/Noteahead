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

#include "fm_drums_presets.hpp"

namespace noteahead {

using namespace DrumSynthV2;

namespace {

using Waveform = FmOperator::Waveform;
using NoiseFilterMode = FmDrumEngine::NoiseFilterMode;

// Algorithm ordinals, as FmSynthDevice::algorithms() lists them
constexpr size_t Serial { 0 };
constexpr size_t TwinStacks { 4 };
constexpr size_t Additive { 7 };

//! @p crash played backwards, as near as attack-decay envelopes get: everything swells over the
//! longest attack there is and then stops short.
FmDrumsPadVoicing reversed(FmDrumsPadVoicing crash)
{
    constexpr double SwellSeconds { 1.0 };
    constexpr double CutSeconds { 0.05 };
    for (auto && op : crash.operators) {
        op.attack = SwellSeconds;
        op.decay = CutSeconds;
        op.curve = 0.3;
    }
    crash.noiseAttack = SwellSeconds;
    crash.noiseDecay = CutSeconds;
    crash.noiseCurve = 0.3;
    return crash;
}

//! Every pad fitted to Drum Synth V2's, band for band and moment for moment, on a measure that also
//! weighs how noisy each band is: what tells a hi-hat from a cowbell is not where its energy sits but
//! that it is noise there rather than partials. All but the reverse crash.
std::array<FmDrumsPadVoicing, NumVoices> analogKit()
{
    std::array<FmDrumsPadVoicing, NumVoices> kit { {
      // Kick
      { .algorithm = 0,
        .frequency = 48.0113,
        .feedback = 0.8324,
        .pitchSemitones = 30.0,
        .pitchDecay = 0.025,
        .operators = { { { .ratio = 0.999861, .level = 1.0, .decay = 2.6985, .curve = 0.0421 },
                         { .waveform = Waveform::AlternatingAbsSine, .ratio = 0.617966, .level = 0.007, .decay = 0.0195331, .curve = 0.3834 },
                         { .waveform = Waveform::Square, .ratio = 3.3776, .level = 0.3355, .decay = 0.146277, .curve = 0.5168 },
                         { .waveform = Waveform::QuarterSine, .ratio = 2.55833, .level = 0.2366, .attack = 0.15878, .decay = 3.21174, .curve = 0.9298 } } },
        .noiseLevel = 0.0,
        .noiseModulation = 0.0,
        .noiseTarget = 2,
        .noiseDecay = 8.0,
        .noiseCurve = 0.0255,
        .noiseFilterMode = NoiseFilterMode::BandPass,
        .noiseCutoffHz = 7489.11,
        .noiseResonance = 0.6151 },
      // Snare
      { .algorithm = 1,
        .frequency = 26.127,
        .feedback = 0.8978,
        .pitchSemitones = 7.0,
        .pitchDecay = 0.02,
        .operators = { { { .ratio = 0.380508, .level = 1.0, .attack = 0.00116425, .decay = 1.3759, .curve = 0.9119 },
                         { .waveform = Waveform::Square, .ratio = 7.89314, .level = 0.7494, .decay = 0.00836175, .curve = 0.3249 },
                         { .waveform = Waveform::AlternatingSine, .ratio = 12.5342, .level = 0.1143, .attack = 0.00173665, .decay = 8.0, .curve = 0.0651 },
                         { .waveform = Waveform::Square, .ratio = 1.1774, .level = 0.154, .attack = 0.00755261, .decay = 1.86465, .curve = 0.673 } } },
        .noiseLevel = 0.1432,
        .noiseModulation = 0.2027,
        .noiseTarget = 3,
        .noiseDecay = 0.40044,
        .noiseCurve = 0.3579,
        .noiseFilterMode = NoiseFilterMode::LowPass,
        .noiseCutoffHz = 3239.89,
        .noiseResonance = 0.2254 },
      // Closed hi-hat
      { .algorithm = 5,
        .frequency = 2081.69,
        .feedback = 0.0862,
        .operators = { { { .waveform = Waveform::AlternatingAbsSine, .ratio = 0.385767, .level = 0.7766, .decay = 0.106041, .curve = 1.0 },
                         { .ratio = 1.47019, .level = 0.8332, .attack = 0.00191586, .decay = 0.879219, .curve = 0.0 },
                         { .ratio = 1.66832, .level = 0.3931, .decay = 0.0999638, .curve = 0.9034 },
                         { .waveform = Waveform::HalfSine, .ratio = 5.95376, .level = 0.0198, .attack = 0.015984, .decay = 8.0, .curve = 0.0 } } },
        .noiseLevel = 0.4657,
        .noiseModulation = 0.7311,
        .noiseTarget = 0,
        .noiseDecay = 0.0999638,
        .noiseCurve = 0.8426,
        .noiseFilterMode = NoiseFilterMode::HighPass,
        .noiseCutoffHz = 7305.16,
        .noiseResonance = 0.2824 },
      // Clap
      { .algorithm = 7,
        .frequency = 602.005,
        .feedback = 0.2582,
        .operators = { { { .waveform = Waveform::AlternatingAbsSine, .ratio = 1.30749, .level = 0.8859, .attack = 0.0298487, .decay = 0.287523, .curve = 0.0 },
                         { .waveform = Waveform::AlternatingSine, .ratio = 0.331573, .level = 0.9951, .attack = 0.00292421, .decay = 0.0385363, .curve = 0.7765 },
                         { .waveform = Waveform::AbsSine, .ratio = 0.646893, .level = 1.0, .attack = 0.0438749, .decay = 0.005, .curve = 0.6697 },
                         { .waveform = Waveform::Square, .ratio = 15.2211, .level = 0.3147, .attack = 0.0365283, .decay = 0.320933, .curve = 0.7157 } } },
        .noiseLevel = 0.8749,
        .noiseModulation = 0.4341,
        .noiseTarget = 2,
        .noiseAttack = 0.043059,
        .noiseDecay = 0.674138,
        .noiseCurve = 0.7703,
        .noiseFilterMode = NoiseFilterMode::BandPass,
        .noiseCutoffHz = 1287.45,
        .noiseResonance = 0.4823 },
      // Open hi-hat
      { .algorithm = 1,
        .frequency = 77.0206,
        .feedback = 0.2778,
        .operators = { { { .waveform = Waveform::Saw, .ratio = 1.00111, .level = 0.5814, .attack = 0.0210246, .decay = 0.360877, .curve = 0.9832 },
                         { .waveform = Waveform::AlternatingAbsSine, .ratio = 12.261, .level = 1.0, .attack = 0.00602857, .decay = 0.00511567, .curve = 0.0 },
                         { .waveform = Waveform::AlternatingSine, .ratio = 2.33658, .level = 1.0, .attack = 0.0961978, .decay = 0.0108573, .curve = 0.6419 },
                         { .waveform = Waveform::Square, .ratio = 16.0, .level = 0.5857, .attack = 0.0171137, .decay = 0.00552364, .curve = 1.0 } } },
        .noiseLevel = 0.4815,
        .noiseModulation = 0.567,
        .noiseTarget = 0,
        .noiseDecay = 0.833119,
        .noiseCurve = 0.7801,
        .noiseFilterMode = NoiseFilterMode::HighPass,
        .noiseCutoffHz = 7101.17,
        .noiseResonance = 0.1233,
        .lpfCutoff = 0.975,
        .hpfCutoff = 0.3157 },
      // Low tom
      { .algorithm = 0,
        .frequency = 53.7949,
        .feedback = 0.0644,
        .pitchSemitones = 10.0,
        .pitchDecay = 0.05,
        .operators = { { { .ratio = 1.75297, .level = 0.99, .decay = 0.662306, .curve = 0.5176 },
                         { .waveform = Waveform::AbsSine, .ratio = 0.791466, .level = 0.3236, .decay = 0.447961, .curve = 0.8006 },
                         { .waveform = Waveform::Saw, .ratio = 2.43378, .level = 0.3336, .attack = 0.00354658, .decay = 0.322119, .curve = 0.1272 },
                         { .waveform = Waveform::AbsSine, .ratio = 0.516128, .level = 0.0747, .attack = 0.073192, .decay = 0.267666, .curve = 0.1465 } } },
        .noiseLevel = 0.1378,
        .noiseModulation = 0.106,
        .noiseTarget = 1,
        .noiseAttack = 0.0532422,
        .noiseDecay = 0.0321165,
        .noiseCurve = 1.0,
        .noiseFilterMode = NoiseFilterMode::LowPass,
        .noiseCutoffHz = 2192.96,
        .noiseResonance = 0.3092 },
      // Mid tom
      { .algorithm = 0,
        .frequency = 119.995,
        .feedback = 0.5984,
        .pitchSemitones = 10.0,
        .pitchDecay = 0.045,
        .operators = { { { .ratio = 1.43038, .level = 0.8429, .decay = 0.599962, .curve = 0.7411 },
                         { .waveform = Waveform::Square, .ratio = 2.21637, .level = 0.2843, .attack = 0.0126493, .decay = 0.244446, .curve = 0.6609 },
                         { .waveform = Waveform::Square, .ratio = 3.51518, .level = 0.2898, .attack = 0.00677272, .decay = 0.830051, .curve = 0.0 },
                         { .ratio = 0.999861, .level = 0.2015, .attack = 0.184706, .decay = 0.0445648, .curve = 0.4514 } } },
        .noiseLevel = 0.1245,
        .noiseModulation = 0.33,
        .noiseTarget = 3,
        .noiseAttack = 0.0296471,
        .noiseDecay = 0.0322115,
        .noiseCurve = 0.5292,
        .noiseFilterMode = NoiseFilterMode::LowPass,
        .noiseCutoffHz = 1877.28,
        .noiseResonance = 0.0175 },
      // High tom
      { .algorithm = 3,
        .frequency = 170.009,
        .feedback = 0.8566,
        .pitchSemitones = 10.0,
        .pitchDecay = 0.04,
        .operators = { { { .waveform = Waveform::AbsSine, .ratio = 0.458629, .level = 0.9201, .decay = 0.713543, .curve = 0.8591 },
                         { .waveform = Waveform::AbsSine, .ratio = 1.29128, .level = 0.8858, .decay = 0.013231, .curve = 0.9229 },
                         { .waveform = Waveform::HalfSine, .ratio = 0.431251, .level = 0.3218, .attack = 0.0334797, .decay = 7.02583, .curve = 0.164 },
                         { .waveform = Waveform::Square, .ratio = 9.96728, .level = 0.0733, .decay = 0.0154825, .curve = 0.0 } } },
        .noiseLevel = 0.277,
        .noiseModulation = 0.0039,
        .noiseTarget = 2,
        .noiseDecay = 0.0875969,
        .noiseCurve = 0.2356,
        .noiseFilterMode = NoiseFilterMode::LowPass,
        .noiseCutoffHz = 2257.51,
        .noiseResonance = 0.3114 },
      // Crash
      { .algorithm = 4,
        .frequency = 805.325,
        .operators = { { { .waveform = Waveform::AlternatingSine, .ratio = 4.31153, .level = 0.4428, .attack = 0.325093, .decay = 2.73055, .curve = 0.4409 },
                         { .waveform = Waveform::Square, .ratio = 0.36913, .level = 0.4582, .decay = 8.0, .curve = 0.2038 },
                         { .ratio = 10.5254, .level = 1.0, .attack = 0.0365944, .decay = 0.725219, .curve = 0.9345 },
                         { .ratio = 0.25, .level = 0.8679, .attack = 1.0, .decay = 0.007252, .curve = 0.3815 } } },
        .noiseLevel = 0.0,
        .noiseModulation = 0.679,
        .noiseTarget = 2,
        .noiseAttack = 0.215892,
        .noiseDecay = 0.0859327,
        .noiseCurve = 0.536,
        .noiseFilterMode = NoiseFilterMode::LowPass,
        .noiseCutoffHz = 17335.2,
        .noiseResonance = 0.5275,
        .lpfCutoff = 0.8028,
        .hpfCutoff = 0.387 },
      // Ride
      { .algorithm = 4,
        .frequency = 566.842,
        .feedback = 0.2874,
        .operators = { { { .waveform = Waveform::AlternatingSine, .ratio = 2.79646, .level = 0.7046, .decay = 2.36816, .curve = 1.0 },
                         { .waveform = Waveform::AlternatingSine, .ratio = 0.819605, .level = 1.0, .attack = 0.029475, .decay = 2.97452, .curve = 0.5292 },
                         { .ratio = 0.252823, .level = 0.6273, .attack = 0.00239535, .decay = 4.34299, .curve = 0.0527 },
                         { .waveform = Waveform::HalfSine, .ratio = 9.85189, .level = 0.4127, .attack = 0.00191586, .decay = 8.0, .curve = 0.5477 } } },
        .noiseLevel = 0.0,
        .noiseModulation = 0.5393,
        .noiseTarget = 2,
        .noiseDecay = 8.0,
        .noiseCurve = 0.3706,
        .noiseFilterMode = NoiseFilterMode::BandPass,
        .noiseCutoffHz = 1824.86,
        .noiseResonance = 0.0,
        .lpfCutoff = 0.9983 },
      {}, // Reverse crash, below
      // Rim
      { .algorithm = 0,
        .frequency = 520.061,
        .operators = { { { .waveform = Waveform::HalfSine, .ratio = 0.389474, .level = 1.0, .attack = 0.00268562, .decay = 0.08549, .curve = 0.7237 },
                         { .ratio = 0.570777, .level = 0.2205, .decay = 0.161835, .curve = 0.4479 },
                         { .ratio = 0.999861, .level = 0.1206, .decay = 8.0, .curve = 0.0 },
                         { .waveform = Waveform::AbsSine, .ratio = 1.51425, .level = 0.2376, .attack = 0.0166595, .decay = 0.720951, .curve = 0.0 } } },
        .noiseLevel = 0.33,
        .noiseModulation = 0.2755,
        .noiseTarget = 2,
        .noiseAttack = 0.00163053,
        .noiseDecay = 0.005,
        .noiseCurve = 0.4508,
        .noiseFilterMode = NoiseFilterMode::LowPass,
        .noiseCutoffHz = 3761.23,
        .noiseResonance = 0.7347 },
    } };
    // Not fitted: V2's is a two second swell, and an attack that tops out at one second fits it
    // best by staying nearly silent. The crash turned round instead, as the original one was.
    kit.at(static_cast<size_t>(VoiceIndex::ReverseCrash)) = reversed(kit.at(static_cast<size_t>(VoiceIndex::Crash)));
    return kit;
}

//! The kit a new FM Drums starts with, in pad order: the drums built the way FM drum machines have
//! always built them, a sine falling in pitch with noise for what FM cannot rattle on its own, and
//! the metal from the Analog kit.
std::array<FmDrumsPadVoicing, NumVoices> defaultKit()
{
    // The metal is the fitted one. FM's own recipes for it -- two inharmonic stacks at a few hundred
    // hertz -- gave a handful of clean partials where a cymbal has noise, and rang like cowbells.
    const auto analog = analogKit();
    return { {
      // Kick: a sine diving three octaves, and a modulator gone in forty milliseconds for the beater
      { .algorithm = Serial,
        .frequency = 48.0,
        .pitchSemitones = 30.0,
        .pitchDecay = 0.025,
        .operators = { { { .level = 1.0, .decay = 1.2, .curve = 0.6 },
                         { .ratio = 1.0, .level = 0.45, .decay = 0.04, .curve = 0.7 },
                         {},
                         {} } } },
      // Snare: a short pitched body, with band passed noise both heard and rattling the body
      { .algorithm = Serial,
        .frequency = 185.0,
        .pitchSemitones = 7.0,
        .pitchDecay = 0.02,
        .operators = { { { .level = 0.8, .decay = 0.3, .curve = 0.6 },
                         { .ratio = 1.6, .level = 0.4, .decay = 0.05, .curve = 0.6 },
                         {},
                         {} } },
        .noiseLevel = 0.4,
        .noiseModulation = 0.25,
        .noiseDecay = 0.4,
        .noiseCurve = 0.6,
        .noiseFilterMode = NoiseFilterMode::BandPass,
        .noiseCutoffHz = 4500.0,
        .noiseResonance = 0.2 },
      analog.at(static_cast<size_t>(VoiceIndex::ClosedHiHat)),
      // Clap: noise round two kilohertz, rattling a quiet sine for a body
      { .algorithm = Serial,
        .frequency = 1100.0,
        .operators = { { { .level = 0.35, .decay = 0.1, .curve = 0.6 }, {}, {}, {} } },
        .noiseLevel = 1.0,
        .noiseModulation = 0.8,
        .noiseDecay = 0.5,
        .noiseCurve = 0.7,
        .noiseFilterMode = NoiseFilterMode::BandPass,
        .noiseCutoffHz = 1800.0,
        .noiseResonance = 0.3 },
      analog.at(static_cast<size_t>(VoiceIndex::OpenHiHat)),
      // Low tom
      { .algorithm = Serial,
        .frequency = 82.0,
        .pitchSemitones = 10.0,
        .pitchDecay = 0.05,
        .operators = { { { .level = 0.8, .decay = 0.6, .curve = 0.6 },
                         { .ratio = 1.5, .level = 0.25, .decay = 0.06, .curve = 0.6 },
                         {},
                         {} } },
        .noiseLevel = 0.15,
        .noiseDecay = 0.03,
        .noiseFilterMode = NoiseFilterMode::LowPass,
        .noiseCutoffHz = 3000.0 },
      // Mid tom
      { .algorithm = Serial,
        .frequency = 120.0,
        .pitchSemitones = 10.0,
        .pitchDecay = 0.045,
        .operators = { { { .level = 0.8, .decay = 0.6, .curve = 0.6 },
                         { .ratio = 1.5, .level = 0.25, .decay = 0.06, .curve = 0.6 },
                         {},
                         {} } },
        .noiseLevel = 0.15,
        .noiseDecay = 0.03,
        .noiseFilterMode = NoiseFilterMode::LowPass,
        .noiseCutoffHz = 3000.0 },
      // High tom
      { .algorithm = Serial,
        .frequency = 170.0,
        .pitchSemitones = 10.0,
        .pitchDecay = 0.04,
        .operators = { { { .level = 0.8, .decay = 0.6, .curve = 0.6 },
                         { .ratio = 1.5, .level = 0.25, .decay = 0.06, .curve = 0.6 },
                         {},
                         {} } },
        .noiseLevel = 0.15,
        .noiseDecay = 0.03,
        .noiseFilterMode = NoiseFilterMode::LowPass,
        .noiseCutoffHz = 3000.0 },
      analog.at(static_cast<size_t>(VoiceIndex::Crash)),
      analog.at(static_cast<size_t>(VoiceIndex::Ride)),
      analog.at(static_cast<size_t>(VoiceIndex::ReverseCrash)),
      // Rim: a high, hard and very short click
      { .algorithm = Serial,
        .frequency = 520.0,
        .operators = { { { .level = 1.0, .decay = 0.035, .curve = 0.7 },
                         { .ratio = 2.3, .level = 0.5, .decay = 0.012, .curve = 0.7 },
                         {},
                         {} } },
        .noiseLevel = 0.3,
        .noiseDecay = 0.012,
        .noiseFilterMode = NoiseFilterMode::BandPass,
        .noiseCutoffHz = 3500.0 },
    } };
}

//! What FM is best at besides drums: tuned and inharmonic percussion, on the pads whose notes
//! they take over.
std::array<FmDrumsPadVoicing, NumVoices> percussionKit()
{
    const auto conga = [](double frequency, double decay) {
        // A short slap of a fifth above, gone before the skin has properly started ringing
        return FmDrumsPadVoicing { .algorithm = Serial,
                                   .frequency = frequency,
                                   .pitchSemitones = 3.0,
                                   .pitchDecay = 0.01,
                                   .operators = { { { .level = 0.85, .decay = decay, .curve = 0.6 },
                                                    { .ratio = 1.5, .level = 0.25, .decay = 0.02, .curve = 0.7 },
                                                    {},
                                                    {} } },
                                   .noiseLevel = 0.1,
                                   .noiseDecay = 0.015,
                                   .noiseFilterMode = NoiseFilterMode::LowPass,
                                   .noiseCutoffHz = 2000.0 };
    };
    return { {
      // Boom: a long sine kick with only a little fall to it
      { .algorithm = Serial,
        .frequency = 42.0,
        .pitchSemitones = 12.0,
        .pitchDecay = 0.06,
        .operators = { { { .level = 1.0, .decay = 2.5, .curve = 0.5 }, {}, {}, {} } } },
      // Timbale: a high, ringing shell with an inharmonic overtone
      { .algorithm = Serial,
        .frequency = 330.0,
        .pitchSemitones = 5.0,
        .pitchDecay = 0.015,
        .operators = { { { .level = 0.8, .decay = 0.5, .curve = 0.6 },
                         { .ratio = 2.76, .level = 0.35, .decay = 0.08, .curve = 0.6 },
                         {},
                         {} } },
        .noiseLevel = 0.15,
        .noiseDecay = 0.05,
        .noiseFilterMode = NoiseFilterMode::BandPass,
        .noiseCutoffHz = 3000.0 },
      // Shaker: band passed noise only, with an attack for the beads to arrive on
      { .operators = { { { .level = 0.0 }, {}, {}, {} } },
        .noiseLevel = 0.7,
        .noiseAttack = 0.025,
        .noiseDecay = 0.08,
        .noiseCurve = 0.6,
        .noiseFilterMode = NoiseFilterMode::BandPass,
        .noiseCutoffHz = 7000.0,
        .noiseResonance = 0.3 },
      // Clave: very nearly a pure sine, with a click on the front
      { .algorithm = Serial,
        .frequency = 2500.0,
        .operators = { { { .level = 0.9, .decay = 0.06, .curve = 0.7 },
                         { .ratio = 1.0, .level = 0.15, .decay = 0.01, .curve = 0.7 },
                         {},
                         {} } } },
      // Tambourine: two inharmonic stacks for the jingles, and high passed noise
      { .algorithm = TwinStacks,
        .frequency = 3200.0,
        .operators = { { { .level = 0.6, .attack = 0.005, .decay = 0.35, .curve = 0.6 },
                         { .ratio = 2.37, .level = 0.8, .decay = 0.35 },
                         { .ratio = 1.43, .level = 0.6, .attack = 0.005, .decay = 0.35, .curve = 0.6 },
                         { .ratio = 3.11, .level = 0.8, .decay = 0.35 } } },
        .noiseLevel = 0.5,
        .noiseAttack = 0.005,
        .noiseDecay = 0.3,
        .noiseCutoffHz = 8000.0 },
      conga(175.0, 0.35),
      conga(225.0, 0.3),
      conga(300.0, 0.25),
      // Bell: the DX recipe, a modulator three and a half times up whose depth dies with the note
      { .algorithm = Serial,
        .frequency = 880.0,
        .operators = { { { .level = 0.8, .decay = 3.0, .curve = 0.5 },
                         { .ratio = 3.5, .level = 0.5, .decay = 2.5, .curve = 0.5 },
                         {},
                         {} } },
        .velocityToModulation = 0.7 },
      // Cowbell: the 808's, two square partials a fifth and a bit apart
      { .algorithm = Additive,
        .frequency = 540.0,
        .operators = { { { .waveform = Waveform::Square, .level = 0.8, .decay = 0.35, .curve = 0.75 },
                         { .waveform = Waveform::Square, .ratio = 1.48, .level = 0.8, .decay = 0.35, .curve = 0.75 },
                         {},
                         {} } } },
      // Triangle: high, long and faintly inharmonic
      { .algorithm = Serial,
        .frequency = 3950.0,
        .operators = { { { .level = 0.7, .decay = 2.5, .curve = 0.4 },
                         { .ratio = 2.76, .level = 0.2, .decay = 1.5 },
                         {},
                         {} } } },
      // Woodblock: hollow, short and a little sharp at the start
      { .algorithm = Serial,
        .frequency = 780.0,
        .pitchSemitones = 2.0,
        .pitchDecay = 0.005,
        .operators = { { { .level = 0.9, .decay = 0.07, .curve = 0.7 },
                         { .ratio = 2.6, .level = 0.3, .decay = 0.02, .curve = 0.7 },
                         {},
                         {} } } },
    } };
}

} // namespace

const std::vector<FmDrumsKit> & FmDrumsPresets::kits()
{
    static const std::vector<FmDrumsKit> kits {
        { "Default", defaultKit() },
        { "Analog", analogKit() },
        { "Percussion",
          percussionKit(),
          { "Boom", "Timbale", "Shaker", "Clave", "Tambourine", "Conga Low", "Conga Mid", "Conga High", "Bell", "Cowbell", "Triangle", "Woodblock" } },
    };
    return kits;
}

const std::vector<FmDrumsPadPreset> & FmDrumsPresets::pads()
{
    static const std::vector<FmDrumsPadPreset> pads = [] {
        std::vector<FmDrumsPadPreset> list;
        for (auto && kit : kits()) {
            for (int pad = 0; pad < NumVoices; pad++) {
                const auto & sound = kit.soundNames.at(static_cast<size_t>(pad));
                const auto name = sound.empty() ? voiceName(pad).toStdString() : sound;
                list.push_back({ kit.name + " " + name, kit.pads.at(static_cast<size_t>(pad)) });
            }
        }
        return list;
    }();
    return pads;
}

} // namespace noteahead
