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

#include "midi_service_test.hpp"

#include "../../application/instrument_request.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/midi_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device_factory.hpp"
#include "../../domain/devices/drum_synth_constants.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"

#include <QTest>

namespace noteahead {

namespace {

//! MidiService without the RtMidi workers: the internal device path never touches them.
class TestMidiService : public MidiService
{
public:
    explicit TestMidiService(DeviceServiceS deviceService)
      : MidiService { std::move(deviceService), nullptr, false }
    {
    }
};

//! CC of the kick's HPF cutoff, the setting that stayed unapplied on rewind.
constexpr uint8_t kickHpfCc()
{
    return DrumSynth::CcStartRange1 + (static_cast<int>(DrumSynth::VoiceIndex::Kick) * 3) + 2;
}

struct TestSetup
{
    std::shared_ptr<DeviceService> deviceService;
    std::shared_ptr<DrumSynthDevice> device;
    std::unique_ptr<TestMidiService> midiService;
    Instrument instrument { Constants::internalDevicePortPrefix() + " 1" };
};

TestSetup createSetup()
{
    TestSetup setup;
    setup.deviceService = std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>());
    setup.device = std::make_shared<DrumSynthDevice>("Drum Synth 1");
    setup.deviceService->setDevice(0, setup.device);
    setup.midiService = std::make_unique<TestMidiService>(setup.deviceService);
    return setup;
}

float kickHpfCutoff(const DrumSynthDevice & device)
{
    const auto name = DrumSynth::voiceId(static_cast<int>(DrumSynth::VoiceIndex::Kick)) + "_" + Constants::NahdXml::xmlKeyHpfCutoff().toStdString();
    const auto parameter = device.parameter(name);
    return parameter.has_value() ? parameter->get().value() : -1;
}

InstrumentSettings settingsWithKickHpf(bool enabled, uint32_t value)
{
    InstrumentSettings settings;
    settings.midiCcSettings.push_back({ enabled, kickHpfCc(), value });
    return settings;
}

} // namespace

void MidiServiceTest::initTestCase()
{
    EffectFactory::init();
    DeviceFactory::init();
}

void MidiServiceTest::cleanupTestCase()
{
    EffectFactory::clear();
    DeviceFactory::clear();
}

void MidiServiceTest::test_instrumentRequest_internalDevice_applyAll_shouldApplyMidiCcSettings()
{
    // Rewinding the song applies all track settings as ApplyAll requests. Internal devices used to
    // be skipped altogether, so a MIDI CC set in the track settings dialog never reached them.
    auto setup = createSetup();
    setup.instrument.setSettings(settingsWithKickHpf(true, 64));

    setup.midiService->handleInstrumentRequest({ InstrumentRequest::Type::ApplyAll, setup.instrument });

    QVERIFY(qFuzzyCompare(kickHpfCutoff(*setup.device), 64.0f / 127.0f));
}

void MidiServiceTest::test_instrumentRequest_internalDevice_applyMidiCc_shouldApplyMidiCcSettings()
{
    auto setup = createSetup();
    setup.instrument.setSettings(settingsWithKickHpf(true, 127));

    setup.midiService->handleInstrumentRequest({ InstrumentRequest::Type::ApplyMidiCc, setup.instrument });

    QVERIFY(qFuzzyCompare(kickHpfCutoff(*setup.device), 1.0f));
}

void MidiServiceTest::test_instrumentRequest_internalDevice_disabledSetting_shouldNotApplyMidiCcSetting()
{
    auto setup = createSetup();
    const auto originalValue = kickHpfCutoff(*setup.device);
    setup.instrument.setSettings(settingsWithKickHpf(false, 64));

    setup.midiService->handleInstrumentRequest({ InstrumentRequest::Type::ApplyAll, setup.instrument });

    QVERIFY(qFuzzyCompare(kickHpfCutoff(*setup.device), originalValue));
}

void MidiServiceTest::test_instrumentRequest_internalDevice_applyPatch_shouldNotApplyMidiCcSettings()
{
    auto setup = createSetup();
    const auto originalValue = kickHpfCutoff(*setup.device);
    setup.instrument.setSettings(settingsWithKickHpf(true, 64));

    setup.midiService->handleInstrumentRequest({ InstrumentRequest::Type::ApplyPatch, setup.instrument });

    QVERIFY(qFuzzyCompare(kickHpfCutoff(*setup.device), originalValue));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MidiServiceTest)
