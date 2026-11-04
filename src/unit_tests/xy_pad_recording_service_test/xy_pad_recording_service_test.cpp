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

#include "xy_pad_recording_service_test.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../application/service/xy_pad_recording_service.hpp"
#include "../../domain/devices/xy_pad_device.hpp"
#include "../../domain/tracker/song.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

using Axis = XyPadDevice::Axis;
using namespace std::chrono_literals;

constexpr double SampleRate { 48000.0 };

//! A song being played in edit mode, with an XY pad device, on a clock the test moves
class Session
{
public:
    Session()
      : automationService { std::make_shared<AutomationService>(std::make_shared<PropertyService>()) }
      , editorService { std::make_shared<EditorService>(std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), automationService, std::make_shared<DataService>()) }
      , deviceService { std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>()) }
      , device { std::make_shared<XyPadDevice>("XY") }
      , service { deviceService, editorService, automationService }
    {
        deviceService->setDevice(0, device);
        // What maps the ticks playback announces to lines, as playing the song does
        editorService->song()->renderToEvents(automationService, std::make_shared<SideChainService>(), 0);
        service.setClock([this] { return now; });
        service.setArmed(true);
        // A pad that stays where the hand leaves it, so that a take ends as the hand lifts
        device->updatePadParameter(0, Constants::NahdXml::xmlKeyReturnTime(), 0.0f);
    }

    std::chrono::duration<double> lineDuration() const
    {
        const auto song = editorService->song();
        return std::chrono::duration<double> { 60.0 / static_cast<double>(song->beatsPerMinute() * song->linesPerBeat()) };
    }

    //! Plays on to @p line of the first pattern, sampling the takes on the way as the timer would
    void playTo(double line)
    {
        while (position + 0.25 <= line) {
            position += 0.25;
            now = start + std::chrono::duration_cast<std::chrono::steady_clock::duration>(lineDuration() * position);
            if (std::floor(position) == position) {
                service.setTick(static_cast<quint64>(position) * editorService->song()->ticksPerLine(), 0s);
            }
            const auto frames = static_cast<uint32_t>(SampleRate * lineDuration().count() * 0.25);
            std::vector<ModulationSource::Step> steps;
            device->evaluate(frame, frames, SampleRate, 120.0, steps);
            frame += frames;
            service.sample();
        }
    }

    void press(float x, float y)
    {
        device->press(0, x, y);
        service.startTake(QString::fromStdString(device->uuid()), 0);
    }

    std::shared_ptr<AutomationService> automationService;
    std::shared_ptr<EditorService> editorService;
    std::shared_ptr<DeviceService> deviceService;
    std::shared_ptr<XyPadDevice> device;
    XyPadRecordingService service;
    std::chrono::steady_clock::time_point start { std::chrono::steady_clock::now() };
    std::chrono::steady_clock::time_point now { start };
    double position = -0.25;
    uint64_t frame = 0;
};

ParameterAutomation::Target positionTarget(const XyPadDevice & device, Axis axis)
{
    return { ParameterAutomation::Target::Owner::Device, device.uuid(), XyPadDevice::positionKey(0, axis) };
}

} // namespace

void XyPadRecordingServiceTest::test_take_shouldRecordADrawnCurveOnTheCursorColumn()
{
    Session session;
    session.editorService->requestPosition(0, 1, 0, 0, 0);
    session.playTo(4.0);
    session.press(1.0f, 0.0f);
    session.playTo(5.0);
    session.press(1.0f, 1.0f);
    session.service.sample();
    session.playTo(6.0);
    session.device->release(0);
    QVERIFY(session.service.isRecording());
    session.playTo(7.0);
    QVERIFY(!session.service.isRecording());

    const auto automations = session.automationService->parameterAutomations();
    QCOMPARE(automations.size(), size_t { 2 });
    for (auto && automation : automations) {
        QCOMPARE(automation.location().pattern(), size_t { 0 });
        QCOMPARE(automation.location().track(), size_t { 1 });
        QCOMPARE(automation.location().column(), size_t { 0 });
        QCOMPARE(automation.mode(), ParameterAutomation::Mode::Absolute);
        QCOMPARE(automation.interpolation().curve, Interpolator::CurveType::Drawn);
        QCOMPARE(automation.interpolation().line0, size_t { 4 });
    }
    const auto x = std::ranges::find_if(automations, [&](auto && automation) { return automation.target() == positionTarget(*session.device, Axis::X); });
    QVERIFY(x != automations.end());
    // From the centre where the hand found it to the right edge where it held it
    QCOMPARE(x->interpolation().value0, 0.5);
    QVERIFY(std::abs(x->interpolation().drawnCurve.valueAt(5.5) - 1.0) < 0.01);
    QVERIFY(std::abs(x->interpolation().value1 - 1.0) < 0.01);
}

