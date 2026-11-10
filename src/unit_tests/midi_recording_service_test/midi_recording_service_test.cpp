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

#include "midi_recording_service_test.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/midi_recording_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../domain/tracker/song.hpp"
#include "../../infra/data_service.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

namespace {

using namespace std::chrono_literals;

constexpr quint8 ModWheel { static_cast<quint8>(MidiCcMapping::Controller::ModulationWheelMSB) };
constexpr quint8 Sustain { static_cast<quint8>(MidiCcMapping::Controller::SustainPedal) };
constexpr quint16 BendCentre { 8192 };
constexpr quint16 BendTop { 16383 };

//! A song being played in edit mode, on a clock the test moves
class Session
{
public:
    Session()
      : automationService { std::make_shared<AutomationService>(std::make_shared<PropertyService>()) }
      , editorService { std::make_shared<EditorService>(std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), automationService, std::make_shared<DataService>()) }
      , service { editorService, automationService }
    {
        // What maps the ticks playback announces to lines, as playing the song does
        editorService->song()->renderToEvents(automationService, std::make_shared<SideChainService>(), 0);
        service.setClock([this] { return now; });
        service.setArmed(true);
    }

    std::chrono::duration<double> lineDuration() const
    {
        const auto song = editorService->song();
        return std::chrono::duration<double> { 60.0 / static_cast<double>(song->beatsPerMinute() * song->linesPerBeat()) };
    }

    //! Lines the song plays while a controller is still for long enough to end its take
    double idleLines() const
    {
        return std::ceil(std::chrono::duration<double>(MidiRecordingService::IdleTimeout) / lineDuration()) + 1.0;
    }

    //! Plays on to @p line of the first pattern, letting the idle timer fire on the way as it would
    void playTo(double line)
    {
        while (position + 0.1 <= line + 1.0e-9) {
            position += 0.1;
            now = start + std::chrono::duration_cast<std::chrono::steady_clock::duration>(lineDuration() * position);
            if (const double whole = std::round(position); std::abs(position - whole) < 1.0e-9) {
                service.setTick(static_cast<quint64>(whole) * editorService->song()->ticksPerLine(), 0s);
            }
            service.finishIdleTakes();
        }
    }

    std::shared_ptr<AutomationService> automationService;
    std::shared_ptr<EditorService> editorService;
    MidiRecordingService service;
    std::chrono::steady_clock::time_point start { std::chrono::steady_clock::now() };
    std::chrono::steady_clock::time_point now { start };
    double position = -0.1;
};

} // namespace

void MidiRecordingServiceTest::test_cc_shouldRecordADrawnCurveOnTheCursorColumn()
{
    Session session;
    session.editorService->requestPosition(0, 1, 0, 0, 0);
    session.playTo(4.0);
    session.service.recordCc(ModWheel, 0);
    session.playTo(5.0);
    session.service.recordCc(ModWheel, 64);
    session.playTo(6.0);
    session.service.recordCc(ModWheel, 127);
    QVERIFY(session.service.isRecording());
    session.playTo(6.0 + session.idleLines());
    QVERIFY(!session.service.isRecording());

    const auto automations = session.automationService->midiCcAutomations();
    QCOMPARE(automations.size(), size_t { 1 });
    const auto & automation = automations.front();
    QCOMPARE(automation.controller(), ModWheel);
    QCOMPARE(automation.location().pattern(), size_t { 0 });
    QCOMPARE(automation.location().track(), size_t { 1 });
    QCOMPARE(automation.location().column(), size_t { 0 });
    QCOMPARE(automation.interpolation().curve, Interpolator::CurveType::Drawn);
    QCOMPARE(automation.interpolation().line0, size_t { 4 });
    QCOMPARE(automation.interpolation().value0, uint8_t { 0 });
    QCOMPARE(automation.interpolation().value1, uint8_t { 127 });
    QVERIFY(std::abs(automation.interpolation().drawnCurve.valueAt(5.0) - 64.0) < 1.0);
}

void MidiRecordingServiceTest::test_cc_sustainPedal_shouldStepOnTheNearestLine()
{
    Session session;
    session.playTo(2.4);
    session.service.recordCc(Sustain, 127);
    session.playTo(5.6);
    session.service.recordCc(Sustain, 0);
    session.playTo(6.0 + session.idleLines());

    const auto automations = session.automationService->midiCcAutomations();
    QCOMPARE(automations.size(), size_t { 1 });
    const auto & interpolation = automations.front().interpolation();
    QCOMPARE(interpolation.line0, size_t { 2 });
    QCOMPARE(interpolation.value0, uint8_t { 127 });
    // Held down to the line it was let go nearest to, and up on it: never half way
    QCOMPARE(interpolation.drawnCurve.valueAt(5.0), 127.0);
    QCOMPARE(interpolation.drawnCurve.valueAt(6.0), 0.0);
}

