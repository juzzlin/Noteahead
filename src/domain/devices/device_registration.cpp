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

#include "../../plugins/devices/bass_synth/bass_synth_device.hpp"
#include "../../plugins/devices/drum_synth/drum_synth_device.hpp"
#include "../../plugins/devices/drum_synth_v2/drum_synth_v2_device.hpp"
#include "../../plugins/devices/electric_bass/electric_bass_device.hpp"
#include "../../plugins/devices/fm_drums/fm_drums_device.hpp"
#include "../../plugins/devices/fm_synth/fm_synth_device.hpp"
#include "../../plugins/devices/ju_synth/ju_synth_device.hpp"
#include "../../plugins/devices/ju_synth_08/ju_synth_08_device.hpp"
#include "../../plugins/devices/kick_808/kick_808_device.hpp"
#include "../../plugins/devices/kick_909/kick_909_device.hpp"
#include "../../plugins/devices/kick_synth/kick_synth_device.hpp"
#include "../../plugins/devices/lfo/lfo_device.hpp"
#include "../../plugins/devices/noise_fx/noise_fx_device.hpp"
#include "../../plugins/devices/piano_synth/piano_synth_device.hpp"
#include "../../plugins/devices/piano_synth_acoustic/piano_synth_acoustic_device.hpp"
#include "../../plugins/devices/piano_synth_v2/piano_synth_v2_device.hpp"
#include "../../plugins/devices/piano_synth_v3/piano_synth_v3_device.hpp"
#include "../../plugins/devices/sampler/sampler_device.hpp"
#include "../../plugins/devices/snare_synth/snare_synth_device.hpp"
#include "../../plugins/devices/speech/speech_device.hpp"
#include "../../plugins/devices/string_ensemble/string_ensemble_device.hpp"
#include "../../plugins/devices/string_voice/string_voice_device.hpp"
#include "../../plugins/devices/string_voice_v2/string_voice_v2_device.hpp"
#include "../../plugins/devices/string_voice_v3/string_voice_v3_device.hpp"
#include "../../plugins/devices/sub_mixer/sub_mixer_device.hpp"
#include "../../plugins/devices/synth/synth_device.hpp"
#include "../../plugins/devices/synth_d/synth_d_device.hpp"
#include "../../plugins/devices/wavetable_synth/wavetable_synth_device.hpp"
#include "../../plugins/devices/xy_pad/xy_pad_device.hpp"
#include "device_factory.hpp"

namespace noteahead {

void DeviceFactory::init()
{
    registerDevice(BassSynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<BassSynthDevice>(name);
    });
    registerDevice(DrumSynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<DrumSynthDevice>(name);
    });

    registerDevice(DrumSynthV2Device::typeIdString(), [](const std::string & name) {
        return std::make_shared<DrumSynthV2Device>(name);
    });
    registerDevice(FmDrumsDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<FmDrumsDevice>(name);
    });
    registerDevice(ElectricBassDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<ElectricBassDevice>(name);
    });
    registerDevice(FmSynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<FmSynthDevice>(name);
    });
    registerDevice(JuSynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<JuSynthDevice>(name);
    });
    registerDevice(Kick808Device::typeIdString(), [](const std::string & name) {
        return std::make_shared<Kick808Device>(name);
    });
    registerDevice(Kick909Device::typeIdString(), [](const std::string & name) {
        return std::make_shared<Kick909Device>(name);
    });
    registerDevice(KickSynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<KickSynthDevice>(name);
    });
    registerDevice(LfoDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<LfoDevice>(name);
    });
    registerDevice(NoiseFxDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<NoiseFxDevice>(name);
    });
    registerDevice(SamplerDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<SamplerDevice>(name);
    });
    registerDevice(SynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<SynthDevice>(name);
    });
    registerDevice(JuSynth08Device::typeIdString(), [](const std::string & name) {
        return std::make_shared<JuSynth08Device>(name);
    });
    registerDevice(WavetableSynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<WavetableSynthDevice>(name);
    });
    registerDevice(XyPadDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<XyPadDevice>(name);
    });
    registerDevice(PianoSynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<PianoSynthDevice>(name);
    });
    registerDevice(PianoSynthV2Device::typeIdString(), [](const std::string & name) {
        return std::make_shared<PianoSynthV2Device>(name);
    });
    registerDevice(PianoSynthV3Device::typeIdString(), [](const std::string & name) {
        return std::make_shared<PianoSynthV3Device>(name);
    });
    registerDevice(PianoSynthAcousticDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<PianoSynthAcousticDevice>(name);
    });
    registerDevice(SynthDDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<SynthDDevice>(name);
    });
    registerDevice(SubMixerDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<SubMixerDevice>(name);
    });
    registerDevice(StringVoiceDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<StringVoiceDevice>(name);
    });
    registerDevice(StringVoiceV2Device::typeIdString(), [](const std::string & name) {
        return std::make_shared<StringVoiceV2Device>(name);
    });
    registerDevice(StringVoiceV3Device::typeIdString(), [](const std::string & name) {
        return std::make_shared<StringVoiceV3Device>(name);
    });
    registerDevice(StringEnsembleDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<StringEnsembleDevice>(name);
    });
    registerDevice(SpeechDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<SpeechDevice>(name);
    });
    registerDevice(SnareSynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<SnareSynthDevice>(name);
    });
}

} // namespace noteahead
