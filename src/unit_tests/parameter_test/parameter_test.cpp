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

#include "../../domain/tracker/parameter.hpp"
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
    const ParameterDisplay gain { ParameterDisplay::Mapping::Linear, "dB", -12.0, 12.0, ParameterDisplay::Rest::Centre };

    QCOMPARE(gain.toUnits(0.5), 0.0);
    QCOMPARE(gain.toUnits(1.0), 12.0);
    QCOMPARE(gain.toPosition(-6.0), 0.25);
}

void ParameterTest::test_display_curvedMappings_shouldRoundTrip()
{
    // Every curve has to come back to the position it started from, or a value typed in units
    // would land somewhere other than where it reads.
    const std::vector<ParameterDisplay> displays {
        { ParameterDisplay::Mapping::Exponential, "ms", 0.1, 500.0 },
        { ParameterDisplay::Mapping::CubicCentered, "%", -100.0, 100.0, ParameterDisplay::Rest::Centre },
        { ParameterDisplay::Mapping::LogFrequency, "Hz", 20.0, 20000.0 },
        { ParameterDisplay::Mapping::LfoFrequency, "Hz", 0.05, 20.0 }
    };
    for (const auto & display : displays) {
        for (const double position : { 0.0, 0.1, 0.37, 0.5, 0.82, 1.0 }) {
            QVERIFY2(std::abs(display.toPosition(display.toUnits(position)) - position) < 1.0e-9, display.mappingName().c_str());
        }
    }

    // The log curve doubles in equal steps: the middle of 20 Hz to 20 kHz is their geometric mean.
    const ParameterDisplay cutoff { ParameterDisplay::Mapping::LogFrequency, "Hz", 20.0, 20000.0 };
    QVERIFY(std::abs(cutoff.toUnits(0.5) - std::sqrt(20.0 * 20000.0)) < 1.0e-6);
}

void ParameterTest::test_display_toPosition_outOfRange_shouldClamp()
{
    const ParameterDisplay cutoff { ParameterDisplay::Mapping::LogFrequency, "Hz", 20.0, 20000.0 };

    QCOMPARE(cutoff.toPosition(5.0), 0.0);
    QCOMPARE(cutoff.toPosition(40000.0), 1.0);
}

void ParameterTest::test_display_restPosition_shouldFollowTheRest()
{
    QCOMPARE((ParameterDisplay { ParameterDisplay::Mapping::Linear, "%", 0.0, 100.0 }.restPosition()), 0.0);
    QCOMPARE((ParameterDisplay { ParameterDisplay::Mapping::CubicCentered, "%", -100.0, 100.0, ParameterDisplay::Rest::Centre }.restPosition()), 0.5);
}

void ParameterTest::test_container_setParameterDisplay_shouldAttachIt()
{
    ParameterContainer container;
    container.addParameter(continuousParameter());
    QVERIFY(!container.parameter("cutoff")->get().display().has_value());

    container.setParameterDisplay("cutoff", { ParameterDisplay::Mapping::LogFrequency, "Hz", 20.0, 20000.0 });

    const auto & display = container.parameter("cutoff")->get().display();
    QVERIFY(display.has_value());
    QCOMPARE(display->unit, std::string { "Hz" });
    // An unknown name is ignored rather than creating a parameter.
    container.setParameterDisplay("missing", {});
    QVERIFY(!container.parameter("missing").has_value());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ParameterTest)
