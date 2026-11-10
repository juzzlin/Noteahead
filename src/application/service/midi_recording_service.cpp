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

#include "midi_recording_service.hpp"

#include "../../domain/midi/pitch_bend_data.hpp"
#include "../../domain/tracker/song.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "automation_service.hpp"
#include "editor_service.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

using MidiCcMapping::Controller;

//! Highest CC recorded: 120 and up are channel mode messages, not controls.
constexpr uint8_t MaxRecordedCc { 119 };

//! The RPN and NRPN controls: data entry, increment and decrement, and the parameter selects. They
//! address a parameter rather than set one, so only together do they mean anything.
bool isParameterAddressing(uint8_t controller)
{
    return controller == static_cast<uint8_t>(Controller::DataEntryMSB) || controller == static_cast<uint8_t>(Controller::DataEntryLSB) || (controller >= static_cast<uint8_t>(Controller::DataIncrement) && controller <= static_cast<uint8_t>(Controller::RPNMSB));
}

//! The pedals and footswitches: on or off, so a take of one steps rather than ramps.
bool isSwitch(uint8_t controller)
{
    return controller >= static_cast<uint8_t>(Controller::SustainPedal) && controller <= static_cast<uint8_t>(Controller::Hold2);
}

} // namespace

MidiRecordingService::MidiRecordingService(std::shared_ptr<EditorService> editorService, std::shared_ptr<AutomationService> automationService, QObject * parent)
  : QObject { parent }
  , m_editorService { std::move(editorService) }
  , m_automationService { std::move(automationService) }
  , m_clock { [] { return std::chrono::steady_clock::now(); } }
{
    m_idleTimer.setSingleShot(true);
    connect(&m_idleTimer, &QTimer::timeout, this, &MidiRecordingService::finishIdleTakes);
}

MidiRecordingService::~MidiRecordingService() = default;

void MidiRecordingService::setClock(Clock clock)
{
    m_clock = std::move(clock);
}

bool MidiRecordingService::armed() const
{
    return m_armed;
}

void MidiRecordingService::setArmed(bool armed)
{
    if (m_armed != armed) {
        m_armed = armed;
        if (!m_armed) {
            finishTakes();
            m_lineClock.clear();
        }
    }
}

void MidiRecordingService::setTick(quint64 tick, std::chrono::steady_clock::duration lookahead)
{
    if (const auto song = m_editorService->song(); song) {
        if (const auto position = song->songPositionByTick(static_cast<size_t>(tick)); position) {
            m_lineClock.announce(position->pattern, position->line, m_clock() + lookahead);
        }
    }
}

void MidiRecordingService::recordCc(quint8 controller, quint8 value)
{
    if (controller <= MaxRecordedCc && !isParameterAddressing(controller)) {
        record(controller, isSwitch(controller) ? (value >= 64 ? 127.0 : 0.0) : static_cast<double>(value));
    }
}

void MidiRecordingService::recordPitchBend(quint16 value)
{
    record(std::nullopt, PitchBendData { 0, 0, value }.bend() * 100.0);
}

void MidiRecordingService::record(std::optional<uint8_t> controller, double value)
{
    const auto song = m_editorService->song();
    if (!m_armed || !song) {
        return;
    }
    const auto time = m_clock();
    const auto line = m_lineClock.songLine(*song, time);
    if (!line) {
        return;
    }
    auto take = std::ranges::find(m_takes, controller, &Take::controller);
    if (take == m_takes.end()) {
        const auto position = m_editorService->position();
        m_takes.push_back({ controller, static_cast<size_t>(position.track), static_cast<size_t>(position.column), {}, time });
        take = std::prev(m_takes.end());
    }
    take->samples.push_back({ line->first, line->second, value });
    take->lastMessage = time;
    if (!m_idleTimer.isActive()) {
        scheduleIdleCheck();
    }
}

void MidiRecordingService::scheduleIdleCheck()
{
    if (m_takes.empty()) {
        m_idleTimer.stop();
        return;
    }
    const auto stillest = std::ranges::min(m_takes, {}, &Take::lastMessage).lastMessage;
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(stillest + IdleTimeout - m_clock());
    m_idleTimer.start(std::max(remaining, std::chrono::milliseconds { 1 }));
}

