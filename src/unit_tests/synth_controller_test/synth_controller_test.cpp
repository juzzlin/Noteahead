// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
#include "synth_controller_test.hpp"
#include "../../application/service/preset_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/devices/synth_presets.hpp"
#include "../../view/controllers/synth_controller.hpp"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace noteahead {

void SynthControllerTest::test_sampleRateChange_shouldUpdateHzValues()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };

    // Initially sample rate should be default
    QCOMPARE(controller.sampleRate(), static_cast<uint32_t>(Constants::defaultSampleRate()));

    synth->setLpfCutoff(0.5f);
    const auto initialHz = controller.cutoffToHz(controller.lpfCutoff());
    QVERIFY(initialHz > 0.0f);

    QSignalSpy cutoffSpy { &controller, &SynthController::lpfCutoffChanged };
    QSignalSpy srSpy { &controller, &SynthController::sampleRateChanged };

    // Change sample rate to something lower so maxFreq is affected (min(20000, sr*0.49))
    synth->setSampleRate(32000);

    QCOMPARE(srSpy.count(), 1);
    QCOMPARE(cutoffSpy.count(), 1);

    const auto newHz = controller.cutoffToHz(controller.lpfCutoff());
    const auto expectedHz = initialHz * ((32000.0f * 0.49f) / 20000.0f);
    QVERIFY2(std::abs(newHz - expectedHz) < 1.0f,
             QString("newHz: %1, initialHz: %2, expectedHz: %3").arg(newHz).arg(initialHz).arg(expectedHz).toUtf8().constData());
}

void SynthControllerTest::test_properties_shouldUpdateDeviceAndEmitSignals()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };

    // Common properties
    {
        QSignalSpy spy { &controller, &SynthController::volumeChanged };
        controller.setVolume(800);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.volume(), 800);
    }

    // VCO1
    {
        QSignalSpy spy { &controller, &SynthController::vco1WaveformChanged };
        controller.setVco1Waveform(1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.vco1Waveform(), 1);
    }
    {
        QSignalSpy spy { &controller, &SynthController::vco1OctaveChanged };
        controller.setVco1Octave(1);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.vco1Octave(), 1);
    }
    // A knob's value binding only re-reads when its NOTIFY fires, and dragging a Slider whose value
    // is bound re-applies that binding: a property that changes the device but never announces it
    // leaves the handle springing straight back to where it started.
    {
        QSignalSpy spy { &controller, &SynthController::vco1RoundnessChanged };
        controller.setVco1Roundness(750);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.vco1Roundness(), 750);
    }
    {
        QSignalSpy spy { &controller, &SynthController::vco2RoundnessChanged };
        controller.setVco2Roundness(250);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.vco2Roundness(), 250);
    }
    {
        QSignalSpy spy { &controller, &SynthController::vco3RoundnessChanged };
        controller.setVco3Roundness(1000);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.vco3Roundness(), 1000);
    }

    // Filter
    {
        QSignalSpy spy { &controller, &SynthController::lpfCutoffChanged };
        controller.setLpfCutoff(600);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.lpfCutoff(), 600);
    }

    // Envelopes
    {
        QSignalSpy spy { &controller, &SynthController::ampCurveChanged };
        controller.setAmpCurve(700);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.ampCurve(), 700);
    }
    {
        QSignalSpy spy { &controller, &SynthController::modCurveChanged };
        controller.setModCurve(350);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.modCurve(), 350);
    }

    // LFO
    {
        QSignalSpy spy { &controller, &SynthController::lfoRateChanged };
        controller.setLfoRate(400);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.lfoRate(), 400);
    }
}

void SynthControllerTest::test_reset_shouldRestoreDefaultValues()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };

    controller.setVolume(100);
    QSignalSpy spy { &controller, &SynthController::volumeChanged };
    controller.reset();
    QVERIFY(spy.count() >= 1);
    QCOMPARE(controller.volume(), static_cast<int>(std::round(Constants::faderUnityPosition() * Constants::uiInternalScaling())));
}

void SynthControllerTest::test_octaveNames()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };
    const auto octaves = controller.octaveNames();
    QCOMPARE(octaves.size(), 5);
    // The dialogs index this list by octave + 2, so 8' has to stay in the middle.
    QCOMPARE(octaves.at(0), QString("32'"));
    QCOMPARE(octaves.at(1), QString("16'"));
    QCOMPARE(octaves.at(2), QString("8'"));
    QCOMPARE(octaves.at(3), QString("4'"));
    QCOMPARE(octaves.at(4), QString("2'"));

    // And the range the names cover has to be the range the synth actually accepts.
    controller.setVco1Octave(-2);
    QCOMPARE(controller.vco1Octave(), -2);
    controller.setVco1Octave(2);
    QCOMPARE(controller.vco1Octave(), 2);
}

