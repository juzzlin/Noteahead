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

#include "session_timer_service.hpp"

#include "../../common/constants.hpp"
#include "../../domain/tracker/song.hpp"
#include "editor_service.hpp"
#include "settings_service.hpp"

#include <QElapsedTimer>
#include <QEvent>

#include <algorithm>

namespace noteahead {

namespace {

constexpr int tickIntervalMs { 1000 };

qint64 monotonicMs()
{
    static QElapsedTimer timer;
    if (!timer.isValid()) {
        timer.start();
    }
    return timer.elapsed();
}

} // namespace

SessionTimerService::SessionTimerService(std::shared_ptr<EditorService> editorService, std::shared_ptr<SettingsService> settingsService, Clock clock, QObject * parent)
  : QObject { parent }
  , m_editorService { std::move(editorService) }
  , m_settingsService { std::move(settingsService) }
  , m_clock { clock ? std::move(clock) : Clock { monotonicMs } }
{
    m_timer.setInterval(tickIntervalMs);
    connect(&m_timer, &QTimer::timeout, this, &SessionTimerService::tick);
    if (m_editorService) {
        // A new or loaded song starts a new session, with whatever total it brings
        connect(m_editorService.get(), &EditorService::songChanged, this, &SessionTimerService::startSession);
    }
    startSession();
}

int SessionTimerService::sessionSeconds() const
{
    return m_sessionSeconds;
}

int SessionTimerService::totalSeconds() const
{
    const auto song = m_editorService ? m_editorService->song() : nullptr;
    return song ? static_cast<int>(song->timeSpentSeconds()) : 0;
}

bool SessionTimerService::idle() const
{
    return m_idle;
}

void SessionTimerService::resetTotal()
{
    if (const auto song = m_editorService ? m_editorService->song() : nullptr; song && song->timeSpentSeconds() > 0) {
        song->setTimeSpentSeconds(0);
        m_editorService->setIsModified(true);
        emit timeChanged();
    }
}

void SessionTimerService::notifyActivity()
{
    const auto now = m_clock();
    m_lastActivityMs = now;
    if (m_idle) {
        // The idle stretch is not counted: the next tick measures from here
        m_lastTickMs = now;
        setIdle(false);
    }
}

void SessionTimerService::setPlaying(bool playing)
{
    m_playing = playing;
    if (playing) {
        notifyActivity();
    }
}

qint64 SessionTimerService::timeoutMs() const
{
    const int minutes = m_settingsService ? m_settingsService->idleTimeoutMinutes() : Constants::defaultIdleTimeoutMinutes();
    return static_cast<qint64>(minutes) * 60 * 1000;
}

void SessionTimerService::tick()
{
    if (m_idle) {
        return;
    }
    const auto now = m_clock();
    const auto elapsed = now - m_lastTickMs;
    m_lastTickMs = now;
    if (m_playing) {
        m_lastActivityMs = now;
    }
    // A gap longer than the timeout between two ticks is the machine having slept, not work
    if (elapsed > timeoutMs()) {
        m_carryMs = 0;
        setIdle(true);
        return;
    }
    m_carryMs += std::max<qint64>(0, elapsed);
    if (const auto seconds = m_carryMs / 1000; seconds > 0) {
        m_carryMs %= 1000;
        m_sessionSeconds += static_cast<int>(seconds);
        if (const auto song = m_editorService ? m_editorService->song() : nullptr) {
            song->setTimeSpentSeconds(song->timeSpentSeconds() + static_cast<size_t>(seconds));
        }
        emit timeChanged();
    }
    if (now - m_lastActivityMs >= timeoutMs()) {
        setIdle(true);
    }
}

bool SessionTimerService::eventFilter(QObject * watched, QEvent * event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::Wheel:
    case QEvent::TouchBegin:
        notifyActivity();
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

void SessionTimerService::startSession()
{
    const auto now = m_clock();
    m_sessionSeconds = 0;
    m_carryMs = 0;
    m_lastTickMs = now;
    m_lastActivityMs = now;
    setIdle(false);
    emit timeChanged();
}

void SessionTimerService::setIdle(bool idle)
{
    if (idle) {
        m_timer.stop();
    } else if (!m_timer.isActive()) {
        m_timer.start();
    }
    if (m_idle != idle) {
        m_idle = idle;
        emit idleChanged();
    }
}

} // namespace noteahead