void MidiRecordingService::finishIdleTakes()
{
    const auto time = m_clock();
    std::vector<Take> finished;
    std::erase_if(m_takes, [&](auto && take) {
        if (time - take.lastMessage < IdleTimeout) {
            return false;
        }
        finished.push_back(std::move(take));
        return true;
    });
    scheduleIdleCheck();
    finish(std::move(finished));
}

void MidiRecordingService::finishTakes()
{
    m_idleTimer.stop();
    finish(std::exchange(m_takes, {}));
}

bool MidiRecordingService::isRecording() const
{
    return !m_takes.empty();
}

void MidiRecordingService::clearLines(const Take & take, size_t pattern, size_t line0, size_t line1, AutomationSet & additions, AutomationSet & deletions) const
{
    const auto covers = [&](auto && automation) {
        const auto & location = automation.location();
        const auto & interpolation = automation.interpolation();
        return location.pattern() == pattern && location.track() == take.track && location.column() == take.column && interpolation.line0 <= line1 && interpolation.line1 >= line0;
    };
    // The take's own earlier pieces first, so that a looped pattern keeps the last pass, then what
    // the song had
    const auto clear = [&](auto & added, auto & deleted, auto && existing, auto && isSameControl) {
        std::remove_cvref_t<decltype(added)> kept;
        for (auto && automation : added) {
            if (isSameControl(automation) && covers(automation)) {
                std::ranges::move(AutomationTake::remainderOutside(automation, line0, line1), std::back_inserter(kept));
            } else {
                kept.push_back(automation);
            }
        }
        added = std::move(kept);
        for (auto && automation : existing) {
            const bool alreadyDeleted = std::ranges::any_of(deleted, [&](auto && other) { return other.id() == automation.id(); });
            if (!alreadyDeleted && isSameControl(automation) && covers(automation)) {
                deleted.push_back(automation);
                std::ranges::move(AutomationTake::remainderOutside(automation, line0, line1), std::back_inserter(added));
            }
        }
    };
    if (take.controller) {
        clear(additions.midiCc, deletions.midiCc, m_automationService->midiCcAutomations(), [&](const MidiCcAutomation & automation) { return automation.controller() == *take.controller; });
    } else {
        clear(additions.pitchBend, deletions.pitchBend, m_automationService->pitchBendAutomations(), [](const PitchBendAutomation &) { return true; });
    }
}

void MidiRecordingService::finish(const Take & take, AutomationSet & additions, AutomationSet & deletions) const
{
    const auto song = m_editorService->song();
    for (auto && piece : AutomationTake::split(take.samples)) {
        const auto pattern = piece.front().pattern;
        const bool isStepped = take.controller && isSwitch(*take.controller);
        const auto tolerance = take.controller ? CcTolerance : PitchBendTolerance;
        const auto span = AutomationTake::drawnSpan(isStepped ? AutomationTake::steps(piece) : piece, song->lineCount(pattern), tolerance);
        if (!span) {
            continue;
        }
        clearLines(take, pattern, span->line0, span->line1, additions, deletions);
        const AutomationLocation location { pattern, take.track, take.column };
        const double first = span->curve.nodes().front().value;
        const double last = span->curve.nodes().back().value;
        if (take.controller) {
            const auto ccValue = [](double value) { return static_cast<uint8_t>(std::clamp(std::lround(value), 0L, 127L)); };
            additions.midiCc.emplace_back(0, location, *take.controller, MidiCcAutomation::InterpolationParameters { span->line0, span->line1, ccValue(first), ccValue(last), Interpolator::CurveType::Drawn, span->curve }, QString {}, true);
        } else {
            const auto percent = [](double value) { return static_cast<int>(std::clamp(std::lround(value), -100L, 100L)); };
            PitchBendAutomation automation { 0, location, { span->line0, span->line1, percent(first), percent(last), Interpolator::CurveType::Drawn, span->curve }, QString {}, true };
            // Every tick, as finely as it was played, rather than once a line
            automation.setEventsPerBeat(uint8_t { 0 });
            additions.pitchBend.push_back(std::move(automation));
        }
    }
}

void MidiRecordingService::finish(std::vector<Take> takes)
{
    if (takes.empty() || !m_editorService->song()) {
        return;
    }
    AutomationSet additions;
    AutomationSet deletions;
    for (auto && take : takes) {
        finish(take, additions, deletions);
    }
    if (!additions.empty() || !deletions.empty()) {
        m_editorService->requestAutomationEdit(std::move(additions), std::move(deletions));
    }
}

} // namespace noteahead
