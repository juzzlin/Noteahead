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

#include "fm_synth_controller_test.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/fm_synth_device.hpp"
#include "../../domain/devices/fm_synth_presets.hpp"
#include "../../view/controllers/fm_operator_controller.hpp"
#include "../../view/controllers/fm_synth_controller.hpp"

#include <QMetaProperty>
#include <QSignalSpy>
#include <QTest>

#include <cmath>
#include <vector>

namespace noteahead {

namespace {

FmOperatorController * operatorAt(const FmSynthController & controller, int index)
{
    return qobject_cast<FmOperatorController *>(controller.operators().at(index).value<QObject *>());
}

int toUi(float value)
{
    return static_cast<int>(std::round(value * Constants::uiInternalScaling()));
}

} // namespace

void FmSynthControllerTest::test_properties_shouldUpdateDevice()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    FmSynthController controller { synth };

    controller.setAlgorithm(5);
    QCOMPARE(synth->algorithm(), 5);

    const int feedback = 750;
    controller.setFeedback(feedback);
    QCOMPARE(synth->feedback(), static_cast<float>(feedback) / Constants::uiInternalScaling());

    const int cutoff = 500;
    controller.setLpfCutoff(cutoff);
    QCOMPARE(synth->lpfCutoff(), static_cast<float>(cutoff) / Constants::uiInternalScaling());

    controller.setPitchBendRange(9);
    QCOMPARE(synth->pitchBendRange(), 9);
}

void FmSynthControllerTest::test_properties_shouldEmitSignals()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    FmSynthController controller { synth };

    QSignalSpy algorithmSpy { &controller, &FmSynthController::algorithmChanged };
    QSignalSpy feedbackSpy { &controller, &FmSynthController::feedbackChanged };

    // The signal travels FmSynthDevice::dataChanged -> FmSynthController::requestSettings.
    controller.setAlgorithm(3);
    QCOMPARE(algorithmSpy.count(), 1);
    QCOMPARE(feedbackSpy.count(), 1);
}

void FmSynthControllerTest::test_everyContinuousProperty_shouldRoundTripThroughTheDevice()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    FmSynthController controller { synth };

    // Every knob on the dialog, driven through the same path QML drives it: write the UI value,
    // read it back. The accessors are mechanical and near-identical, which is exactly the shape of
    // code where one wrong member goes unnoticed -- so all of them are checked rather than a
    // sample. The Amp EG and Mod EG rows are the reason this test exists.
    const QStringList properties {
        "feedback", "lpfCutoff", "lpfResonance", "hpfCutoff",
        "ampAttack", "ampDecay", "ampSustain", "ampRelease", "ampCurve", "ampVelocitySensitivity",
        "modAttack", "modDecay", "modSustain", "modInt", "modCurve",
        "lfoRate", "lfoInt", "lfoDelay", "lfoFade",
        "lfo2Rate", "lfo2Int", "lfo2Delay", "lfo2Fade",
        "voiceDepth", "panSpread", "portamento"
    };

    for (const auto & name : properties) {
        const int written = 321;
        QVERIFY2(controller.setProperty(name.toUtf8().constData(), written), qPrintable(name));
        QVERIFY2(controller.property(name.toUtf8().constData()).toInt() == written, qPrintable(name));
    }
}

void FmSynthControllerTest::test_everyContinuousProperty_shouldEmitItsOwnSignal()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    FmSynthController controller { synth };

    // A knob whose notification never fires shows a stale value the moment anything else moves it,
    // which reads in the dialog as a control that does nothing.
    const auto metaObject = controller.metaObject();
    for (int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); i++) {
        const auto property = metaObject->property(i);
        if (!property.isWritable() || property.typeId() != QMetaType::Int) {
            continue;
        }
        QVERIFY2(property.hasNotifySignal(), property.name());
    }
}

