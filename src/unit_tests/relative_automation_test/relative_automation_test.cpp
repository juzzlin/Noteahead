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

#include "relative_automation_test.hpp"

#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/tracker/parameter_data.hpp"
#include "../../domain/tracker/parameter_modulation.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../plugins/devices/bass_synth/bass_synth_device.hpp"
#include "../../plugins/devices/bass_synth/bass_synth_dialog_model.hpp"
#include "../../plugins/devices/drum_synth/drum_synth_device.hpp"
#include "../../plugins/devices/drum_synth/drum_synth_dialog_model.hpp"
#include "../../plugins/devices/drum_synth_v2/drum_synth_v2_device.hpp"
#include "../../plugins/devices/drum_synth_v2/drum_synth_v2_dialog_model.hpp"
#include "../../plugins/devices/fm_drums/fm_drums_device.hpp"
#include "../../plugins/devices/fm_drums/fm_drums_dialog_model.hpp"
#include "../../plugins/devices/fm_synth/fm_synth_device.hpp"
#include "../../plugins/devices/fm_synth/fm_synth_dialog_model.hpp"
#include "../../plugins/devices/ju_synth/ju_synth_device.hpp"
#include "../../plugins/devices/ju_synth/ju_synth_dialog_model.hpp"
#include "../../plugins/devices/kick_808/kick_808_device.hpp"
#include "../../plugins/devices/kick_808/kick_808_dialog_model.hpp"
#include "../../plugins/devices/noise_fx/noise_fx_device.hpp"
#include "../../plugins/devices/noise_fx/noise_fx_dialog_model.hpp"
#include "../../plugins/devices/piano_synth/piano_synth_device.hpp"
#include "../../plugins/devices/piano_synth/piano_synth_dialog_model.hpp"
#include "../../plugins/devices/piano_synth_v2/piano_synth_v2_device.hpp"
#include "../../plugins/devices/piano_synth_v2/piano_synth_v2_dialog_model.hpp"
#include "../../plugins/devices/piano_synth_v3/piano_synth_v3_device.hpp"
#include "../../plugins/devices/piano_synth_v3/piano_synth_v3_dialog_model.hpp"
#include "../../plugins/devices/sampler/sampler_device.hpp"
#include "../../plugins/devices/snare_synth/snare_synth_device.hpp"
#include "../../plugins/devices/snare_synth/snare_synth_dialog_model.hpp"
#include "../../plugins/devices/speech/speech_device.hpp"
#include "../../plugins/devices/speech/speech_dialog_model.hpp"
#include "../../plugins/devices/string_ensemble/string_ensemble_device.hpp"
#include "../../plugins/devices/string_ensemble/string_ensemble_dialog_model.hpp"
#include "../../plugins/devices/string_voice/string_voice_device.hpp"
#include "../../plugins/devices/string_voice/string_voice_dialog_model.hpp"
#include "../../plugins/devices/string_voice_v2/string_voice_v2_device.hpp"
#include "../../plugins/devices/string_voice_v2/string_voice_v2_dialog_model.hpp"
#include "../../plugins/devices/string_voice_v3/string_voice_v3_device.hpp"
#include "../../plugins/devices/string_voice_v3/string_voice_v3_dialog_model.hpp"
#include "../../plugins/devices/synth/synth_device.hpp"
#include "../../plugins/devices/synth/synth_dialog_model.hpp"
#include "../../plugins/devices/wavetable_synth/wavetable_synth_device.hpp"
#include "../../plugins/devices/wavetable_synth/wavetable_synth_dialog_model.hpp"
#include "../../plugins/effects/auto_filter/auto_filter.hpp"
#include "../../view/controllers/effect_rack_controller.hpp"

#include <QMetaProperty>
#include <QSignalSpy>
#include <QTest>

#include <functional>
#include <map>
#include <memory>

