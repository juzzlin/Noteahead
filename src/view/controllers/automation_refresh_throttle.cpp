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

#include "automation_refresh_throttle.hpp"

#include <algorithm>

namespace noteahead {

AutomationRefreshThrottle::AutomationRefreshThrottle(QObject * parent)
  : QObject { parent }
{
    m_clock.start();
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &AutomationRefreshThrottle::refresh);
}

void AutomationRefreshThrottle::schedule(qint64 delayMs)
{
    const auto now = m_clock.elapsed();
    m_pendingUntil = std::max(m_pendingUntil, now + std::max<qint64>(0, delayMs));
    if (!m_timer.isActive()) {
        const auto earliest = m_lastRefresh < 0 ? now : m_lastRefresh + RefreshIntervalMs;
        m_timer.start(static_cast<int>(std::max<qint64>({ 0, delayMs, earliest - now })));
    }
}

void AutomationRefreshThrottle::refresh()
{
    const auto now = m_clock.elapsed();
    m_lastRefresh = now;
    emit refreshDue();
    // Changes that land after this one still have to be shown once they do
    if (m_pendingUntil > now) {
        m_timer.start(static_cast<int>(std::max(m_pendingUntil - now, RefreshIntervalMs)));
    } else {
        m_pendingUntil = -1;
    }
}

} // namespace noteahead
