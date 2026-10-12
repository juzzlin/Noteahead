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

#include "wavetable_synth_controller_test.hpp"

#include "../../application/service/preset_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/wavetable_synth_device.hpp"
#include "../../domain/devices/wavetable_synth_presets.hpp"
#include "../../domain/dsp/lfo.hpp"
#include "../../view/controllers/wavetable_synth_controller.hpp"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>
#include <cmath>

namespace noteahead {

void WavetableSynthControllerTest::test_properties_shouldUpdateDevice()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    const int pos = 750;
    controller.setOsc1Pos(pos);
    QCOMPARE(synth->osc1Pos(), static_cast<float>(pos) / Constants::uiInternalScaling());

    const int cutoff = 500;
    controller.setLpfCutoff(cutoff);
    QCOMPARE(synth->lpfCutoff(), static_cast<float>(cutoff) / Constants::uiInternalScaling());

    QSignalSpy spy { &controller, &WavetableSynthController::phaseRandomnessChanged };
    controller.setPhaseRandomness(400);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(controller.phaseRandomness(), 400);
    QVERIFY(std::abs(synth->phaseRandomness() - 0.4f) < 1.0e-6f);
}

void WavetableSynthControllerTest::test_deviceChange_shouldRefreshProperties()
{
    const auto synth1 = std::make_shared<WavetableSynthDevice>("Synth 1");
    const auto synth2 = std::make_shared<WavetableSynthDevice>("Synth 2");
    synth2->setOsc1Pos(0.8f);

    WavetableSynthController controller { synth1 };
    QCOMPARE(controller.osc1Pos(), 0);

    controller.setDevice(synth2);
    QCOMPARE(controller.osc1Pos(), static_cast<int>(std::round(0.8f * Constants::uiInternalScaling())));
}

void WavetableSynthControllerTest::test_reset_shouldRestoreDefaultValues()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    controller.setOsc1Pos(800);
    QCOMPARE(synth->osc1Pos(), 0.8f);

    synth->reset();
    QCOMPARE(controller.osc1Pos(), 0);
}

void WavetableSynthControllerTest::test_properties_shouldEmitSignals()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    QSignalSpy spy { &controller, &WavetableSynthController::osc1PosChanged };
    controller.setOsc1Pos(500);

    // The signal is emitted via WavetableSynthDevice::dataChanged -> WavetableSynthController::requestSettings
    QCOMPARE(spy.count(), 1);
}

void WavetableSynthControllerTest::test_voiceModes()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };
    // The order is the persisted VoiceMode ordinal, so this list is append-only
    const auto modes = controller.voiceModes();
    QCOMPARE(modes.size(), 6);
    QCOMPARE(modes.at(0), QString("Poly"));
    QCOMPARE(modes.at(1), QString("Unison"));
    QCOMPARE(modes.at(2), QString("Dual"));
    QCOMPARE(modes.at(3), QString("Supersaw"));
    QCOMPARE(modes.at(4), QString("Drift"));
    QCOMPARE(modes.at(5), QString("Mono"));
}

void WavetableSynthControllerTest::test_voiceMode_everyOfferedMode_shouldReachTheDevice()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    // A mode the dialog lists but the setter drops leaves the combo box showing one thing and the
    // device playing another, until the next unrelated edit snaps the combo box back
    for (int mode = 0; mode < controller.voiceModes().size(); mode++) {
        controller.setVoiceMode(mode);
        QCOMPARE(controller.voiceMode(), mode);
        QCOMPARE(static_cast<int>(synth->voiceMode()), mode);
    }
}

void WavetableSynthControllerTest::test_lfoTarget_everyOfferedTarget_shouldReachTheDevice()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    for (int target = 0; target < controller.lfoTargetNames().size(); target++) {
        controller.setLfoTarget(target);
        QCOMPARE(controller.lfoTarget(), target);
        QCOMPARE(static_cast<int>(synth->lfoTarget()), target);
    }
}

void WavetableSynthControllerTest::test_lfo2Target_everyOfferedTarget_shouldReachTheDevice()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    for (int target = 0; target < controller.lfo2TargetNames().size(); target++) {
        controller.setLfo2Target(target);
        QCOMPARE(controller.lfo2Target(), target);
        QCOMPARE(static_cast<int>(synth->lfo2Target()), target);
    }
}

void WavetableSynthControllerTest::test_lfo2_properties_shouldUpdateDevice()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    const int rate = 750;
    controller.setLfo2Rate(rate);
    QCOMPARE(synth->lfo2Rate(), static_cast<float>(rate) / Constants::uiInternalScaling());

    const int intensity = 300;
    controller.setLfo2Int(intensity);
    QCOMPARE(synth->lfo2Int(), static_cast<float>(intensity) / Constants::uiInternalScaling());

    controller.setLfo2Target(static_cast<int>(WavetableSynthDevice::LfoTarget::Cutoff));
    QCOMPARE(synth->lfo2Target(), WavetableSynthDevice::LfoTarget::Cutoff);
}

void WavetableSynthControllerTest::test_lfoEngagementProperties_shouldRoundTripThroughTheDevice()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    // Four near-identical accessors across two LFOs, which is exactly where one wrong member goes
    // unnoticed, so each is driven the way QML drives it: write the UI value, read it back.
    const QStringList properties { "lfoDelay", "lfoFade", "lfo2Delay", "lfo2Fade" };
    for (const auto & name : properties) {
        const int written = 321;
        QVERIFY2(controller.setProperty(name.toUtf8().constData(), written), qPrintable(name));
        QVERIFY2(controller.property(name.toUtf8().constData()).toInt() == written, qPrintable(name));
    }
}