namespace noteahead {

namespace {

using Snapshot = std::map<QString, QVariant>;

//! Every number and switch a dialog reads from @p object, and from the objects it hands out in a
//! list -- the FM Synth's operators -- by name.
Snapshot snapshot(QObject * object, const QString & prefix = {})
{
    Snapshot values;
    const auto meta = object->metaObject();
    for (int i = QObject::staticMetaObject.propertyCount(); i < meta->propertyCount(); i++) {
        const auto property = meta->property(i);
        const auto value = property.read(object);
        const auto type = value.metaType().id();
        if (type == QMetaType::Int || type == QMetaType::Bool || type == QMetaType::Double || type == QMetaType::Float) {
            values[prefix + property.name()] = value;
        } else if (type == QMetaType::QVariantList) {
            const auto list = value.toList();
            for (int j = 0; j < list.size(); j++) {
                if (const auto child = list.at(j).value<QObject *>(); child) {
                    values.merge(snapshot(child, prefix + property.name() + QString::number(j) + "."));
                }
            }
        }
    }
    return values;
}

QStringList differences(const Snapshot & before, const Snapshot & after)
{
    QStringList moved;
    for (auto && [name, value] : before) {
        if (after.at(name) != value) {
            moved << QString { "%1: %2 -> %3" }.arg(name, value.toString(), after.at(name).toString());
        }
    }
    return moved;
}

struct Case
{
    QString name;
    std::shared_ptr<Device> device;
    std::function<std::unique_ptr<QObject>(std::shared_ptr<Device>)> controller;
};

template<typename DeviceType, typename ControllerType>
Case makeCase(const QString & name)
{
    return { name, std::make_shared<DeviceType>("Device"), [](std::shared_ptr<Device> device) {
                return std::make_unique<ControllerType>(std::dynamic_pointer_cast<DeviceType>(device));
            } };
}

template<typename DeviceType, typename ControllerType>
Case makeDrumCase(const QString & name)
{
    return { name, std::make_shared<DeviceType>("Device"), [](std::shared_ptr<Device> device) {
                auto controller = std::make_unique<ControllerType>(nullptr);
                controller->setDevice(device);
                return controller;
            } };
}

std::vector<Case> cases()
{
    return {
        makeCase<SynthDevice, SynthDialogModel>("Synth"),
        makeCase<WavetableSynthDevice, WavetableSynthDialogModel>("Wavetable Synth"),
        makeCase<FmSynthDevice, FmSynthDialogModel>("FM Synth"),
        makeCase<JuSynthDevice, JuSynthDialogModel>("JU Synth"),
        makeCase<BassSynthDevice, BassSynthDialogModel>("Bass Synth"),
        makeCase<Kick808Device, Kick808DialogModel>("Kick 808"),
        makeCase<SnareSynthDevice, SnareSynthDialogModel>("Snare Synth"),
        makeCase<PianoSynthDevice, PianoSynthDialogModel>("Piano Synth"),
        makeCase<PianoSynthV2Device, PianoSynthV2DialogModel>("Piano Synth V2"),
        makeCase<PianoSynthV3Device, PianoSynthV3DialogModel>("Piano Synth V3"),
        makeCase<NoiseFxDevice, NoiseFxDialogModel>("Noise FX"),
        makeCase<SpeechDevice, SpeechDialogModel>("Speech"),
        makeCase<StringEnsembleDevice, StringEnsembleDialogModel>("String Ensemble"),
        makeCase<StringVoiceDevice, StringVoiceDialogModel>("String & Voice"),
        makeCase<StringVoiceV2Device, StringVoiceV2DialogModel>("String & Voice V2"),
        makeCase<StringVoiceV3Device, StringVoiceV3DialogModel>("String & Voice V3"),
        makeDrumCase<DrumSynthDevice, DrumSynthDialogModel>("Drum Synth"),
        makeDrumCase<DrumSynthV2Device, DrumSynthV2DialogModel>("Drum Synth V2"),
        makeDrumCase<FmDrumsDevice, FmDrumsDialogModel>("FM Drums"),
    };
}

} // namespace

void RelativeAutomationTest::test_parameter_relativeAutomation_shouldShowTheAuthoredValue()
{
    Parameter parameter { "p", 0.4f, 0, 10000, 4000, 100 };
    parameter.setRelativeAutomationValue(0.2f);
    QCOMPARE(parameter.value(), 0.2f);
    QCOMPARE(parameter.shownValue(), 0.4f);
    parameter.setAutomationValue(0.9f);
    QCOMPARE(parameter.shownValue(), 0.9f);
    parameter.setRelativeAutomationValue(0.1f);
    parameter.clearAutomation();
    QCOMPARE(parameter.value(), 0.4f);
    QCOMPARE(parameter.shownValue(), 0.4f);
    QVERIFY(!parameter.isRelativelyAutomated());
}

void RelativeAutomationTest::test_deviceDialogs_relativeAutomation_shouldLeaveEveryKnobAlone()
{
    // A relative automation scales the value the knob shows rather than replacing it, so the knob is
    // the reference and stays where it was set, while the device itself does follow the automation.
    // Every automatable parameter of every instrument at once, read back through every number its
    // dialog shows.
    for (auto && testCase : cases()) {
        const auto controller = testCase.controller(testCase.device);
        const auto before = snapshot(controller.get());
        QVERIFY2(!before.empty(), qPrintable(testCase.name));
        int followed = 0;
        for (auto && entry : testCase.device->automatableParameters()) {
            const float authored = entry.parameter.value();
            QVERIFY(testCase.device->applyParameterAutomation(entry.key, ParameterAutomation::Mode::Relative, 0.3));
            if (const auto p = testCase.device->parameter(entry.key); p && std::abs(p->get().value() - authored) > 1.0e-6f) {
                followed++;
            }
        }
        QVERIFY2(followed > 0, qPrintable(testCase.name + ": the automation did nothing"));
        const auto moved = differences(before, snapshot(controller.get()));
        QVERIFY2(moved.isEmpty(), qPrintable(testCase.name + ": " + moved.join("; ")));
    }
}

void RelativeAutomationTest::test_deviceDialogs_absoluteAutomation_shouldMoveTheKnobs()
{
    // The other half of the rule: an absolute automation replaces the value, and the knob shows it
    for (auto && testCase : cases()) {
        const auto controller = testCase.controller(testCase.device);
        const auto before = snapshot(controller.get());
        for (auto && entry : testCase.device->automatableParameters()) {
            const double target = entry.parameter.value() > 0.5f ? 0.05 : 0.95;
            QVERIFY(testCase.device->applyParameterAutomation(entry.key, ParameterAutomation::Mode::Absolute, target));
        }
        QVERIFY2(!differences(before, snapshot(controller.get())).isEmpty(), qPrintable(testCase.name));
    }
}

void RelativeAutomationTest::test_effectRack_relativeAutomation_shouldLeaveTheKnobAlone()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    EffectFactory::init();
    EffectRackController controller { deviceService, std::make_shared<EditorService>() };
    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(AutoFilter::typeIdString()));
    const auto effect = deviceService->insertEffectRack().effect(0);
    QVERIFY(effect);
    const auto key = Constants::NahdXml::xmlKeyCutoff();
    const float authored = controller.parameterValue(0, key);

    QVERIFY(effect->applyParameterAutomation(key.toStdString(), ParameterAutomation::Mode::Relative, 0.3));
    QVERIFY(std::abs(effect->parameter(key.toStdString())->get().value() - authored) > 1.0e-6f);
    QCOMPARE(controller.parameterValue(0, key), authored);

    QVERIFY(effect->applyParameterAutomation(key.toStdString(), ParameterAutomation::Mode::Absolute, 0.1));
    QCOMPARE(controller.parameterValue(0, key), 0.1f);
}