void SynthControllerTest::test_squareWaveformIndex_shouldMatchWaveformNames()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };

    // The dialogs disable Roundness on everything but the pulse by comparing the VCO's waveform
    // against this index, so it has to keep pointing at the pulse in the list they show.
    const auto index = controller.squareWaveformIndex();
    QCOMPARE(controller.vcoWaveformNames().at(index), QString("Square"));
}

void SynthControllerTest::test_modTargetNames()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };
    const auto targets = controller.modTargetNames();
    QCOMPARE(targets.size(), 10);

    // The first four are what the Mod EG has always offered, and a project saved before the rest
    // were added stores one of these ordinals. They must not move.
    QCOMPARE(targets.at(0), QString("Pitch 1"));
    QCOMPARE(targets.at(1), QString("Pitch 2"));
    QCOMPARE(targets.at(2), QString("Pitch 3"));
    QCOMPARE(targets.at(3), QString("Cutoff"));

    // Appended to bring the Mod EG up to the LFOs' destinations.
    QCOMPARE(targets.at(4), QString("Pitch"));
    QCOMPARE(targets.at(5), QString("Shape"));
    QCOMPARE(targets.at(6), QString("Volume"));
    QCOMPARE(targets.at(7), QString("Resonance"));
    QCOMPARE(targets.at(8), QString("Pan"));
    QCOMPARE(targets.at(9), QString("HPF Cutoff"));

    // And every one of them has to survive the parameter's clamp, which is the other half of
    // widening an enum: the range the value is stored in has to grow with it.
    for (int target = 0; target < targets.size(); target++) {
        controller.setModTarget(target);
        QCOMPARE(controller.modTarget(), target);
    }
}

void SynthControllerTest::test_voiceModes()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };
    const auto modes = controller.voiceModes();
    QCOMPARE(modes.size(), 6);
    // The order is the serialized VoiceMode ordinal, so it is append-only: reordering these would
    // change the voice mode of every project saved before the change.
    QCOMPARE(modes.at(0), QString("Poly"));
    QCOMPARE(modes.at(1), QString("Unison"));
    QCOMPARE(modes.at(2), QString("Dual"));
    QCOMPARE(modes.at(3), QString("Supersaw"));
    QCOMPARE(modes.at(4), QString("Drift"));
    QCOMPARE(modes.at(5), QString("Mono"));
}

void SynthControllerTest::test_lfoEngagementProperties_shouldRoundTripThroughTheDevice()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };

    // Four near-identical accessors across two LFOs, which is exactly where one wrong member goes
    // unnoticed, so each is driven the way QML drives it: write the UI value, read it back.
    const QStringList properties { "lfoDelay", "lfoFade", "lfo2Delay", "lfo2Fade" };
    for (const auto & name : properties) {
        const int written = 321;
        QVERIFY2(controller.setProperty(name.toUtf8().constData(), written), qPrintable(name));
        QVERIFY2(controller.property(name.toUtf8().constData()).toInt() == written, qPrintable(name));
    }
}

void SynthControllerTest::test_lfoTargetNames()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };
    const auto targets = controller.lfoTargetNames();
    QCOMPARE(targets.size(), 10);
    // The order is the serialized LfoTarget ordinal, so it is append-only: reordering these would
    // change the LFO destination of every project saved before the change.
    QCOMPARE(targets.at(0), QString("Pitch"));
    QCOMPARE(targets.at(1), QString("Shape"));
    QCOMPARE(targets.at(2), QString("Cutoff"));
    QCOMPARE(targets.at(3), QString("Volume"));
    QCOMPARE(targets.at(4), QString("Resonance"));
    QCOMPARE(targets.at(5), QString("Pan"));
    QCOMPARE(targets.at(6), QString("Pitch 1"));
    QCOMPARE(targets.at(7), QString("Pitch 2"));
    QCOMPARE(targets.at(8), QString("Pitch 3"));
    // Appended, not inserted: everything above keeps the ordinal it had.
    QCOMPARE(targets.at(9), QString("HPF Cutoff"));

    // LFO 2 offers the same destinations.
    QCOMPARE(controller.lfo2TargetNames(), targets);

    // And the whole range the names cover has to survive the parameter clamp.
    for (int target = 0; target < targets.size(); target++) {
        controller.setLfoTarget(target);
        QCOMPARE(controller.lfoTarget(), target);
        controller.setLfo2Target(target);
        QCOMPARE(controller.lfo2Target(), target);
    }
}

void SynthControllerTest::test_scopeActive_shouldFollowShownInstance()
{
    const auto synthA = std::make_shared<SynthDevice>("Synth A");
    const auto synthB = std::make_shared<SynthDevice>("Synth B");
    SynthController controller { synthA };

    // Nothing captures until the scope is shown.
    QVERIFY(!synthA->scope().active());

    // Showing the scope enables capture on the current instance only.
    controller.setScopeActive(true);
    QVERIFY(synthA->scope().active());
    QVERIFY(!synthB->scope().active());

    // Switching instance while the scope is shown must move capture to the new instance and stop
    // it on the old one, so a device that is no longer displayed never keeps capturing.
    controller.setDevice(synthB);
    QVERIFY(!synthA->scope().active());
    QVERIFY(synthB->scope().active());

    // Hiding the scope stops all capture.
    controller.setScopeActive(false);
    QVERIFY(!synthB->scope().active());
}