void WavetableSynthControllerTest::test_lfo2_properties_shouldEmitSignals()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    QSignalSpy spy { &controller, &WavetableSynthController::lfo2IntChanged };
    controller.setLfo2Int(500);

    QCOMPARE(spy.count(), 1);
}

void WavetableSynthControllerTest::test_sustain_properties_shouldEmitSignals()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    const int sustain = 300;

    // Without the signal the knob's binding never sees the new value and the knob springs back to
    // where it was as soon as it is let go, even though the device did take the value.
    QSignalSpy ampSpy { &controller, &WavetableSynthController::ampSustainChanged };
    controller.setAmpSustain(sustain);
    QCOMPARE(controller.ampSustain(), sustain);
    QCOMPARE(ampSpy.count(), 1);

    QSignalSpy modSpy { &controller, &WavetableSynthController::modSustainChanged };
    controller.setModSustain(sustain);
    QCOMPARE(controller.modSustain(), sustain);
    QCOMPARE(modSpy.count(), 1);
}

void WavetableSynthControllerTest::test_curve_properties_shouldEmitSignals()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };

    const int curve = 700;

    QSignalSpy ampSpy { &controller, &WavetableSynthController::ampCurveChanged };
    controller.setAmpCurve(curve);
    QCOMPARE(controller.ampCurve(), curve);
    QCOMPARE(ampSpy.count(), 1);

    QSignalSpy modSpy { &controller, &WavetableSynthController::modCurveChanged };
    controller.setModCurve(curve);
    QCOMPARE(controller.modCurve(), curve);
    QCOMPARE(modSpy.count(), 1);
}

void WavetableSynthControllerTest::test_lfoRateDivider_shouldReachTheDevice()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Divider");
    WavetableSynthController controller { synth };
    QCOMPARE(controller.maxLfoRateDivider(), Lfo::MaxRateDivider);

    QSignalSpy spy { &controller, &WavetableSynthController::lfoRateDividerChanged };
    controller.setLfoRateDivider(8);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(controller.lfoRateDivider(), 8);
    QCOMPARE(synth->lfoRateDivider(), 8);

    QSignalSpy spy2 { &controller, &WavetableSynthController::lfo2RateDividerChanged };
    controller.setLfo2RateDivider(64);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(synth->lfo2RateDivider(), 64);
}

void WavetableSynthControllerTest::test_presetNames_shouldListTheFactoryPatches()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };
    const auto names = controller.presetNames();
    QCOMPARE(names.size(), static_cast<int>(WavetableSynthPresets::presets().size()));
    QVERIFY(names.first().endsWith("Init"));
}

void WavetableSynthControllerTest::test_loadPreset_factory_shouldApplyThePatch()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };
    const auto & presets = WavetableSynthPresets::presets();
    const auto pulseBass = std::ranges::find_if(presets, [](auto && preset) { return preset.name == "Pulse Bass"; });
    QVERIFY(pulseBass != presets.end());
    const auto index = static_cast<int>(pulseBass - presets.begin());

    controller.loadPreset(index);
    QCOMPARE(controller.currentPresetIndex(), index);
    QVERIFY(!controller.currentPresetIsUserPreset());
    QCOMPARE(synth->voiceMode(), WavetableSynthDevice::VoiceMode::Mono);
}

void WavetableSynthControllerTest::test_loadPreset_userPreset_shouldApplyTheStoredPatch()
{
    QTemporaryDir root;
    const auto presetService = std::make_shared<PresetService>(root.path());
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };
    controller.setPresetService(presetService);

    synth->setLpfCutoff(0.4f);
    QVERIFY(controller.saveUserPreset("My Pad"));
    const auto factoryCount = static_cast<int>(WavetableSynthPresets::presets().size());
    QCOMPARE(controller.presetNames().size(), factoryCount + 1);

    // A factory preset in between, so that what the user preset restores cannot already be there
    controller.loadPreset(0);
    QVERIFY(std::abs(synth->lpfCutoff() - 0.4f) > 0.001f);

    controller.loadPreset(factoryCount);
    QVERIFY(controller.currentPresetIsUserPreset());
    QCOMPARE(controller.currentUserPresetName(), QString { "My Pad" });
    QVERIFY(std::abs(synth->lpfCutoff() - 0.4f) < 0.001f);
}

void WavetableSynthControllerTest::test_ampVelocitySensitivity_shouldReachTheDevice()
{
    const auto synth = std::make_shared<WavetableSynthDevice>("Test Synth");
    WavetableSynthController controller { synth };
    QCOMPARE(controller.ampVelocitySensitivity(), static_cast<int>(Constants::uiInternalScaling()));
    QSignalSpy spy { &controller, &WavetableSynthController::ampVelocitySensitivityChanged };
    controller.setAmpVelocitySensitivity(static_cast<int>(0.25 * Constants::uiInternalScaling()));
    QVERIFY(std::abs(synth->ampVelocitySensitivity() - 0.25f) < 0.001f);
    QCOMPARE(controller.ampVelocitySensitivity(), static_cast<int>(0.25 * Constants::uiInternalScaling()));
    // Re-read with every other setting, so a dialog follows a project load or a preset
    controller.requestSettings();
    QVERIFY(spy.count() >= 1);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::WavetableSynthControllerTest)