void RelativeAutomationTest::test_samplerPad_relativeAutomation_shouldLeaveTheKnobAlone()
{
    // A pad's knobs read either a field the pad keeps or the pad's parameter itself; one of each
    SamplerDevice sampler { "Sampler" };
    sampler.sampleEffectRack(60); // Gives pad 60 its parameters
    sampler.setSampleCutoff(60, 0.6f);
    sampler.setSampleDetune(60, 0.7f);
    for (auto && [key, read] : std::vector<std::pair<QString, std::function<float()>>> {
           { Constants::NahdXml::xmlKeyCutoff(), [&] { return sampler.sampleCutoff(60); } },
           { Constants::NahdXml::xmlKeyDetune(), [&] { return sampler.sampleDetune(60); } } }) {
        const float authored = read();
        const auto padKey = SamplerDevice::padAutomationKey(60, key.toStdString());
        QVERIFY2(sampler.applyParameterAutomation(padKey, ParameterAutomation::Mode::Relative, 0.3), qPrintable(key));
        QVERIFY2(std::abs(read() - authored) < 1.0e-6f, qPrintable(key));
        QVERIFY(sampler.applyParameterAutomation(padKey, ParameterAutomation::Mode::Absolute, 0.1));
        QVERIFY2(std::abs(read() - 0.1f) < 1.0e-6f, qPrintable(key));
    }
}

void RelativeAutomationTest::test_deviceDialogs_modulation_shouldLeaveEveryKnobAlone()
{
    // An LFO rides on top of the setting, and the knob is the setting: a knob that shook with the
    // LFO could neither show what it is modulated around nor be set
    for (auto && testCase : cases()) {
        const auto controller = testCase.controller(testCase.device);
        const auto before = snapshot(controller.get());
        int followed = 0;
        for (auto && entry : testCase.device->automatableParameters()) {
            const float authored = entry.parameter.value();
            const double source = authored > 0.5f ? -1.0 : 1.0;
            if (testCase.device->applyParameterModulation(entry.key, ModulationSourceId {}, ParameterModulation::Mode::Bipolar, source, 0.6)) {
                if (const auto p = testCase.device->parameter(entry.key); p && std::abs(p->get().value() - authored) > 1.0e-6f) {
                    followed++;
                }
            }
        }
        QVERIFY2(followed > 0, qPrintable(testCase.name + ": the modulation did nothing"));
        const auto moved = differences(before, snapshot(controller.get()));
        QVERIFY2(moved.isEmpty(), qPrintable(testCase.name + ": " + moved.join("; ")));
    }
}