void MidiRecordingServiceTest::test_cc_channelModeAndParameterAddressing_shouldRecordNothing()
{
    Session session;
    session.playTo(1.0);
    session.service.recordCc(static_cast<quint8>(MidiCcMapping::Controller::ResetAllControllers), 0);
    session.service.recordCc(static_cast<quint8>(MidiCcMapping::Controller::RPNMSB), 0);
    session.service.recordCc(static_cast<quint8>(MidiCcMapping::Controller::DataEntryMSB), 2);
    QVERIFY(!session.service.isRecording());
}

void MidiRecordingServiceTest::test_pitchBend_shouldRecordEveryTickInPercent()
{
    Session session;
    session.playTo(1.0);
    session.service.recordPitchBend(BendCentre);
    session.playTo(2.0);
    session.service.recordPitchBend(BendTop);
    session.playTo(3.0);
    session.service.recordPitchBend(BendCentre);
    session.playTo(3.0 + session.idleLines());

    QVERIFY(session.automationService->midiCcAutomations().empty());
    const auto automations = session.automationService->pitchBendAutomations();
    QCOMPARE(automations.size(), size_t { 1 });
    const auto & automation = automations.front();
    QCOMPARE(automation.eventsPerBeat(), std::optional<uint8_t> { 0 });
    QCOMPARE(automation.interpolation().line0, size_t { 1 });
    QCOMPARE(automation.interpolation().value0, 0);
    QVERIFY(std::abs(automation.interpolation().drawnCurve.valueAt(2.0) - 100.0) < 0.1);
    QCOMPARE(automation.interpolation().value1, 0);
}

void MidiRecordingServiceTest::test_take_still_shouldEndAndTheNextMoveStartAnother()
{
    Session session;
    session.playTo(1.0);
    session.service.recordCc(ModWheel, 100);
    session.playTo(2.0 + session.idleLines());
    QVERIFY(!session.service.isRecording());
    QCOMPARE(session.automationService->midiCcAutomations().size(), size_t { 1 });

    const double later = session.position + 2.0;
    session.playTo(later);
    session.service.recordCc(ModWheel, 20);
    QVERIFY(session.service.isRecording());
    session.playTo(later + session.idleLines());
    QCOMPARE(session.automationService->midiCcAutomations().size(), size_t { 2 });
}

void MidiRecordingServiceTest::test_take_notArmed_shouldRecordNothing()
{
    Session session;
    session.service.setArmed(false);
    session.playTo(1.0);
    session.service.recordCc(ModWheel, 100);
    session.service.recordPitchBend(BendTop);
    QVERIFY(!session.service.isRecording());
    session.playTo(2.0 + session.idleLines());
    QVERIFY(session.automationService->midiCcAutomations().empty());
    QVERIFY(session.automationService->pitchBendAutomations().empty());
}

void MidiRecordingServiceTest::test_take_overAnAutomation_shouldReplaceOnlyItsControllerOverTheLinesItCovers()
{
    Session session;
    session.automationService->addMidiCcAutomation({ 0, AutomationLocation { 0, 0, 0 }, ModWheel, { 0, 16, 0, 127 }, {} });
    session.automationService->addMidiCcAutomation({ 0, AutomationLocation { 0, 0, 0 }, Sustain, { 0, 16, 127, 127 }, {} });

    session.playTo(6.0);
    session.service.recordCc(ModWheel, 10);
    session.playTo(8.0);
    session.service.recordCc(ModWheel, 90);
    session.playTo(8.0 + session.idleLines());

    auto automations = session.automationService->midiCcAutomations();
    QCOMPARE(automations.size(), size_t { 4 });
    std::erase_if(automations, [](auto && automation) { return automation.controller() != ModWheel; });
    QCOMPARE(automations.size(), size_t { 3 });
    std::ranges::sort(automations, {}, [](auto && automation) { return automation.interpolation().line0; });
    QCOMPARE(automations.at(0).interpolation().line1, size_t { 5 });
    QCOMPARE(automations.at(1).interpolation().line0, size_t { 6 });
    QCOMPARE(automations.at(2).interpolation().line0, automations.at(1).interpolation().line1 + 1);
    QCOMPARE(automations.at(2).interpolation().line1, size_t { 16 });
    // What is left plays as it did
    QCOMPARE(automations.at(2).interpolation().value1, uint8_t { 127 });

    session.editorService->undo();
    QCOMPARE(session.automationService->midiCcAutomations().size(), size_t { 2 });
}

void MidiRecordingServiceTest::test_setArmed_off_shouldKeepEveryTakeInOneUndoStep()
{
    Session session;
    session.playTo(1.0);
    session.service.recordCc(ModWheel, 100);
    session.service.recordPitchBend(BendTop);
    session.playTo(3.0);
    session.service.recordCc(ModWheel, 50);
    session.service.setArmed(false);
    QVERIFY(!session.service.isRecording());
    QCOMPARE(session.automationService->midiCcAutomations().size(), size_t { 1 });
    QCOMPARE(session.automationService->pitchBendAutomations().size(), size_t { 1 });

    QVERIFY(session.editorService->canUndo());
    session.editorService->undo();
    QVERIFY(session.automationService->midiCcAutomations().empty());
    QVERIFY(session.automationService->pitchBendAutomations().empty());
    QVERIFY(!session.editorService->canUndo());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::MidiRecordingServiceTest)
