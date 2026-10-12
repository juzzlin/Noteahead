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

#include "fm_drums_controller_test.hpp"

#include "../../application/service/device_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/drum_synth_v2_device.hpp"
#include "../../domain/devices/fm_drums_device.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../view/controllers/fm_drum_operator_controller.hpp"
#include "../../view/controllers/fm_drums_controller.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

namespace {

namespace NahdXml = Constants::NahdXml;

constexpr int Kick { static_cast<int>(DrumSynthV2::VoiceIndex::Kick) };
constexpr int Snare { static_cast<int>(DrumSynthV2::VoiceIndex::Snare) };

struct Fixture
{
    std::shared_ptr<DeviceService> deviceService { std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>()) };
    std::shared_ptr<FmDrumsDevice> device { std::make_shared<FmDrumsDevice>(Constants::fmDrumsDeviceName().toStdString()) };
    FmDrumsController controller { deviceService };

    Fixture()
    {
        deviceService->setDevice(0, device);
        controller.setDevice(device);
    }

    FmDrumOperatorController & op(int index)
    {
        return *qobject_cast<FmDrumOperatorController *>(controller.operators().at(index).value<QObject *>());
    }
};

} // namespace

void FmDrumsControllerTest::test_setDevice_otherDrumMachine_shouldBeRefused()
{
    // Both are pad devices, but the FM controls mean nothing on V2's pads
    FmDrumsController controller { nullptr };

    QVERIFY(!controller.setDevice(std::make_shared<DrumSynthV2Device>("Drum Synth V2")));
    QVERIFY(!controller.device());
    QVERIFY(controller.setDevice(std::make_shared<FmDrumsDevice>("FM Drums")));
}

void FmDrumsControllerTest::test_knob_write_shouldReachTheSelectedPadOnly()
{
    Fixture fixture;
    fixture.controller.setSelectedVoice(Snare);
    const float kickPitch = fixture.device->voiceParameterValue(Kick, NahdXml::xmlKeyFrequency().toStdString());

    fixture.controller.setPitch(250);

    QCOMPARE(fixture.device->voiceParameterValue(Snare, NahdXml::xmlKeyFrequency().toStdString()), 0.25f);
    QCOMPARE(fixture.device->voiceParameterValue(Kick, NahdXml::xmlKeyFrequency().toStdString()), kickPitch);
    QCOMPARE(fixture.controller.pitch(), 250);
}

void FmDrumsControllerTest::test_choice_write_shouldStoreThePlainIndex()
{
    Fixture fixture;
    fixture.controller.setSelectedVoice(Snare);

    fixture.controller.setNoiseFilterMode(2);
    fixture.controller.setAlgorithm(6);
    fixture.controller.setNoiseTarget(3);

    QCOMPARE(fixture.device->voiceParameterValue(Snare, NahdXml::xmlKeyNoiseFilterMode().toStdString()), 2.0f);
    QCOMPARE(fixture.controller.noiseFilterMode(), 2);
    QCOMPARE(fixture.controller.algorithm(), 6);
    QCOMPARE(fixture.controller.noiseTarget(), 3);
}

void FmDrumsControllerTest::test_selectedVoice_change_shouldFollowTheNewPad()
{
    Fixture fixture;
    fixture.controller.setSelectedVoice(Kick);
    const int kickPitchDepth = fixture.controller.pitchDepth();
    QSignalSpy padSpy { &fixture.controller, &FmDrumsController::padChanged };

    fixture.controller.setSelectedVoice(Snare);

    QVERIFY(padSpy.count() > 0);
    QVERIFY(fixture.controller.pitchDepth() != kickPitchDepth);
    QCOMPARE(fixture.controller.pitchDepth(), static_cast<int>(std::round(fixture.device->voiceParameterValue(Snare, NahdXml::xmlKeyPitchDepth().toStdString()) * Constants::uiInternalScaling())));
}

void FmDrumsControllerTest::test_operator_write_shouldReachTheSelectedPad()
{
    Fixture fixture;
    fixture.controller.setSelectedVoice(Snare);

    fixture.op(2).setRatio(600);
    fixture.op(1).setWaveform(5);

    QCOMPARE(fixture.device->voiceParameterValue(Snare, NahdXml::xmlKeyOperatorRatio(2).toStdString()), 0.6f);
    QCOMPARE(fixture.device->voiceParameterValue(Snare, NahdXml::xmlKeyOperatorWaveform(1).toStdString()), 5.0f);
    QCOMPARE(fixture.op(2).ratio(), 600);
    QCOMPARE(fixture.op(1).waveform(), 5);
}

void FmDrumsControllerTest::test_operator_routing_shouldFollowTheAlgorithm()
{
    Fixture fixture;

    fixture.controller.setAlgorithm(0); // Serial: 4>3>2>1
    QVERIFY(fixture.op(0).carrier());
    QVERIFY(!fixture.op(1).carrier());
    QCOMPARE(fixture.op(0).modulatedBy(), QVariantList { 2 });

    fixture.controller.setAlgorithm(7); // Additive
    for (int op = 0; op < 4; op++) {
        QVERIFY(fixture.op(op).carrier());
        QVERIFY(fixture.op(op).modulatedBy().isEmpty());
    }
    QVERIFY(fixture.op(3).feedbackOperator());
    QVERIFY(!fixture.op(0).feedbackOperator());
}

void FmDrumsControllerTest::test_deviceChange_shouldNotifyTheOperators()
{
    // Automation, MIDI CC and a project load all arrive as a device change, not through a setter
    Fixture fixture;
    QSignalSpy operatorSpy { &fixture.op(0), &FmDrumOperatorController::changed };

    fixture.device->updateVoiceParameter(Kick, NahdXml::xmlKeyOperatorDecay(0).toStdString(), 0.1f);

    QVERIFY(operatorSpy.count() > 0);
    QCOMPARE(fixture.op(0).decay(), 100);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::FmDrumsControllerTest)
