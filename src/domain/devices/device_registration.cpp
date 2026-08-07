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

#include "bass_synth_device.hpp"
#include "device_factory.hpp"
#include "drum_synth_device.hpp"
#include "kick_808_device.hpp"
#include "piano_synth_device.hpp"
#include "piano_synth_v2_device.hpp"
#include "sampler_device.hpp"
#include "string_ensemble_device.hpp"
#include "string_voice_device.hpp"
#include "sub_mixer_device.hpp"
#include "synth_device.hpp"
#include "wavetable_synth_device.hpp"

namespace noteahead {

void DeviceFactory::init()
{
    registerDevice(BassSynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<BassSynthDevice>(name);
    });
    registerDevice(DrumSynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<DrumSynthDevice>(name);
    });
    registerDevice(Kick808Device::typeIdString(), [](const std::string & name) {
        return std::make_shared<Kick808Device>(name);
    });
    registerDevice(SamplerDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<SamplerDevice>(name);
    });
    registerDevice(SynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<SynthDevice>(name);
    });
    registerDevice(WavetableSynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<WavetableSynthDevice>(name);
    });
    registerDevice(PianoSynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<PianoSynthDevice>(name);
    });
    registerDevice(PianoSynthV2Device::typeIdString(), [](const std::string & name) {
        return std::make_shared<PianoSynthV2Device>(name);
    });
    registerDevice(SubMixerDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<SubMixerDevice>(name);
    });
    registerDevice(StringVoiceDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<StringVoiceDevice>(name);
    });
    registerDevice(StringEnsembleDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<StringEnsembleDevice>(name);
    });
}

} // namespace noteahead
