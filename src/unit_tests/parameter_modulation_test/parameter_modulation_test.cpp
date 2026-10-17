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

#include "parameter_modulation_test.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/effects/gain.hpp"
#include "../../domain/tracker/parameter.hpp"
#include "../../domain/tracker/parameter_modulation.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

using Mode = ParameterModulation::Mode;

Parameter knob(float value, std::optional<ParameterDisplay> display = std::nullopt)
{
    Parameter parameter { "test", value, 0, 10000, 0, 100 };
    if (display) {
        parameter.setDisplay(*display);
    }
    return parameter;
}

bool near(double a, double b)
{
    return std::abs(a - b) < 1.0e-6;
}

} // namespace

void ParameterModulationTest::test_modulatedValue_bipolar_shouldSwingAroundTheSetting()
{
    // Full depth swings half the travel either way
    const auto parameter = knob(0.5f);
    QVERIFY(near(ParameterModulation::modulatedValue(Mode::Bipolar, 1.0, 1.0, parameter), 1.0));
    QVERIFY(near(ParameterModulation::modulatedValue(Mode::Bipolar, -1.0, 1.0, parameter), 0.0));
    QVERIFY(near(ParameterModulation::modulatedValue(Mode::Bipolar, 0.0, 1.0, parameter), 0.5));
    QVERIFY(near(ParameterModulation::modulatedValue(Mode::Bipolar, 0.5, 0.4, parameter), 0.6));
}

void ParameterModulationTest::test_modulatedValue_unipolar_shouldScaleTowardsTheRest()
{
    // The bottom of the source is the setting; the top is depth of the way to where the control rests
    const auto parameter = knob(0.8f);
    QVERIFY(near(ParameterModulation::modulatedValue(Mode::Unipolar, -1.0, 1.0, parameter), 0.8));
    QVERIFY(near(ParameterModulation::modulatedValue(Mode::Unipolar, 1.0, 1.0, parameter), 0.0));
    QVERIFY(near(ParameterModulation::modulatedValue(Mode::Unipolar, 1.0, 0.5, parameter), 0.4));
}

void ParameterModulationTest::test_modulatedValue_unipolarCentred_shouldScaleTowardsTheCentre()
{
    // A control that runs either way about off fades towards off rather than through it
    const auto parameter = knob(0.9f, ParameterDisplay { .mapping = ParameterDisplay::Mapping::Pan, .unit = "%", .min = -100.0, .max = 100.0, .rest = ParameterDisplay::Rest::Centre, .label = "Pan" });
    QVERIFY(near(ParameterModulation::modulatedValue(Mode::Unipolar, 1.0, 1.0, parameter), 0.5));
    QVERIFY(near(ParameterModulation::modulatedValue(Mode::Unipolar, 1.0, 0.5, parameter), 0.7));
}

void ParameterModulationTest::test_modulatedValue_tremolo_shouldScaleTheFadersGain()
{
    // As Expression does: half depth at the top of the source is half the gain
    const auto parameter = knob(Constants::faderUnityPosition(), ParameterDisplay { .mapping = ParameterDisplay::Mapping::Fader, .unit = "dB", .label = "Fader" });
    const auto value = ParameterModulation::modulatedValue(Mode::Tremolo, 1.0, 0.5, parameter);
    QVERIFY(std::abs(ParameterMapper::mapFader(value) - 0.5 * ParameterMapper::mapFader(Constants::faderUnityPosition())) < 1.0e-5);
    QVERIFY(near(ParameterModulation::modulatedValue(Mode::Tremolo, -1.0, 1.0, parameter), Constants::faderUnityPosition()));
}

void ParameterModulationTest::test_modulatedValue_tremoloOnAnythingElse_shouldBeUnipolar()
{
    const auto parameter = knob(0.8f);
    QVERIFY(near(ParameterModulation::modulatedValue(Mode::Tremolo, 0.2, 0.7, parameter), ParameterModulation::modulatedValue(Mode::Unipolar, 0.2, 0.7, parameter)));
}

