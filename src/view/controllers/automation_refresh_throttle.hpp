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

#ifndef AUTOMATION_REFRESH_THROTTLE_HPP
#define AUTOMATION_REFRESH_THROTTLE_HPP

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

namespace noteahead {

//! Turns the automation changes playback announces into refreshes of the dialog showing them.
//!
//! Automation moves a parameter's live value on the audio thread, where nothing may signal, so the
//! playback that schedules a change announces it instead, ahead of time. A refresh is due when the
//! change lands rather than when it was announced, at most once per refresh interval however busy
//! the automation, and once more for the last change of a burst. The timer is single shot and only
//! ever started by an announcement: nothing runs while nothing is automated.
class AutomationRefreshThrottle : public QObject
{
    Q_OBJECT

public:
    //! About as often as a screen redraws a knob. A refresh re-reads every binding on the dialog,
    //! which is not worth doing for each event of a busy automation.
    static constexpr qint64 RefreshIntervalMs { 33 };

    explicit AutomationRefreshThrottle(QObject * parent = nullptr);

    //! A change will land @p delayMs from now.
    void schedule(qint64 delayMs);

signals:
    void refreshDue();

private:
    void refresh();

    QTimer m_timer;
    QElapsedTimer m_clock;
    //! When the latest change that has not been shown yet lands, on m_clock.
    qint64 m_pendingUntil { -1 };
    qint64 m_lastRefresh { -1 };
};

} // namespace noteahead

#endif // AUTOMATION_REFRESH_THROTTLE_HPP
