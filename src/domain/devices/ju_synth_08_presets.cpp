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

namespace noteahead {

namespace {

namespace C = Constants::NahdXml;

} // namespace

const std::vector<JuSynth08Presets::Preset> & JuSynth08Presets::presets()
{
    static const std::vector<Preset> Presets {
        {
          .name = "Init",
          .parameters = {},
        },
        {
          .name = "Jupiter Strings",
          .parameters = {
            { C::xmlKeyVco1Range().toStdString(), 1.0f }, // 8'
            { C::xmlKeyVco1Waveform().toStdString(), 1.0f }, // Saw
            { C::xmlKeyVco1Level().toStdString(), 0.85f },
            { C::xmlKeyVco2Range().toStdString(), 1.0f }, // 8'
            { C::xmlKeyVco2Waveform().toStdString(), 0.0f }, // Saw
            { C::xmlKeyVco2FineTune().toStdString(), 0.12f },
            { C::xmlKeyVco2Level().toStdString(), 0.85f },
            { C::xmlKeyVcfSlope().toStdString(), 0.0f }, // 12 dB/oct silky open slope
            { C::xmlKeyLpfCutoff().toStdString(), 0.65f },
            { C::xmlKeyLpfResonance().toStdString(), 0.15f },
            { C::xmlKeyEnvMod().toStdString(), 0.35f },
            { C::xmlKeyKeyTrack().toStdString(), 0.6f },
            { C::xmlKeyEnv1Attack().toStdString(), 0.25f },
            { C::xmlKeyEnv1Decay().toStdString(), 0.6f },
            { C::xmlKeyEnv1Sustain().toStdString(), 0.7f },
            { C::xmlKeyEnv1Release().toStdString(), 0.5f },
            { C::xmlKeyEnv2Attack().toStdString(), 0.25f },
            { C::xmlKeyEnv2Decay().toStdString(), 0.6f },
            { C::xmlKeyEnv2Sustain().toStdString(), 0.85f },
            { C::xmlKeyEnv2Release().toStdString(), 0.55f },
            { C::xmlKeyPanSpread().toStdString(), 0.6f },
            { C::xmlKeyDelayLevel().toStdString(), 0.25f },
            { C::xmlKeyDelayTime().toStdString(), 0.35f },
            { C::xmlKeyDelayFeedback().toStdString(), 0.3f },
          },
        },
        {
          .name = "Sync Lead",
          .parameters = {
            { C::xmlKeyVoiceMode().toStdString(), 1.0f }, // Solo
            { C::xmlKeyPortamento().toStdString(), 0.15f },
            { C::xmlKeyVco1Range().toStdString(), 1.0f }, // 8'
            { C::xmlKeyVco1Waveform().toStdString(), 1.0f }, // Saw
            { C::xmlKeyVco1Level().toStdString(), 0.9f },
            { C::xmlKeyVco2Range().toStdString(), 2.0f }, // 4'
            { C::xmlKeyVco2Waveform().toStdString(), 0.0f }, // Saw
            { C::xmlKeyVco2Sync().toStdString(), 1.0f }, // Hard sync ON
            { C::xmlKeyVco2FineTune().toStdString(), 0.35f },
            { C::xmlKeyVco2Level().toStdString(), 1.0f },
            { C::xmlKeyPitchEnvDepth().toStdString(), 0.65f },
            { C::xmlKeyPitchEnvTarget().toStdString(), 1.0f }, // VCO-2 only
            { C::xmlKeyVcfSlope().toStdString(), 1.0f }, // 24 dB/oct bite
            { C::xmlKeyLpfCutoff().toStdString(), 0.75f },
            { C::xmlKeyLpfResonance().toStdString(), 0.45f },
            { C::xmlKeyEnvMod().toStdString(), 0.4f },
            { C::xmlKeyEnv1Attack().toStdString(), 0.05f },
            { C::xmlKeyEnv1Decay().toStdString(), 0.4f },
            { C::xmlKeyEnv1Sustain().toStdString(), 0.2f },
            { C::xmlKeyEnv1Release().toStdString(), 0.25f },
            { C::xmlKeyEnv2Attack().toStdString(), 0.01f },
            { C::xmlKeyEnv2Decay().toStdString(), 0.5f },
            { C::xmlKeyEnv2Sustain().toStdString(), 0.8f },
            { C::xmlKeyEnv2Release().toStdString(), 0.3f },
            { C::xmlKeyDelayLevel().toStdString(), 0.3f },
            { C::xmlKeyDelayTime().toStdString(), 0.375f },
            { C::xmlKeyDelayFeedback().toStdString(), 0.4f },
          },
        },
        {
          .name = "Cross Mod Bell",
          .parameters = {
            { C::xmlKeyVco1Range().toStdString(), 2.0f }, // 4'
            { C::xmlKeyVco1Waveform().toStdString(), 3.0f }, // Sine
            { C::xmlKeyVco1Level().toStdString(), 0.8f },
            { C::xmlKeyVco2Range().toStdString(), 1.0f }, // 8'
            { C::xmlKeyVco2Waveform().toStdString(), 2.0f }, // Triangle
            { C::xmlKeyVco2FineTune().toStdString(), 0.22f },
            { C::xmlKeyVco2Level().toStdString(), 0.9f },
            { C::xmlKeyCrossMod().toStdString(), 0.55f }, // Cross mod FM depth
            { C::xmlKeyVcfSlope().toStdString(), 0.0f }, // 12 dB/oct
            { C::xmlKeyLpfCutoff().toStdString(), 0.85f },
            { C::xmlKeyLpfResonance().toStdString(), 0.1f },
            { C::xmlKeyEnvMod().toStdString(), 0.2f },
            { C::xmlKeyEnv1Attack().toStdString(), 0.01f },
            { C::xmlKeyEnv1Decay().toStdString(), 0.65f },
            { C::xmlKeyEnv1Sustain().toStdString(), 0.0f },
            { C::xmlKeyEnv1Release().toStdString(), 0.6f },
            { C::xmlKeyEnv2Attack().toStdString(), 0.01f },
            { C::xmlKeyEnv2Decay().toStdString(), 0.7f },
            { C::xmlKeyEnv2Sustain().toStdString(), 0.0f },
            { C::xmlKeyEnv2Release().toStdString(), 0.65f },
            { C::xmlKeyDelayLevel().toStdString(), 0.35f },
            { C::xmlKeyDelayTime().toStdString(), 0.4f },
            { C::xmlKeyDelayFeedback().toStdString(), 0.45f },
          },
        },
        {
          .name = "Jupiter Brass",
          .parameters = {
            { C::xmlKeyVco1Range().toStdString(), 1.0f }, // 8'
            { C::xmlKeyVco1Waveform().toStdString(), 1.0f }, // Saw
            { C::xmlKeyVco1Level().toStdString(), 0.9f },
            { C::xmlKeyVco2Range().toStdString(), 1.0f }, // 8'
            { C::xmlKeyVco2Waveform().toStdString(), 0.0f }, // Saw
            { C::xmlKeyVco2FineTune().toStdString(), 0.08f },
            { C::xmlKeyVco2Level().toStdString(), 0.9f },
            { C::xmlKeyVcfSlope().toStdString(), 1.0f }, // 24 dB/oct punch
            { C::xmlKeyLpfCutoff().toStdString(), 0.35f },
            { C::xmlKeyLpfResonance().toStdString(), 0.25f },
            { C::xmlKeyEnvMod().toStdString(), 0.65f },
            { C::xmlKeyKeyTrack().toStdString(), 0.75f },
            { C::xmlKeyEnv1Attack().toStdString(), 0.12f },
            { C::xmlKeyEnv1Decay().toStdString(), 0.5f },
            { C::xmlKeyEnv1Sustain().toStdString(), 0.5f },
            { C::xmlKeyEnv1Release().toStdString(), 0.35f },
            { C::xmlKeyEnv2Attack().toStdString(), 0.08f },
            { C::xmlKeyEnv2Decay().toStdString(), 0.55f },
            { C::xmlKeyEnv2Sustain().toStdString(), 0.75f },
            { C::xmlKeyEnv2Release().toStdString(), 0.35f },
            { C::xmlKeyPanSpread().toStdString(), 0.5f },
          },
        },
        {
          .name = "Unison Monster Bass",
          .parameters = {
            { C::xmlKeyVoiceMode().toStdString(), 2.0f }, // Unison
            { C::xmlKeyUnisonDetune().toStdString(), 0.35f },
            { C::xmlKeyVco1Range().toStdString(), 0.0f }, // 16'
            { C::xmlKeyVco1Waveform().toStdString(), 1.0f }, // Saw
            { C::xmlKeyVco1Level().toStdString(), 1.0f },
            { C::xmlKeyVco2Range().toStdString(), 0.0f }, // 16'
            { C::xmlKeyVco2Waveform().toStdString(), 1.0f }, // Square
            { C::xmlKeyVco2FineTune().toStdString(), 0.15f },
            { C::xmlKeyVco2Level().toStdString(), 0.9f },
            { C::xmlKeyPulseWidth().toStdString(), 0.5f },
            { C::xmlKeyVcfSlope().toStdString(), 1.0f }, // 24 dB/oct
            { C::xmlKeyLpfCutoff().toStdString(), 0.4f },
            { C::xmlKeyLpfResonance().toStdString(), 0.35f },
            { C::xmlKeyEnvMod().toStdString(), 0.55f },
            { C::xmlKeyEnv1Attack().toStdString(), 0.01f },
            { C::xmlKeyEnv1Decay().toStdString(), 0.45f },
            { C::xmlKeyEnv1Sustain().toStdString(), 0.15f },
            { C::xmlKeyEnv1Release().toStdString(), 0.2f },
            { C::xmlKeyEnv2Attack().toStdString(), 0.01f },
            { C::xmlKeyEnv2Decay().toStdString(), 0.5f },
            { C::xmlKeyEnv2Sustain().toStdString(), 0.8f },
            { C::xmlKeyEnv2Release().toStdString(), 0.2f },
          },
        },
        {
          .name = "Reso Sweep Pad",
          .parameters = {
            { C::xmlKeyVco1Range().toStdString(), 1.0f }, // 8'
            { C::xmlKeyVco1Waveform().toStdString(), 2.0f }, // Square
            { C::xmlKeyVco1Level().toStdString(), 0.85f },
            { C::xmlKeyPwmSource().toStdString(), 1.0f }, // LFO PWM
            { C::xmlKeyPulseWidth().toStdString(), 0.6f },
            { C::xmlKeyVco2Range().toStdString(), 1.0f }, // 8'
            { C::xmlKeyVco2Waveform().toStdString(), 0.0f }, // Saw
            { C::xmlKeyVco2FineTune().toStdString(), 0.1f },
            { C::xmlKeyVco2Level().toStdString(), 0.85f },
            { C::xmlKeyVcfSlope().toStdString(), 1.0f }, // 24 dB/oct
            { C::xmlKeyLpfCutoff().toStdString(), 0.3f },
            { C::xmlKeyLpfResonance().toStdString(), 0.65f },
            { C::xmlKeyFilterLfoDepth().toStdString(), 0.45f },
            { C::xmlKeyLfoRate().toStdString(), 0.2f },
            { C::xmlKeyEnvMod().toStdString(), 0.4f },
            { C::xmlKeyEnv1Attack().toStdString(), 0.45f },
            { C::xmlKeyEnv1Decay().toStdString(), 0.7f },
            { C::xmlKeyEnv1Sustain().toStdString(), 0.5f },
            { C::xmlKeyEnv1Release().toStdString(), 0.6f },
            { C::xmlKeyEnv2Attack().toStdString(), 0.35f },
            { C::xmlKeyEnv2Decay().toStdString(), 0.6f },
            { C::xmlKeyEnv2Sustain().toStdString(), 0.8f },
            { C::xmlKeyEnv2Release().toStdString(), 0.6f },
            { C::xmlKeyPanSpread().toStdString(), 0.7f },
            { C::xmlKeyDelayLevel().toStdString(), 0.3f },
            { C::xmlKeyDelayTime().toStdString(), 0.45f },
            { C::xmlKeyDelayFeedback().toStdString(), 0.4f },
          },
        },
    };
    return Presets;
}

} // namespace noteahead