void ParameterModulationTest::test_modulatedValue_shouldStayInRange()
{
    const auto parameter = knob(0.9f);
    QCOMPARE(ParameterModulation::modulatedValue(Mode::Bipolar, 1.0, 1.0, parameter), 1.0f);
    QCOMPARE(ParameterModulation::modulatedValue(Mode::Bipolar, 5.0, 3.0, parameter), 1.0f);
    QCOMPARE(ParameterModulation::modulatedValue(Mode::Unipolar, -5.0, -3.0, parameter), 0.9f);
}

void ParameterModulationTest::test_deviceModulation_fader_shouldMoveTheGainAndNotTheKnob()
{
    SynthDevice synth { "Synth" };
    synth.setVolume(Constants::faderUnityPosition());
    const auto faderKey = Constants::NahdXml::xmlKeyFader().toStdString();

    QVERIFY(synth.applyParameterModulation(faderKey, Mode::Tremolo, 1.0, 0.5));

    QVERIFY(std::abs(synth.faderGain() - 0.5 * ParameterMapper::mapFader(Constants::faderUnityPosition())) < 1.0e-5);
    QCOMPARE(synth.volume(), Constants::faderUnityPosition());
}

void ParameterModulationTest::test_deviceModulation_shouldSurviveStop()
{
    // Stop clears automation; a free-running LFO is not automation and keeps its place
    SynthDevice synth { "Synth" };
    const auto cutoffKey = Constants::NahdXml::xmlKeyLpfCutoff().toStdString();
    const float authored = synth.parameter(cutoffKey)->get().value();
    QVERIFY(synth.applyParameterModulation(cutoffKey, Mode::Unipolar, 1.0, 0.5));
    const float modulated = synth.parameter(cutoffKey)->get().value();
    QVERIFY(std::abs(modulated - authored * 0.5f) < 1.0e-6f);

    synth.clearParameterAutomation();

    QCOMPARE(synth.parameter(cutoffKey)->get().value(), modulated);
}

void ParameterModulationTest::test_deviceModulation_clear_shouldRestoreTheSetting()
{
    SynthDevice synth { "Synth" };
    const auto cutoffKey = Constants::NahdXml::xmlKeyLpfCutoff().toStdString();
    const float authored = synth.parameter(cutoffKey)->get().value();
    QVERIFY(synth.applyParameterModulation(cutoffKey, Mode::Bipolar, 1.0, 1.0));

    synth.clearParameterModulation(cutoffKey);

    QCOMPARE(synth.parameter(cutoffKey)->get().value(), authored);
    QVERIFY(!synth.parameter(cutoffKey)->get().isModulated());
}

void ParameterModulationTest::test_deviceModulation_switch_shouldBeRefused()
{
    SynthDevice synth { "Synth" };
    QVERIFY(!synth.applyParameterModulation(Constants::NahdXml::xmlKeyVoiceMode().toStdString(), Mode::Bipolar, 1.0, 1.0));
    QVERIFY(!synth.applyParameterModulation("noSuchParameter", Mode::Bipolar, 1.0, 1.0));
}

void ParameterModulationTest::test_effectModulation_shouldReachTheDsp()
{
    // An effect resyncs on a step, so what it processes follows straight away
    Gain gain;
    const auto gainKey = Constants::NahdXml::xmlKeyGain().toStdString();
    double left = 1.0;
    double right = 1.0;
    gain.process(left, right);
    QVERIFY(near(left, 1.0));

    // Bipolar at full depth and the top of the source: half the travel up from 0 dB
    QVERIFY(gain.applyParameterModulation(gainKey, Mode::Bipolar, 1.0, 1.0));
    left = 1.0;
    right = 1.0;
    gain.process(left, right);
    QVERIFY(left > 10.0);

    gain.clearParameterModulation(gainKey);
    left = 1.0;
    right = 1.0;
    gain.process(left, right);
    QVERIFY(near(left, 1.0));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ParameterModulationTest)
