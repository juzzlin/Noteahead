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

#include "noise_fx_controller_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/bass_synth_device.hpp"
#include "../../domain/devices/noise_fx_device.hpp"
#include "../../domain/devices/noise_fx_presets.hpp"
#include "../../view/controllers/noise_fx_controller.hpp"
#include "../../view/controllers/noise_fx_sweep_controller.hpp"

#include <QMetaProperty>
#include <QSignalSpy>
#include <QTest>

#include <cmath>
#include <memory>
#include <vector>

namespace noteahead {

namespace NahdXml = Constants::NahdXml;

namespace {

NoiseFxSweepController * sweepPanel(const NoiseFxController & controller, int index)
{
    return qobject_cast<NoiseFxSweepController *>(controller.sweeps().at(index).value<QObject *>());
}

} // namespace

void NoiseFxControllerTest::test_requestSettings_shouldNotifyEveryProperty()
{
    // Stated over the metaobject, so a property added without its line in requestSettings() fails
    // here instead of leaving a knob that never refreshes.
    NoiseFxController controller { std::make_shared<NoiseFxDevice>("Noise FX") };
    const auto * meta = controller.metaObject();
    std::vector<std::unique_ptr<QSignalSpy>> spies;
    std::vector<QString> names;
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); i++) {
        const auto property = meta->property(i);
        if (!property.hasNotifySignal() || !property.isWritable()) {
            continue;
        }
        spies.push_back(std::make_unique<QSignalSpy>(&controller, property.notifySignal()));
        names.push_back(QString::fromLatin1(property.name()));
    }
    QVERIFY(spies.size() > 25);
    std::vector<std::unique_ptr<QSignalSpy>> sweepSpies;
    for (int sweep = 0; sweep < static_cast<int>(NoiseFxDevice::SweepCount); sweep++) {
        sweepSpies.push_back(std::make_unique<QSignalSpy>(sweepPanel(controller, sweep), &NoiseFxSweepController::changed));
    }

    controller.requestSettings();

    for (size_t i = 0; i < spies.size(); i++) {
        QVERIFY2(spies.at(i)->count() > 0, qPrintable(names.at(i) + " is never notified"));
    }
    for (auto && spy : sweepSpies) {
        QVERIFY(spy->count() > 0);
    }
}

void NoiseFxControllerTest::test_properties_shouldReachTheDevice()
{
    const auto device = std::make_shared<NoiseFxDevice>("Noise FX");
    NoiseFxController controller { device };
    const auto scaling = Constants::uiInternalScaling();

    controller.setCutoff(static_cast<int>(0.4 * scaling));
    controller.setColour(static_cast<int>(0.25 * scaling));
    controller.setFilterType(static_cast<int>(NoiseFxDevice::FilterType::HighPass));
    controller.setToneWaveform(static_cast<int>(PolyBlepOscillator::Waveform::Square));
    controller.setPlayToEnd(false);
    controller.setChromatic(true);
    controller.setLfoSync(true);
    controller.setLfoTarget2(static_cast<int>(NoiseFxDevice::Destination::Pan));
    controller.setLfoDepth2(static_cast<int>(0.8 * scaling));
    controller.setGateRate(5);
    controller.setGateDepth(static_cast<int>(0.6 * scaling));

    QVERIFY(std::abs(device->cutoff() - 0.4f) < 0.001f);
    QVERIFY(std::abs(device->colour() - 0.25f) < 0.001f);
    QCOMPARE(device->filterType(), NoiseFxDevice::FilterType::HighPass);
    QCOMPARE(device->toneWaveform(), PolyBlepOscillator::Waveform::Square);
    QVERIFY(!device->playToEnd());
    QVERIFY(device->chromatic());
    QCOMPARE(device->lfoParameterValue(NahdXml::xmlKeySync()), 1.0f);
    QCOMPARE(device->lfoTarget(1), NoiseFxDevice::Destination::Pan);
    QVERIFY(std::abs(device->lfoDepth(1) - 0.8f) < 0.001f);
    QCOMPARE(device->gateParameterValue(NahdXml::xmlKeyRate()), 5.0f);
    QVERIFY(std::abs(device->gateParameterValue(NahdXml::xmlKeyDepth()) - 0.6f) < 0.001f);
    QCOMPARE(controller.filterType(), static_cast<int>(NoiseFxDevice::FilterType::HighPass));
}

