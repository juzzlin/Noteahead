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

#include "xy_pad_recording_service.hpp"

#include "../../domain/devices/xy_pad_device.hpp"
#include "../../domain/tracker/song.hpp"
#include "../position.hpp"
#include "automation_service.hpp"
#include "device_service.hpp"
#include "editor_service.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! How many announced lines are kept: far more than any lookahead spans.
constexpr size_t MaxAnchors { 16 };
//! An axis that stayed this close to the centre the whole take was not moved.
constexpr double CentreThreshold { 0.002 };

double knobPosition(double padPosition)
{
    return std::clamp((padPosition + 1.0) * 0.5, 0.0, 1.0);
}

} // namespace

XyPadRecordingService::XyPadRecordingService(std::shared_ptr<DeviceService> deviceService, std::shared_ptr<EditorService> editorService, std::shared_ptr<AutomationService> automationService, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
  , m_editorService { std::move(editorService) }
  , m_automationService { std::move(automationService) }
  , m_clock { [] { return std::chrono::steady_clock::now(); } }
{
    m_sampleTimer.setInterval(SampleInterval);
    connect(&m_sampleTimer, &QTimer::timeout, this, qOverload<>(&XyPadRecordingService::sample));
}

XyPadRecordingService::~XyPadRecordingService() = default;

void XyPadRecordingService::setClock(Clock clock)
{
    m_clock = std::move(clock);
}

bool XyPadRecordingService::armed() const
{
    return m_armed;
}

void XyPadRecordingService::setArmed(bool armed)
{
    if (m_armed != armed) {
        m_armed = armed;
        if (!m_armed) {
            finishTakes();
            m_anchors.clear();
        }
        emit armedChanged();
    }
}

void XyPadRecordingService::setTick(quint64 tick, std::chrono::steady_clock::duration lookahead)
{
    const auto song = m_editorService->song();
    if (!song) {
        return;
    }
    if (const auto position = song->songPositionByTick(static_cast<size_t>(tick)); position) {
        m_anchors.push_back({ position->pattern, position->line, m_clock() + lookahead });
        while (m_anchors.size() > MaxAnchors) {
            m_anchors.pop_front();
        }
    }
}

std::optional<std::pair<size_t, double>> XyPadRecordingService::songLine(std::chrono::steady_clock::time_point time) const
{
    const auto song = m_editorService->song();
    if (!song || !song->beatsPerMinute() || !song->linesPerBeat()) {
        return std::nullopt;
    }
    const auto lineDuration = std::chrono::duration<double>(60.0 / static_cast<double>(song->beatsPerMinute() * song->linesPerBeat()));
    // The last line already heard, and how far into it the song is, short of the next one
    for (auto anchor = m_anchors.rbegin(); anchor != m_anchors.rend(); anchor++) {
        if (anchor->due <= time) {
            const double into = std::chrono::duration<double>(time - anchor->due) / lineDuration;
            return std::pair { anchor->pattern, static_cast<double>(anchor->line) + std::clamp(into, 0.0, 0.999) };
        }
    }
    return std::nullopt;
}

void XyPadRecordingService::startTake(const QString & deviceUuid, int pad)
{
    if (!m_armed || pad < 0 || static_cast<size_t>(pad) >= XyPadDevice::PadCount) {
        return;
    }
    const auto device = std::dynamic_pointer_cast<XyPadDevice>(m_deviceService->findDevice(deviceUuid.toStdString()));
    if (!device) {
        return;
    }
    const auto uuid = deviceUuid.toStdString();
    if (std::ranges::any_of(m_takes, [&](auto && take) { return take.deviceUuid == uuid && take.pad == static_cast<size_t>(pad); })) {
        return;
    }
    const auto position = m_editorService->position();
    Take take { device, uuid, static_cast<size_t>(pad), static_cast<size_t>(position.track), static_cast<size_t>(position.column), {}, {} };
    // Where the pad stood before the hand moved it, which is where the curve starts from
    sample(take, m_clock());
    m_takes.push_back(std::move(take));
    if (!m_sampleTimer.isActive()) {
        m_sampleTimer.start();
    }
}

void XyPadRecordingService::sample(Take & take, std::chrono::steady_clock::time_point time)
{
    const auto device = take.device.lock();
    if (const auto line = songLine(time); device && line) {
        take.x.push_back({ line->first, line->second, knobPosition(device->positionX(take.pad)) });
        take.y.push_back({ line->first, line->second, knobPosition(device->positionY(take.pad)) });
    }
}

void XyPadRecordingService::sample()
{
    const auto time = m_clock();
    std::vector<Take> finished;
    std::erase_if(m_takes, [&](auto && take) {
        sample(take, time);
        const auto device = take.device.lock();
        if (device && device->isHandActive(take.pad)) {
            return false;
        }
        finished.push_back(std::move(take));
        return true;
    });
    if (m_takes.empty()) {
        m_sampleTimer.stop();
    }
    for (auto && take : finished) {
        finish(take);
    }
}

void XyPadRecordingService::finishTakes()
{
    m_sampleTimer.stop();
    for (auto && take : std::exchange(m_takes, {})) {
        finish(take);
    }
}

bool XyPadRecordingService::isRecording() const
{
    return !m_takes.empty();
}

void XyPadRecordingService::clearLines(const ParameterAutomation::Target & target, size_t pattern, size_t line0, size_t line1, AutomationSet & additions, AutomationSet & deletions) const
{
    const auto covers = [&](const ParameterAutomation & automation) {
        const auto & interpolation = automation.interpolation();
        return automation.target() == target && automation.location().pattern() == pattern && interpolation.line0 <= line1 && interpolation.line1 >= line0;
    };
    // The take's own earlier pieces first: a looped pattern keeps the last pass
    std::vector<ParameterAutomation> added;
    for (auto && automation : additions.parameter) {
        if (covers(automation)) {
            std::ranges::move(AutomationTake::remainderOutside(automation, line0, line1), std::back_inserter(added));
        } else {
            added.push_back(automation);
        }
    }
    additions.parameter = std::move(added);
    for (auto && automation : m_automationService->parameterAutomations()) {
        const bool deleted = std::ranges::any_of(deletions.parameter, [&](auto && other) { return other.id() == automation.id(); });
        if (!deleted && covers(automation)) {
            deletions.parameter.push_back(automation);
            std::ranges::move(AutomationTake::remainderOutside(automation, line0, line1), std::back_inserter(additions.parameter));
        }
    }
}

void XyPadRecordingService::finish(const Take & take)
{
    const auto song = m_editorService->song();
    if (!song) {
        return;
    }
    AutomationSet additions;
    AutomationSet deletions;
    for (auto axis : { XyPadDevice::Axis::X, XyPadDevice::Axis::Y }) {
        const auto & samples = axis == XyPadDevice::Axis::X ? take.x : take.y;
        if (std::ranges::all_of(samples, [](auto && sample) { return std::abs(sample.value - 0.5) < CentreThreshold; })) {
            continue;
        }
        const ParameterAutomation::Target target { ParameterAutomation::Target::Owner::Device, take.deviceUuid, XyPadDevice::positionKey(take.pad, axis) };
        for (auto && piece : AutomationTake::split(samples)) {
            const auto pattern = piece.front().pattern;
            if (const auto curve = AutomationTake::curve(piece, song->lineCount(pattern), Tolerance); curve) {
                clearLines(target, pattern, curve->line0, curve->line1, additions, deletions);
                additions.parameter.emplace_back(0, AutomationLocation { pattern, take.track, take.column }, target, ParameterAutomation::Mode::Absolute, *curve, ModulationParameters {}, QString {}, true);
            }
        }
    }
    m_editorService->requestAutomationEdit(std::move(additions), std::move(deletions));
}

} // namespace noteahead
