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

#ifndef SESSION_TIMER_SERVICE_HPP
#define SESSION_TIMER_SERVICE_HPP

#include <QObject>
#include <QTimer>

#include <functional>
#include <memory>

namespace noteahead {

class EditorService;
class SettingsService;

//! Counts the time spent working on the project: this session's, and the song's total across every
//! session, which the song saves.
//!
//! Working means input in the window or the song playing, since listening back is work too. After
//! the idle timeout without either the count stops, and picks up again on the next input. It does
//! not take the waited minutes back off, so a pause adds at most one timeout's worth.
//!
//! Time passing is not an edit: it never marks the project modified, and is saved only with a save
//! made for some other reason.
class SessionTimerService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int sessionSeconds READ sessionSeconds NOTIFY timeChanged)
    Q_PROPERTY(int totalSeconds READ totalSeconds NOTIFY timeChanged)
    Q_PROPERTY(bool idle READ idle NOTIFY idleChanged)

public:
    //! A monotonic clock in milliseconds, replaceable for tests.
    using Clock = std::function<qint64()>;

    SessionTimerService(std::shared_ptr<EditorService> editorService, std::shared_ptr<SettingsService> settingsService, Clock clock = {}, QObject * parent = nullptr);

    int sessionSeconds() const;
    int totalSeconds() const;
    bool idle() const;

    //! Sets the song's total back to zero, as for a template or a song started over. An edit to the
    //! project, unlike the counting.
    Q_INVOKABLE void resetTotal();

    //! Input, or anything else that shows someone is at work.
    void notifyActivity();
    void setPlaying(bool playing);

    //! Counts the time since the last tick. Driven by a timer while counting; public for the tests.
    void tick();

    //! Watches the application's input for activity. Never takes an event.
    bool eventFilter(QObject * watched, QEvent * event) override;

signals:
    void timeChanged();
    void idleChanged();

private:
    void startSession();
    void setIdle(bool idle);
    qint64 timeoutMs() const;

    std::shared_ptr<EditorService> m_editorService;
    std::shared_ptr<SettingsService> m_settingsService;
    Clock m_clock;
    QTimer m_timer;

    qint64 m_lastTickMs { 0 };
    qint64 m_lastActivityMs { 0 };
    //! What a tick counted short of a whole second, for the next one.
    qint64 m_carryMs { 0 };
    int m_sessionSeconds { 0 };
    bool m_idle { false };
    bool m_playing { false };
};

} // namespace noteahead

#endif // SESSION_TIMER_SERVICE_HPP