void FmSynthControllerTest::test_deviceChange_shouldRefreshProperties()
{
    const auto synth1 = std::make_shared<FmSynthDevice>("FM 1");
    const auto synth2 = std::make_shared<FmSynthDevice>("FM 2");
    synth2->setAlgorithm(6);
    synth2->setLpfCutoff(0.4f);

    FmSynthController controller { synth1 };
    QCOMPARE(controller.algorithm(), 0);

    controller.setDevice(synth2);
    QCOMPARE(controller.algorithm(), 6);
    QCOMPARE(controller.lpfCutoff(), toUi(0.4f));
}

void FmSynthControllerTest::test_reset_shouldRestoreDefaultValues()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    FmSynthController controller { synth };

    controller.setAlgorithm(4);
    controller.setLpfCutoff(300);
    QCOMPARE(synth->algorithm(), 4);

    synth->reset();
    QCOMPARE(controller.algorithm(), 0);
    QCOMPARE(controller.lpfCutoff(), toUi(1.0f));
}

void FmSynthControllerTest::test_presetNames_shouldBeNumberedLikeTheSynths()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    const FmSynthController controller { synth };

    // Three digits and a colon, as the Synth's list is written. The two dialogs sit side by side in
    // the same rack, so a preset list that reads differently in each is just noise.
    const auto names = controller.presetNames();
    QCOMPARE(names.size(), static_cast<int>(FmSynthPresets::presets().size()));
    QCOMPARE(names.at(0), QString { "000: Init" });
    for (int i = 0; i < names.size(); i++) {
        QVERIFY2(names.at(i).startsWith(QString { "%1: " }.arg(i, 3, 10, QChar { '0' })), qPrintable(names.at(i)));
    }
}

void FmSynthControllerTest::test_operators_shouldExposeOneControllerPerOperator()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    const FmSynthController controller { synth };

    QCOMPARE(controller.operators().size(), static_cast<int>(FmSynthDevice::OperatorCount));
    for (int i = 0; i < static_cast<int>(FmSynthDevice::OperatorCount); i++) {
        const auto * op = operatorAt(controller, i);
        QVERIFY(op);
        QCOMPARE(op->index(), i);
    }
}

void FmSynthControllerTest::test_operatorProperties_shouldUpdateDevice()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    const FmSynthController controller { synth };
    auto * op = operatorAt(controller, 2);

    op->setLevel(640);
    QCOMPARE(synth->operatorLevel(2), 640.0f / Constants::uiInternalScaling());
    op->setRatio(7);
    QCOMPARE(synth->operatorRatio(2), 7);
    op->setWaveform(5);
    QCOMPARE(synth->operatorWaveform(2), static_cast<FmOperator::Waveform>(5));
    op->setKeyScale(210);
    QCOMPARE(synth->operatorKeyScale(2), 210.0f / Constants::uiInternalScaling());
    op->setSustain(330);
    QCOMPARE(synth->operatorSustain(2), 330.0f / Constants::uiInternalScaling());
}

void FmSynthControllerTest::test_operatorProperties_shouldAddressTheRightOperator()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    const FmSynthController controller { synth };

    // Four panels driven from one class: an off-by-one in the index would leave the dialog editing
    // a different operator from the one it is drawn under, which nothing else here would catch.
    for (int i = 0; i < static_cast<int>(FmSynthDevice::OperatorCount); i++) {
        operatorAt(controller, i)->setLevel(100 * (i + 1));
    }
    for (int i = 0; i < static_cast<int>(FmSynthDevice::OperatorCount); i++) {
        QCOMPARE(synth->operatorLevel(static_cast<size_t>(i)), static_cast<float>(100 * (i + 1)) / Constants::uiInternalScaling());
        QCOMPARE(operatorAt(controller, i)->level(), 100 * (i + 1));
    }
}