void XyPadRecordingServiceTest::test_take_shouldBeUndoneInOneStep()
{
    Session session;
    session.playTo(2.0);
    session.press(0.5f, -0.5f);
    session.playTo(4.0);
    session.device->release(0);
    session.playTo(5.0);
    QCOMPARE(session.automationService->parameterAutomations().size(), size_t { 2 });

    QVERIFY(session.editorService->canUndo());
    session.editorService->undo();
    QVERIFY(session.automationService->parameterAutomations().empty());
    QVERIFY(!session.editorService->canUndo());
}

void XyPadRecordingServiceTest::test_take_untouchedAxis_shouldRecordNothingForIt()
{
    Session session;
    session.playTo(1.0);
    session.press(0.0f, 0.7f);
    session.playTo(3.0);
    session.device->release(0);
    session.playTo(4.0);

    const auto automations = session.automationService->parameterAutomations();
    QCOMPARE(automations.size(), size_t { 1 });
    QCOMPARE(automations.front().target(), positionTarget(*session.device, Axis::Y));
}

void XyPadRecordingServiceTest::test_take_notArmed_shouldRecordNothing()
{
    Session session;
    session.service.setArmed(false);
    session.playTo(1.0);
    session.press(0.5f, 0.5f);
    QVERIFY(!session.service.isRecording());
    session.playTo(3.0);
    session.device->release(0);
    session.playTo(4.0);
    QVERIFY(session.automationService->parameterAutomations().empty());
}

void XyPadRecordingServiceTest::test_take_overAnAutomation_shouldReplaceTheLinesItCovers()
{
    Session session;
    const auto target = positionTarget(*session.device, Axis::X);
    session.automationService->addParameterAutomation({ 0, AutomationLocation { 0, 2, 0 }, target, ParameterAutomation::Mode::Absolute, { 0, 16, 0.0, 1.0 }, {}, {}, true });

    session.playTo(6.0);
    session.press(-1.0f, 0.0f);
    session.playTo(8.0);
    session.device->release(0);
    session.playTo(9.0);

    auto automations = session.automationService->parameterAutomations();
    QCOMPARE(automations.size(), size_t { 3 });
    std::ranges::sort(automations, {}, [](auto && automation) { return automation.interpolation().line0; });
    QCOMPARE(automations.at(0).interpolation().line1, size_t { 5 });
    QCOMPARE(automations.at(0).location().track(), size_t { 2 });
    QCOMPARE(automations.at(1).interpolation().line0, size_t { 6 });
    QCOMPARE(automations.at(2).interpolation().line0, automations.at(1).interpolation().line1 + 1);
    QCOMPARE(automations.at(2).interpolation().line1, size_t { 16 });
    // What is left plays as it did
    QCOMPARE(automations.at(2).interpolation().value1, 1.0);

    session.editorService->undo();
    automations = session.automationService->parameterAutomations();
    QCOMPARE(automations.size(), size_t { 1 });
    QCOMPARE(automations.front().interpolation().line1, size_t { 16 });
}

void XyPadRecordingServiceTest::test_setArmed_off_shouldKeepTheTakeSoFar()
{
    Session session;
    session.playTo(1.0);
    session.press(0.5f, 0.0f);
    session.playTo(3.0);
    session.service.setArmed(false);
    QVERIFY(!session.service.isRecording());
    QCOMPARE(session.automationService->parameterAutomations().size(), size_t { 1 });
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::XyPadRecordingServiceTest)
