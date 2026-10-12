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

#ifndef FM_DRUMS_PRESETS_HPP
#define FM_DRUMS_PRESETS_HPP

#include "../dsp/drum/fm_drum_engine.hpp"
#include "drum_synth_v2_constants.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace noteahead {

//! One operator of a pad as a preset gives it, in the units its controls read out in.
struct FmDrumsOperatorVoicing
{
    FmOperator::Waveform waveform { FmOperator::Waveform::Sine };
    double ratio { 1.0 };
    //! Knob position: the engine gets its square, as the FM Synth's operators do.
    double level { 0.0 };
    double attack { 0.0 };
    double decay { 0.3 };
    double curve { 0.5 };
};

//! One pad's sound as a preset gives it, in the units its controls read out in: the FM voice and the
//! pad's filters. Not its level, pan or amp envelope, which are its mix rather than its sound.
struct FmDrumsPadVoicing
{
    size_t algorithm { 0 };
    double frequency { 100.0 };
    double feedback { 0.0 };
    double pitchSemitones { 0.0 };
    double pitchDecay { 0.03 };
    std::array<FmDrumsOperatorVoicing, FmDrumEngine::OperatorCount> operators {};
    double noiseLevel { 0.0 };
    double noiseModulation { 0.0 };
    size_t noiseTarget { 0 };
    double noiseAttack { 0.0 };
    double noiseDecay { 0.1 };
    double noiseCurve { 0.5 };
    FmDrumEngine::NoiseFilterMode noiseFilterMode { FmDrumEngine::NoiseFilterMode::HighPass };
    double noiseCutoffHz { 5000.0 };
    double noiseResonance { 0.0 };
    double velocityToLevel { 1.0 };
    double velocityToModulation { 0.5 };
    //! The pad's own low and high pass, as knob positions. Part of the sound for a cymbal, whose FM
    //! spreads further both ways than the metal it stands for.
    double lpfCutoff { 1.0 };
    double hpfCutoff { 0.0 };
};

struct FmDrumsKit
{
    std::string name;
    std::array<FmDrumsPadVoicing, DrumSynthV2::NumVoices> pads;
    //! What each pad is in this kit, where that is not the drum the pad is named after.
    std::array<std::string, DrumSynthV2::NumVoices> soundNames {};
};

struct FmDrumsPadPreset
{
    std::string name;
    FmDrumsPadVoicing voicing;
};

//! The factory kits and single-pad sounds of the FM Drums.
class FmDrumsPresets
{
public:
    //! Every factory kit. The first is the one a new device starts with, so its pads are the
    //! parameters' defaults: changing it changes what a project that never touched a pad sounds like.
    static const std::vector<FmDrumsKit> & kits();

    //! Every pad of every kit as a sound of its own, to load onto any pad.
    static const std::vector<FmDrumsPadPreset> & pads();
};

} // namespace noteahead

#endif // FM_DRUMS_PRESETS_HPP