void SynthControllerTest::test_loadPreset_shouldShowTheLoadedPreset()
{
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };

    // The dialog binds the combo box to this and asks only for a load, so a load that does not move
    // it leaves the box reading its previous entry.
    controller.loadPreset(2);
    QCOMPARE(controller.currentPresetIndex(), 2);
    QCOMPARE(controller.presetNames().size(), static_cast<int>(SynthPresets::presets().size()));
}

void SynthControllerTest::test_presetNames_withUserPresets_shouldContinueTheFactoryNumbering()
{
    QTemporaryDir root;
    const auto presetService = std::make_shared<PresetService>(root.path());
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };
    controller.setPresetService(presetService);

    const auto factoryCount = static_cast<int>(SynthPresets::presets().size());
    QCOMPARE(controller.presetNames().size(), factoryCount);

    QVERIFY(controller.saveUserPreset("My Lead"));

    const auto names = controller.presetNames();
    QCOMPARE(names.size(), factoryCount + 1);
    // The user's own sit at the end of the factory numbering rather than starting one of their own,
    // and are marked so that the two can still be told apart.
    QCOMPARE(names.last(), QString { "%1: My Lead%2" }.arg(factoryCount, 3, 10, QChar { '0' }).arg(Constants::userPresetMarker()));
}

void SynthControllerTest::test_loadPreset_userPreset_shouldApplyTheStoredPatch()
{
    QTemporaryDir root;
    const auto presetService = std::make_shared<PresetService>(root.path());
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };
    controller.setPresetService(presetService);

    synth->setLpfCutoff(0.75f);
    QVERIFY(controller.saveUserPreset("My Lead"));

    const auto factoryCount = static_cast<int>(SynthPresets::presets().size());

    // A factory preset in between, so that what the user preset restores cannot be what was already
    // in the device
    controller.loadPreset(0);
    QVERIFY(std::abs(synth->lpfCutoff() - 0.75f) > 0.001f);

    controller.loadPreset(factoryCount);
    QCOMPARE(controller.currentPresetIndex(), factoryCount);
    QVERIFY(controller.currentPresetIsUserPreset());
    QCOMPARE(controller.currentUserPresetName(), QString { "My Lead" });
    QVERIFY(std::abs(synth->lpfCutoff() - 0.75f) < 0.001f);
}

void SynthControllerTest::test_saveUserPreset_shouldSelectWhatWasJustSaved()
{
    QTemporaryDir root;
    const auto presetService = std::make_shared<PresetService>(root.path());
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };
    controller.setPresetService(presetService);

    QSignalSpy spy { &controller, &SynthController::presetNamesChanged };

    QVERIFY(controller.saveUserPreset("Zither"));
    QVERIFY(controller.saveUserPreset("Apex"));

    QCOMPARE(spy.count(), 2);
    // Sorted by name, so the one just saved is not the last one -- the selection has to follow the
    // preset rather than the end of the list.
    QCOMPARE(controller.currentPresetIndex(), static_cast<int>(SynthPresets::presets().size()));
    QCOMPARE(controller.currentUserPresetName(), QString { "Apex" });
}

void SynthControllerTest::test_deleteCurrentUserPreset_factoryPreset_shouldDeleteNothing()
{
    QTemporaryDir root;
    const auto presetService = std::make_shared<PresetService>(root.path());
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };
    controller.setPresetService(presetService);

    QVERIFY(controller.saveUserPreset("My Lead"));

    controller.loadPreset(0);
    QVERIFY(!controller.currentPresetIsUserPreset());
    QVERIFY(!controller.deleteCurrentUserPreset());
    QCOMPARE(controller.presetNames().size(), static_cast<int>(SynthPresets::presets().size()) + 1);
}

void SynthControllerTest::test_deleteCurrentUserPreset_shouldDropItFromTheList()
{
    QTemporaryDir root;
    const auto presetService = std::make_shared<PresetService>(root.path());
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    SynthController controller { synth };
    controller.setPresetService(presetService);

    QVERIFY(controller.saveUserPreset("My Lead"));
    QVERIFY(controller.deleteCurrentUserPreset());

    const auto factoryCount = static_cast<int>(SynthPresets::presets().size());
    QCOMPARE(controller.presetNames().size(), factoryCount);
    // The selection was on the deleted preset, which was also the last entry: it must land inside
    // the list that is left rather than one past its end.
    QCOMPARE(controller.currentPresetIndex(), factoryCount - 1);
    QVERIFY(!controller.currentPresetIsUserPreset());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SynthControllerTest)