void FmSynthControllerTest::test_operators_deviceChange_shouldFollowTheNewDevice()
{
    const auto synth1 = std::make_shared<FmSynthDevice>("FM 1");
    const auto synth2 = std::make_shared<FmSynthDevice>("FM 2");
    synth2->setOperatorLevel(1, 0.75f);

    FmSynthController controller { synth1 };
    QCOMPARE(operatorAt(controller, 1)->level(), 0);

    controller.setDevice(synth2);
    QCOMPARE(operatorAt(controller, 1)->level(), toUi(0.75f));

    // And writes must land on the new device, not the one the panels were built with.
    operatorAt(controller, 1)->setLevel(200);
    QCOMPARE(synth2->operatorLevel(1), 200.0f / Constants::uiInternalScaling());
    QCOMPARE(synth1->operatorLevel(1), 0.0f);
}

void FmSynthControllerTest::test_operatorRatioText_shouldReadAsTheRatio()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    const FmSynthController controller { synth };
    auto * op = operatorAt(controller, 0);

    op->setRatio(0);
    QCOMPARE(op->ratioText(), QString { "0.5" });
    op->setRatio(1);
    QCOMPARE(op->ratioText(), QString { "1" });
    op->setRatio(12);
    QCOMPARE(op->ratioText(), QString { "12" });
}

void FmSynthControllerTest::test_operatorRouting_shouldFollowTheAlgorithm()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    FmSynthController controller { synth };

    // Serial: only operator 1 is heard, and each operator is modulated by the next one up.
    controller.setAlgorithm(0);
    QVERIFY(operatorAt(controller, 0)->carrier());
    QVERIFY(!operatorAt(controller, 1)->carrier());
    QCOMPARE(operatorAt(controller, 0)->modulatedBy(), QVariantList { 2 });
    QVERIFY(operatorAt(controller, 3)->modulatedBy().isEmpty());

    // Additive: all four are heard and nothing modulates anything.
    controller.setAlgorithm(static_cast<int>(FmSynthDevice::AlgorithmCount) - 1);
    for (int i = 0; i < static_cast<int>(FmSynthDevice::OperatorCount); i++) {
        QVERIFY(operatorAt(controller, i)->carrier());
        QVERIFY(operatorAt(controller, i)->modulatedBy().isEmpty());
    }
}

void FmSynthControllerTest::test_operatorRouting_shouldOnlyNameHigherOperators()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    FmSynthController controller { synth };

    // The dialog draws the routing from these lists, and it stacks a modulator above whatever it
    // feeds. A source at or below its target would have the diagram drawing a loop.
    for (int algorithm = 0; algorithm < static_cast<int>(FmSynthDevice::AlgorithmCount); algorithm++) {
        controller.setAlgorithm(algorithm);
        for (int i = 0; i < static_cast<int>(FmSynthDevice::OperatorCount); i++) {
            for (const auto & source : operatorAt(controller, i)->modulatedBy()) {
                QVERIFY(source.toInt() > i + 1);
            }
        }
    }
}

void FmSynthControllerTest::test_operatorRouting_everyOperator_shouldReachTheOutput()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    FmSynthController controller { synth };

    // The dialog's diagram gives a carrier the bottom row and puts each modulator one row above
    // whatever it feeds. An operator that is neither heard nor feeds anything that is heard would
    // never be given a row at all, and the diagram would try to draw a box at no position. It would
    // also be an operator whose knobs do nothing, which is a patch bug rather than a drawing one.
    for (int algorithm = 0; algorithm < static_cast<int>(FmSynthDevice::AlgorithmCount); algorithm++) {
        controller.setAlgorithm(algorithm);

        std::vector<bool> reaches(FmSynthDevice::OperatorCount, false);
        for (size_t i = 0; i < FmSynthDevice::OperatorCount; i++) {
            reaches[i] = operatorAt(controller, static_cast<int>(i))->carrier();
        }
        // Modulators always carry a higher number than what they feed, so one sweep in increasing
        // order settles every operator: a target is always reached before the operators feeding it
        // are visited.
        for (int target = 0; target < static_cast<int>(FmSynthDevice::OperatorCount); target++) {
            if (!reaches[static_cast<size_t>(target)]) {
                continue;
            }
            for (const auto & source : operatorAt(controller, target)->modulatedBy()) {
                reaches[static_cast<size_t>(source.toInt() - 1)] = true;
            }
        }

        for (size_t i = 0; i < FmSynthDevice::OperatorCount; i++) {
            QVERIFY(reaches[i]);
        }
    }
}

