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

#include "parameter_test.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../domain/tracker/parameter.hpp"
#include "../../domain/tracker/parameter_automation.hpp"
#include "../../domain/tracker/parameter_container.hpp"
#include "../../domain/tracker/parameter_display.hpp"

#include <QTest>

#include <cmath>
#include <vector>

namespace noteahead {

namespace {

Parameter continuousParameter()
{
    return Parameter { "cutoff", 0.25f, 0, 10000, 5000, 100 };
}

} // namespace

void ParameterTest::test_setAutomationValue_shouldNotChangeAuthoredValue()
{
    auto parameter = continuousParameter();

    parameter.setAutomationValue(0.75f);

    QVERIFY(qFuzzyCompare(parameter.value(), 0.75f));
    QVERIFY(qFuzzyCompare(parameter.authoredValue(), 0.25f));
    QVERIFY(parameter.isAutomated());
}

void ParameterTest::test_setAutomationValue_discrete_shouldNotChangeAuthoredValue()
{
    Parameter parameter { "waveform", 1.0f, 0, 3, 0, 1, Parameter::Type::Discrete };

    parameter.setAutomationValue(3.0f);

    QCOMPARE(parameter.xmlValue(), 3);
    QCOMPARE(parameter.xmlAuthoredValue(), 1);
}

void ParameterTest::test_setValue_shouldChangeBothValues()
{
    auto parameter = continuousParameter();

    parameter.setValue(0.75f);

    QVERIFY(qFuzzyCompare(parameter.value(), 0.75f));
    QVERIFY(qFuzzyCompare(parameter.authoredValue(), 0.75f));
    QVERIFY(!parameter.isAutomated());
}

void ParameterTest::test_setValue_whileAutomated_shouldTakeOver()
{
    // Turning a knob while a song plays authors that value there and then. The automation is free
    // to move it again on its next event, but what the user set is what gets saved.
    auto parameter = continuousParameter();
    parameter.setAutomationValue(0.75f);

    parameter.setValue(0.5f);

    QVERIFY(qFuzzyCompare(parameter.authoredValue(), 0.5f));
    QVERIFY(!parameter.isAutomated());

    parameter.clearAutomation();
    QVERIFY(qFuzzyCompare(parameter.value(), 0.5f));
}

void ParameterTest::test_clearAutomation_shouldRestoreAuthoredValue()
{
    auto parameter = continuousParameter();
    parameter.setAutomationValue(0.75f);

    parameter.clearAutomation();

    QVERIFY(qFuzzyCompare(parameter.value(), 0.25f));
    QVERIFY(!parameter.isAutomated());
}

void ParameterTest::test_clearAutomation_withoutAutomation_shouldKeepValue()
{
    auto parameter = continuousParameter();
    parameter.setValue(0.9f);

    parameter.clearAutomation();

    QVERIFY(qFuzzyCompare(parameter.value(), 0.9f));
}

void ParameterTest::test_setFromXml_shouldAuthorValue()
{
    // Loading a project is authoring: nothing of a previous automation may survive it.
    auto parameter = continuousParameter();
    parameter.setAutomationValue(0.75f);

    parameter.setFromXml(10000);

    QVERIFY(qFuzzyCompare(parameter.value(), 1.0f));
    QVERIFY(qFuzzyCompare(parameter.authoredValue(), 1.0f));
    QVERIFY(!parameter.isAutomated());
}

void ParameterTest::test_container_clearAutomation_shouldRestoreEveryParameter()
{
    ParameterContainer container;
    container.addParameter(continuousParameter());
    container.addParameter(Parameter { "pan", 0.5f, 0, 10000, 5000, 100 });

    QVERIFY(!container.isAutomated());

    container.parameter("cutoff")->get().setAutomationValue(1.0f);
    container.parameter("pan")->get().setAutomationValue(0.0f);
    QVERIFY(container.isAutomated());

    container.clearAutomation();

    QVERIFY(!container.isAutomated());
    QVERIFY(qFuzzyCompare(container.parameter("cutoff")->get().value(), 0.25f));
    QVERIFY(qFuzzyCompare(container.parameter("pan")->get().value(), 0.5f));
}

void ParameterTest::test_container_parameterSnapshot_shouldHoldAuthoredValues()
{
    // A device dialog snapshots on open and restores on Cancel. Opening it over a playing song must
    // not turn the automation into the value Cancel puts back.
    ParameterContainer container;
    container.addParameter(continuousParameter());
    container.parameter("cutoff")->get().setAutomationValue(1.0f);

    const auto snapshot = container.parameterSnapshot();

    QVERIFY(qFuzzyCompare(snapshot.at("cutoff"), 0.25f));

    container.restoreParameterSnapshot(snapshot);
    QVERIFY(qFuzzyCompare(container.parameter("cutoff")->get().value(), 0.25f));
    QVERIFY(qFuzzyCompare(container.parameter("cutoff")->get().authoredValue(), 0.25f));
}

void ParameterTest::test_display_linear_shouldRoundTrip()
{
    const ParameterDisplay gain { .mapping = ParameterDisplay::Mapping::Linear, .unit = "dB", .min = -12.0, .max = 12.0, .rest = ParameterDisplay::Rest::Centre, .label = "Test" };

    QCOMPARE(gain.toUnits(0.5), 0.0);
    QCOMPARE(gain.toUnits(1.0), 12.0);
    QCOMPARE(gain.toPosition(-6.0), 0.25);
}

void ParameterTest::test_display_curvedMappings_shouldRoundTrip()
{
    // Every curve has to come back to the position it started from, or a value typed in units
    // would land somewhere other than where it reads.
    const std::vector<ParameterDisplay> displays {
        { .mapping = ParameterDisplay::Mapping::Exponential, .unit = "ms", .min = 0.1, .max = 500.0, .label = "Test" },
        { .mapping = ParameterDisplay::Mapping::CubicCentered, .unit = "%", .min = -100.0, .max = 100.0, .rest = ParameterDisplay::Rest::Centre, .label = "Test" },
        { .mapping = ParameterDisplay::Mapping::LogFrequency, .unit = "Hz", .min = 20.0, .max = 20000.0, .label = "Test" },
        { .mapping = ParameterDisplay::Mapping::LfoFrequency, .unit = "Hz", .min = 0.05, .max = 20.0, .label = "Test" },
        { .mapping = ParameterDisplay::Mapping::ExponentialFromZero, .unit = "s", .min = 0.005, .max = 10.0, .label = "Test" },
        { .mapping = ParameterDisplay::Mapping::Cubic, .unit = "s", .min = 0.0, .max = 8.0, .label = "Test" },
        { .mapping = ParameterDisplay::Mapping::Pan, .unit = "%", .min = -100.0, .max = 100.0, .rest = ParameterDisplay::Rest::Centre, .label = "Test" },
        { .mapping = ParameterDisplay::Mapping::Bipolar, .unit = "st", .min = -24.0, .max = 24.0, .rest = ParameterDisplay::Rest::Centre, .label = "Test" }
    };
    for (const auto & display : displays) {
        for (const double position : { 0.0, 0.1, 0.37, 0.5, 0.82, 1.0 }) {
            QVERIFY2(std::abs(display.toPosition(display.toUnits(position)) - position) < 1.0e-9, display.mappingName().c_str());
        }
    }

    // The fader reads in decibels, unity at its unity position; the bottom is silence, floored
    const ParameterDisplay fader { .mapping = ParameterDisplay::Mapping::Fader, .unit = "dB", .label = "Test" };
    for (const double position : { 0.1, 0.37, 0.5, 0.82, 1.0 }) {
        QVERIFY2(std::abs(fader.toPosition(fader.toUnits(position)) - position) < 1.0e-9, qPrintable(QString::number(position)));
    }
    QVERIFY(std::abs(fader.toUnits(Constants::faderUnityPosition())) < 1.0e-9);

    // The log curve doubles in equal steps: the middle of 20 Hz to 20 kHz is their geometric mean.
    const ParameterDisplay cutoff { .mapping = ParameterDisplay::Mapping::LogFrequency, .unit = "Hz", .min = 20.0, .max = 20000.0, .label = "Test" };
    QVERIFY(std::abs(cutoff.toUnits(0.5) - std::sqrt(20.0 * 20000.0)) < 1.0e-6);
}

void ParameterTest::test_display_toPosition_outOfRange_shouldClamp()
{
    const ParameterDisplay cutoff { .mapping = ParameterDisplay::Mapping::LogFrequency, .unit = "Hz", .min = 20.0, .max = 20000.0, .label = "Test" };

    QCOMPARE(cutoff.toPosition(5.0), 0.0);
    QCOMPARE(cutoff.toPosition(40000.0), 1.0);
}

void ParameterTest::test_display_restPosition_shouldFollowTheRest()
{
    QCOMPARE((ParameterDisplay { .mapping = ParameterDisplay::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = "Test" }.restPosition()), 0.0);
    QCOMPARE((ParameterDisplay { .mapping = ParameterDisplay::Mapping::CubicCentered, .unit = "%", .min = -100.0, .max = 100.0, .rest = ParameterDisplay::Rest::Centre, .label = "Test" }.restPosition()), 0.5);
}

void ParameterTest::test_container_setParameterDisplay_shouldAttachIt()
{
    ParameterContainer container;
    container.addParameter(continuousParameter());
    QVERIFY(!container.parameter("cutoff")->get().display().has_value());

    container.setParameterDisplay("cutoff", { .mapping = ParameterDisplay::Mapping::LogFrequency, .unit = "Hz", .min = 20.0, .max = 20000.0, .label = "Test" });

    const auto & display = container.parameter("cutoff")->get().display();
    QVERIFY(display.has_value());
    QCOMPARE(display->unit, std::string { "Hz" });
    // An unknown name is ignored rather than creating a parameter.
    container.setParameterDisplay("missing", {});
    QVERIFY(!container.parameter("missing").has_value());
}

void ParameterTest::test_effectiveValue_absolute_shouldBeThePosition()
{
    const auto parameter = continuousParameter();
    QCOMPARE(ParameterAutomation::effectiveValue(ParameterAutomation::Mode::Absolute, 0.6, parameter), 0.6f);
    QCOMPARE(ParameterAutomation::effectiveValue(ParameterAutomation::Mode::Absolute, 1.4, parameter), 1.0f);
}

void ParameterTest::test_effectiveValue_relative_shouldScaleFromTheMinimum()
{
    auto parameter = continuousParameter();
    parameter.setValue(0.8f);

    // All the way is the knob itself, nothing is the bottom, and half is half the travel between.
    QCOMPARE(ParameterAutomation::effectiveValue(ParameterAutomation::Mode::Relative, 1.0, parameter), 0.8f);
    QCOMPARE(ParameterAutomation::effectiveValue(ParameterAutomation::Mode::Relative, 0.0, parameter), 0.0f);
    QVERIFY(std::abs(ParameterAutomation::effectiveValue(ParameterAutomation::Mode::Relative, 0.5, parameter) - 0.4f) < 1.0e-6f);
}

void ParameterTest::test_effectiveValue_relativeCentred_shouldScaleFromTheCentre()
{
    // A control that runs either way about off: scaling it down fades it out rather than inverting it.
    auto parameter = continuousParameter();
    parameter.setDisplay({ .mapping = ParameterDisplay::Mapping::CubicCentered, .unit = "%", .min = -100.0, .max = 100.0, .rest = ParameterDisplay::Rest::Centre, .label = "Test" });
    parameter.setValue(0.9f);

    QCOMPARE(ParameterAutomation::effectiveValue(ParameterAutomation::Mode::Relative, 0.0, parameter), 0.5f);
    QVERIFY(std::abs(ParameterAutomation::effectiveValue(ParameterAutomation::Mode::Relative, 0.5, parameter) - 0.7f) < 1.0e-6f);

    // Below the centre too
    parameter.setValue(0.1f);
    QVERIFY(std::abs(ParameterAutomation::effectiveValue(ParameterAutomation::Mode::Relative, 0.5, parameter) - 0.3f) < 1.0e-6f);
}

void ParameterTest::test_modulation_shouldMoveOnlyTheValueInEffect()
{
    // An LFO moves what the DSP runs on, never the knob, nor what is saved
    auto parameter = continuousParameter();
    parameter.setModulatedValue(0.6f);

    QCOMPARE(parameter.value(), 0.6f);
    QCOMPARE(parameter.unmodulatedValue(), 0.25f);
    QCOMPARE(parameter.shownValue(), 0.25f);
    QCOMPARE(parameter.authoredValue(), 0.25f);
    QCOMPARE(parameter.xmlAuthoredValue(), 2500);
    QVERIFY(parameter.isModulated());
    QVERIFY(!parameter.isAutomated());
}

void ParameterTest::test_modulation_underAutomation_shouldKeepItsOffset()
{
    // Between two LFO steps an automation step moves the base, and the wobble rides along with it
    auto parameter = continuousParameter();
    parameter.setModulatedValue(0.35f);

    parameter.setAutomationValue(0.5f);
    QCOMPARE(parameter.unmodulatedValue(), 0.5f);
    QVERIFY(std::abs(parameter.value() - 0.6f) < 1.0e-6f);
    // The knob follows absolute automation, as it always has, but not the modulation
    QCOMPARE(parameter.shownValue(), 0.5f);

    // And the user's own edit is the base too
    parameter.setValue(0.1f);
    QVERIFY(std::abs(parameter.value() - 0.2f) < 1.0e-6f);
}

void ParameterTest::test_modulation_clearAutomation_shouldStay()
{
    // Stopping clears automation; a free-running LFO keeps going, riding on the authored value
    auto parameter = continuousParameter();
    parameter.setAutomationValue(0.5f);
    parameter.setModulatedValue(0.55f);

    parameter.clearAutomation();

    QVERIFY(parameter.isModulated());
    QCOMPARE(parameter.unmodulatedValue(), 0.25f);
    QVERIFY(std::abs(parameter.value() - 0.3f) < 1.0e-6f);
}

void ParameterTest::test_modulation_clear_shouldRestoreTheUnmodulatedValue()
{
    auto parameter = continuousParameter();
    parameter.setAutomationValue(0.5f);
    parameter.setModulatedValue(0.9f);

    parameter.clearModulation();

    QVERIFY(!parameter.isModulated());
    QCOMPARE(parameter.value(), 0.5f);
    QVERIFY(parameter.isAutomated());
}

void ParameterTest::test_modulation_switch_shouldBeRefused()
{
    // A position between two choices means nothing
    Parameter choice { "mode", 1.0f, 0, 3, 1, 1, Parameter::Type::Discrete };
    choice.setModulatedValue(2.6f);

    QCOMPARE(choice.value(), 1.0f);
    QVERIFY(!choice.isModulated());
}

void ParameterTest::test_effectiveValue_relativeFader_shouldScaleTheGain()
{
    // What Expression does over MIDI: half is half the amplitude of wherever the fader stands, not
    // half its throw, which the taper makes a good deal quieter than that
    auto parameter = continuousParameter();
    parameter.setDisplay({ .mapping = ParameterDisplay::Mapping::Fader, .unit = "dB", .label = "Test" });
    for (const float knob : { 0.3f, Constants::faderUnityPosition(), 0.95f }) {
        parameter.setValue(knob);
        const double gain = ParameterMapper::mapFader(static_cast<double>(knob));
        for (const double fraction : { 0.0, 0.25, 0.5, 1.0 }) {
            const auto effective = ParameterAutomation::effectiveValue(ParameterAutomation::Mode::Relative, fraction, parameter);
            QVERIFY2(std::abs(ParameterMapper::mapFader(static_cast<double>(effective)) - gain * fraction) < 1.0e-5, qPrintable(QString { "%1 at %2" }.arg(knob).arg(fraction)));
        }
    }
}

void ParameterTest::test_effectiveValue_relative_shouldFollowTheAuthoredValue()
{
    // What the automation wrote last is not the knob, and scaling that would compound every step.
    auto parameter = continuousParameter();
    parameter.setValue(0.8f);
    parameter.setAutomationValue(0.2f);

    QCOMPARE(ParameterAutomation::effectiveValue(ParameterAutomation::Mode::Relative, 1.0, parameter), 0.8f);
}

void ParameterTest::test_display_highPass_shouldFollowTheLowPassCurve()
{
    // Only the readout differs: which end it calls Bypass
    const ParameterDisplay lowPass { .mapping = ParameterDisplay::Mapping::LogFrequency, .unit = "Hz", .min = 20.0, .max = 20000.0, .label = "Test" };
    const ParameterDisplay highPass { .mapping = ParameterDisplay::Mapping::LogFrequencyHighPass, .unit = "Hz", .min = 20.0, .max = 20000.0, .label = "Test" };
    for (const double position : { 0.0, 0.3, 0.5, 1.0 }) {
        QCOMPARE(highPass.toUnits(position), lowPass.toUnits(position));
        QCOMPARE(highPass.toPosition(lowPass.toUnits(position)), lowPass.toPosition(lowPass.toUnits(position)));
    }
    QCOMPARE(highPass.mappingName(), std::string { "logFrequencyHighPass" });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ParameterTest)