void RelativeAutomationTest::test_effectRack_modulation_shouldLeaveTheKnobAlone()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    EffectFactory::init();
    EffectRackController controller { deviceService, std::make_shared<EditorService>() };
    controller.setIsInsertRack(true);
    controller.setEffect(0, QString::fromStdString(AutoFilter::typeIdString()));
    const auto effect = deviceService->insertEffectRack().effect(0);
    QVERIFY(effect);
    const auto key = Constants::NahdXml::xmlKeyCutoff();
    const float authored = controller.parameterValue(0, key);

    QVERIFY(effect->applyParameterModulation(key.toStdString(), ModulationSourceId {}, ParameterModulation::Mode::Bipolar, authored > 0.5f ? -1.0 : 1.0, 0.6));
    QVERIFY(std::abs(effect->parameter(key.toStdString())->get().value() - authored) > 1.0e-6f);
    QCOMPARE(controller.parameterValue(0, key), authored);
}

void RelativeAutomationTest::test_samplerPad_modulation_shouldLeaveTheKnobAlone()
{
    SamplerDevice sampler { "Sampler" };
    sampler.sampleEffectRack(60); // Gives pad 60 its parameters
    sampler.setSampleCutoff(60, 0.6f);
    sampler.setSampleDetune(60, 0.7f);
    for (auto && [key, read] : std::vector<std::pair<QString, std::function<float()>>> {
           { Constants::NahdXml::xmlKeyCutoff(), [&] { return sampler.sampleCutoff(60); } },
           { Constants::NahdXml::xmlKeyDetune(), [&] { return sampler.sampleDetune(60); } } }) {
        const float authored = read();
        const auto padKey = SamplerDevice::padAutomationKey(60, key.toStdString());
        QVERIFY2(sampler.applyParameterModulation(padKey, ModulationSourceId {}, ParameterModulation::Mode::Bipolar, -1.0, 0.6), qPrintable(key));
        QVERIFY2(std::abs(read() - authored) < 1.0e-6f, qPrintable(key));
    }
}

void RelativeAutomationTest::test_deviceFader_relativeAutomation_shouldMatchExpression()
{
    // Riding a device's fader relatively is riding its Expression: the same level at the same
    // percentage, with the fader's own knob left where it was set
    const auto faderKey = Constants::NahdXml::xmlKeyFader().toStdString();
    for (const double fraction : { 0.0, 0.3, 0.5, 1.0 }) {
        SynthDevice automated { "Synth" };
        automated.setVolume(0.6f);
        QVERIFY(automated.applyParameterAutomation(faderKey, ParameterAutomation::Mode::Relative, fraction));

        SynthDevice expressed { "Synth" };
        expressed.setVolume(0.6f);
        expressed.setExpression(static_cast<float>(fraction));

        const double automatedLevel = automated.faderGain() * static_cast<double>(automated.expression());
        const double expressedLevel = expressed.faderGain() * static_cast<double>(expressed.expression());
        QVERIFY2(std::abs(automatedLevel - expressedLevel) < 1.0e-5, qPrintable(QString { "%1: %2 against %3" }.arg(fraction).arg(automatedLevel).arg(expressedLevel)));
        QCOMPARE(automated.volume(), 0.6f);
    }
}

void RelativeAutomationTest::test_notifyParameterAutomation_relative_shouldNotRefresh()
{
    // Nothing on screen changes, so nothing is worth refreshing
    const auto audioEngine = std::make_shared<AudioEngine>();
    DeviceService deviceService { audioEngine, std::make_shared<DataService>() };
    const auto device = std::make_shared<SynthDevice>("Synth");
    deviceService.setDevice(0, device);
    QSignalSpy spy { device.get(), &Device::parameterAutomationScheduled };
    const auto key = Constants::NahdXml::xmlKeyHpfCutoff().toStdString();
    deviceService.notifyParameterAutomation({ 0, 0, std::static_pointer_cast<ParameterAutomationTarget>(device), key, ParameterAutomation::Mode::Relative, 0.5 }, std::chrono::milliseconds { 0 });
    QCOMPARE(spy.count(), 0);
    deviceService.notifyParameterAutomation({ 0, 0, std::static_pointer_cast<ParameterAutomationTarget>(device), key, ParameterAutomation::Mode::Absolute, 0.5 }, std::chrono::milliseconds { 0 });
    QCOMPARE(spy.count(), 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RelativeAutomationTest)