void FmSynthControllerTest::test_feedbackOperator_shouldBeTheLastOne()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    const FmSynthController controller { synth };

    for (int i = 0; i < static_cast<int>(FmSynthDevice::OperatorCount); i++) {
        QCOMPARE(operatorAt(controller, i)->feedbackOperator(), i == static_cast<int>(FmSynthDevice::OperatorCount) - 1);
    }
}

void FmSynthControllerTest::test_voiceModes_shouldMatchThePersistedOrder()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    const FmSynthController controller { synth };

    // The order is the persisted VoiceMode ordinal, so this list is append-only.
    const auto modes = controller.voiceModes();
    QCOMPARE(modes.size(), 6);
    QCOMPARE(modes.at(0), QString { "Poly" });
    QCOMPARE(modes.at(1), QString { "Unison" });
    QCOMPARE(modes.at(2), QString { "Dual" });
    QCOMPARE(modes.at(3), QString { "Supersaw" });
    QCOMPARE(modes.at(4), QString { "Drift" });
    QCOMPARE(modes.at(5), QString { "Mono" });
}

void FmSynthControllerTest::test_voiceMode_everyOfferedMode_shouldReachTheDevice()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    FmSynthController controller { synth };

    for (int i = 0; i < controller.voiceModes().size(); i++) {
        controller.setVoiceMode(i);
        QCOMPARE(static_cast<int>(synth->voiceMode()), i);
    }
}

void FmSynthControllerTest::test_modTarget_everyOfferedTarget_shouldReachTheDevice()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    FmSynthController controller { synth };

    for (int i = 0; i < controller.modTargetNames().size(); i++) {
        controller.setModTarget(i);
        QCOMPARE(static_cast<int>(synth->modTarget()), i);
    }
}

void FmSynthControllerTest::test_lfoTarget_everyOfferedTarget_shouldReachTheDevice()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    FmSynthController controller { synth };

    for (int i = 0; i < controller.lfoTargetNames().size(); i++) {
        controller.setLfoTarget(i);
        QCOMPARE(static_cast<int>(synth->lfoTarget()), i);
        controller.setLfo2Target(i);
        QCOMPARE(static_cast<int>(synth->lfo2Target()), i);
    }
}

void FmSynthControllerTest::test_algorithmNames_everyOfferedAlgorithm_shouldReachTheDevice()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    FmSynthController controller { synth };

    QCOMPARE(controller.algorithmNames().size(), static_cast<int>(FmSynthDevice::AlgorithmCount));
    for (int i = 0; i < controller.algorithmNames().size(); i++) {
        controller.setAlgorithm(i);
        QCOMPARE(synth->algorithm(), i);
    }
}

void FmSynthControllerTest::test_ratioNames_shouldCoverEveryRatioSetting()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    const FmSynthController controller { synth };

    // The combo box is indexed by the setting itself, so the list has to be exactly as long as the
    // range and in the same order.
    const auto names = controller.ratioNames();
    QCOMPARE(names.size(), FmSynthDevice::MaxRatioSetting + 1);
    QCOMPARE(names.at(0), QString { "0.5" });
    QCOMPARE(names.at(1), QString { "1" });
    QCOMPARE(names.last(), QString::number(FmSynthDevice::MaxRatioSetting));
}

void FmSynthControllerTest::test_operatorWaveformNames_shouldCoverEveryWaveform()
{
    const auto synth = std::make_shared<FmSynthDevice>("Test FM");
    const FmSynthController controller { synth };

    QCOMPARE(controller.operatorWaveformNames().size(), static_cast<int>(FmOperator::WaveformCount));
    QCOMPARE(controller.operatorWaveformNames().at(0), QString { "Sine" });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::FmSynthControllerTest)