void NoiseFxControllerTest::test_sweeps_shouldReachTheirOwnSweep()
{
    const auto device = std::make_shared<NoiseFxDevice>("Noise FX");
    NoiseFxController controller { device };
    const auto scaling = Constants::uiInternalScaling();
    QCOMPARE(controller.sweeps().size(), static_cast<qsizetype>(NoiseFxDevice::SweepCount));

    auto * sweepB = sweepPanel(controller, 1);
    QVERIFY(sweepB);
    QCOMPARE(sweepB->index(), 1);
    sweepB->setStart(static_cast<int>(0.3 * scaling));
    sweepB->setMode(static_cast<int>(SweepEnvelope::Mode::PingPong));
    sweepB->setSync(false);
    sweepB->setTarget2(static_cast<int>(NoiseFxDevice::Destination::Blend));
    sweepB->setDepth2(static_cast<int>(0.2 * scaling));

    QVERIFY(std::abs(device->sweepParameterValue(1, NahdXml::xmlKeyStart()) - 0.3f) < 0.001f);
    QCOMPARE(device->sweepParameterValue(1, NahdXml::xmlKeyMode()), static_cast<float>(SweepEnvelope::Mode::PingPong));
    QCOMPARE(device->sweepParameterValue(1, NahdXml::xmlKeySync()), 0.0f);
    QCOMPARE(device->sweepTarget(1, 1), NoiseFxDevice::Destination::Blend);
    QVERIFY(std::abs(device->sweepDepth(1, 1) - 0.2f) < 0.001f);
    QVERIFY(!sweepB->sync());

    // Sweep A is left as it was
    QCOMPARE(device->sweepParameterValue(0, NahdXml::xmlKeyStart()), 0.0f);
    QCOMPARE(device->sweepTarget(0, 1), NoiseFxDevice::Destination::None);
}

void NoiseFxControllerTest::test_lfoSyncDivision_shouldPickTheDivision()
{
    const auto device = std::make_shared<NoiseFxDevice>("Noise FX");
    NoiseFxController controller { device };
    const auto last = static_cast<int>(NoiseFxDevice::lfoSyncBars().size()) - 1;

    for (int division = 0; division <= last; division++) {
        controller.setLfoSyncDivision(division);
        QCOMPARE(controller.lfoSyncDivision(), division);
    }
    QCOMPARE(controller.lfoSyncNames().size(), static_cast<qsizetype>(last + 1));
}

void NoiseFxControllerTest::test_divisionNames_shouldReadAsNoteValues()
{
    NoiseFxController controller { std::make_shared<NoiseFxDevice>("Noise FX") };

    const auto syncLengths = controller.syncLengthNames();
    QCOMPARE(syncLengths.size(), static_cast<qsizetype>(NoiseFxDevice::syncLengthBeats().size()));
    QCOMPARE(syncLengths.front(), QString { "1/16" });
    QCOMPARE(syncLengths.at(4), QString { "1/1" });
    QCOMPARE(syncLengths.back(), QString { "32/1" });

    const auto gateDivisions = controller.gateDivisionNames();
    QCOMPARE(gateDivisions.front(), QString { "1/1" });
    QCOMPARE(gateDivisions.at(3), QString { "1/8" });
    QCOMPARE(gateDivisions.back(), QString { "1/64" });

    const auto lfoDivisions = controller.lfoSyncNames();
    QCOMPARE(lfoDivisions.front(), QString { "4/1" });
    QCOMPARE(lfoDivisions.back(), QString { "1/32" });
}

void NoiseFxControllerTest::test_factoryPresetNames_shouldListThePresets()
{
    NoiseFxController controller { std::make_shared<NoiseFxDevice>("Noise FX") };
    const auto names = controller.factoryPresetNames();
    QCOMPARE(names.size(), static_cast<qsizetype>(NoiseFxPresets::presets().size()));
    QCOMPARE(names.front(), QString { "Init" });
}

void NoiseFxControllerTest::test_setDevice_otherType_shouldRefuse()
{
    NoiseFxController controller { std::make_shared<NoiseFxDevice>("Noise FX") };
    QVERIFY(!controller.setDevice(std::static_pointer_cast<Device>(std::make_shared<BassSynthDevice>("Bass"))));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::NoiseFxControllerTest)
