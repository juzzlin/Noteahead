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

#include "gain_staging_service_test.hpp"

#include "../../application/service/device_service.hpp"
#include "../../application/service/gain_staging_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../domain/utility/level_meter.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"
#include "../../plugins/devices/sub_mixer/sub_mixer_device.hpp"

#include <QSignalSpy>
#include <QTest>

#include <cmath>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate { 48000 };
constexpr uint32_t FrameCount { 480 };

class StubSettingsService : public SettingsService
{
public:
    int gainStagingTargetDb() const override
    {
        return -18;
    }

    int gainStagingSpeedPercent() const override
    {
        return 20;
    }
};

double gainDb(const Device & device)
{
    return (static_cast<double>(device.gain()) - 0.5) * 60.0;
}

struct Fixture
{
    Fixture()
      : deviceService { std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>()) }
      , device { std::make_shared<SubMixerDevice>("Sub") }
      , service { deviceService, std::make_shared<StubSettingsService>() }
    {
        deviceService->setDevice(0, device);
        device->meter().setActive(true);
    }

    //! A step's worth of the device playing a constant source, as the engine meters it: after Gain
    void play(double sourceDb, int buffers = 10)
    {
        const double gain = ParameterMapper::mapDecibel(device->gain(), 30.0);
        const std::vector<double> samples(static_cast<size_t>(FrameCount) * 2, std::pow(10.0, sourceDb / 20.0) * gain);
        for (int i = 0; i < buffers; i++) {
            device->meter().write(samples.data(), FrameCount, SampleRate, static_cast<float>(gain));
        }
    }

    void run(double sourceDb, int steps)
    {
        for (int i = 0; i < steps; i++) {
            play(sourceDb);
            service.step();
        }
    }

    std::shared_ptr<DeviceService> deviceService;
    std::shared_ptr<SubMixerDevice> device;
    GainStagingService service;
};

} // namespace

void GainStagingServiceTest::test_step_rms_shouldConvergeToTheTarget()
{
    Fixture fixture;
    fixture.device->setGainStagingMode(Device::GainStagingMode::Rms);
    fixture.service.start(0);
    fixture.run(-30.0, 80);

    // A -30 dBFS source needs +12 dB to read -18
    QVERIFY2(std::abs(gainDb(*fixture.device) - 12.0) < 0.1, qPrintable(QString::number(gainDb(*fixture.device))));
}

void GainStagingServiceTest::test_step_trim_shouldShiftTheTarget()
{
    Fixture fixture;
    fixture.device->setGainStagingMode(Device::GainStagingMode::Rms);
    fixture.device->setGainStagingTrimDb(3.0);
    fixture.service.start(0);
    fixture.run(-30.0, 80);

    QVERIFY2(std::abs(gainDb(*fixture.device) - 15.0) < 0.1, qPrintable(QString::number(gainDb(*fixture.device))));
}

void GainStagingServiceTest::test_step_peak_shouldSettleOnTheLoudestHitOfTheSession()
{
    Fixture fixture;
    QCOMPARE(fixture.device->gainStagingMode(), Device::GainStagingMode::Peak);
    fixture.service.start(0);
    // One -6 dBFS hit, then a long quiet stretch that must not pull the gain up
    fixture.play(-6.0, 1);
    fixture.service.step();
    fixture.run(-26.0, 80);

    QVERIFY2(std::abs(gainDb(*fixture.device) + 12.0) < 0.1, qPrintable(QString::number(gainDb(*fixture.device))));
}

void GainStagingServiceTest::test_step_silence_shouldLeaveGainAlone()
{
    Fixture fixture;
    fixture.service.start(0);
    for (int i = 0; i < 20; i++) {
        fixture.service.step();
    }

    QCOMPARE(fixture.device->gain(), 0.5f);
    QVERIFY(!fixture.service.stop());
}

void GainStagingServiceTest::test_step_quietSource_shouldClampGainAtTheTopOfItsRange()
{
    Fixture fixture;
    fixture.service.start(0);
    fixture.run(-70.0, 80);

    QCOMPARE(fixture.device->gain(), 1.0f);
}

void GainStagingServiceTest::test_start_shouldForgetWhatPlayedBeforeThePress()
{
    Fixture fixture;
    fixture.device->setGainStagingMode(Device::GainStagingMode::Rms);
    fixture.play(0.0);
    fixture.service.start(0);
    fixture.run(-30.0, 80);

    QVERIFY2(std::abs(gainDb(*fixture.device) - 12.0) < 0.1, qPrintable(QString::number(gainDb(*fixture.device))));
}

void GainStagingServiceTest::test_stop_shouldEndTheSessionAndReportWhetherGainMoved()
{
    Fixture fixture;
    QSignalSpy spy { &fixture.service, &GainStagingService::activeSlotChanged };
    fixture.service.start(0);
    QCOMPARE(fixture.service.activeSlot(), std::optional<size_t> { 0 });
    fixture.run(-30.0, 5);

    QVERIFY(fixture.service.stop());
    QVERIFY(!fixture.service.activeSlot());
    QCOMPARE(spy.count(), 2);

    // Gain stays where the session left it, and a further step does nothing
    const float gain = fixture.device->gain();
    fixture.play(-30.0);
    fixture.service.step();
    QCOMPARE(fixture.device->gain(), gain);

    // An empty slot has nothing to stage
    fixture.service.start(5);
    QVERIFY(!fixture.service.activeSlot());
}

void GainStagingServiceTest::test_trimAndMode_shouldRoundTripThroughXml()
{
    SubMixerDevice device { "Sub" };
    device.setGainStagingTrimDb(-4.5);
    device.setGainStagingMode(Device::GainStagingMode::Rms);

    QString xml;
    {
        NahdXmlWriter writer { xml };
        device.serializeToXml(writer);
    }

    SubMixerDevice restored { "Sub" };
    NahdXmlReader reader { xml };
    reader.readNextStartElement();
    restored.deserializeFromXml(reader);

    QVERIFY(std::abs(restored.gainStagingTrimDb() + 4.5) < 1.0e-3);
    QCOMPARE(restored.gainStagingMode(), Device::GainStagingMode::Rms);

    // Out of range is held at the edge
    restored.setGainStagingTrimDb(10.0);
    QCOMPARE(restored.gainStagingTrimDb(), 6.0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::GainStagingServiceTest)
