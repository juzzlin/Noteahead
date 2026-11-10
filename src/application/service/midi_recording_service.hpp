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

#ifndef MIDI_RECORDING_SERVICE_HPP
#define MIDI_RECORDING_SERVICE_HPP

#include <QObject>
#include <QTimer>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "../../domain/tracker/automation_set.hpp"
#include "../../domain/tracker/automation_take.hpp"
#include "playback_line_clock.hpp"

namespace noteahead {

class AutomationService;
class EditorService;

//! Records the controller keyboard's CCs and pitch bend while the song plays in edit mode, as notes
//! are recorded.
//!
//! A take of a controller lasts from its first message until it has been still for IdleTimeout.
//! Each take becomes a drawn MIDI CC or pitch bend automation on the column under the cursor, one
//! per pattern played through, so it is edited as any drawn curve is, and plays back through
//! whatever the device does with the controller -- its MIDI mappings included. A take replaces what
//! automated the same controller of the column over the lines it covers, and is undone in one step.
class MidiRecordingService : public QObject
{
    Q_OBJECT

public:
    using Clock = std::function<std::chrono::steady_clock::time_point()>;

    MidiRecordingService(std::shared_ptr<EditorService> editorService, std::shared_ptr<AutomationService> automationService, QObject * parent = nullptr);
    ~MidiRecordingService() override;

    //! What time it is. The steady clock unless a test says otherwise.
    void setClock(Clock clock);

    bool armed() const;
    //! Armed while the song plays in edit mode. Disarming ends every take.
    void setArmed(bool armed);

    //! The song has reached @p tick, which is due @p lookahead from now. What the takes are timed by.
    void setTick(quint64 tick, std::chrono::steady_clock::duration lookahead);

    //! A CC came in from the controller keyboard.
    void recordCc(quint8 controller, quint8 value);
    //! A pitch bend came in from the controller keyboard, 0 to 16383 with the centre at 8192.
    void recordPitchBend(quint16 value);

    //! Ends the takes that have been still for IdleTimeout.
    void finishIdleTakes();
    //! Ends every take, keeping what was recorded.
    void finishTakes();

    bool isRecording() const;

    //! How long a controller stays still before its take ends.
    static constexpr std::chrono::milliseconds IdleTimeout { 1000 };
    //! How far a recorded curve may stray from the take, in CC steps and in percent of the bend.
    static constexpr double CcTolerance { 0.5 };
    static constexpr double PitchBendTolerance { 0.1 };

private:
    struct Take
    {
        //! Nothing for pitch bend
        std::optional<uint8_t> controller;
        size_t track = 0;
        size_t column = 0;
        AutomationTake::SampleList samples;
        std::chrono::steady_clock::time_point lastMessage;
    };

    void record(std::optional<uint8_t> controller, double value);
    void finish(std::vector<Take> takes);
    void finish(const Take & take, AutomationSet & additions, AutomationSet & deletions) const;
    //! Takes what automates @p take's controller of its column in @p pattern off lines @p line0 to
    //! @p line1: out of the song into @p deletions, and out of earlier pieces in @p additions.
    void clearLines(const Take & take, size_t pattern, size_t line0, size_t line1, AutomationSet & additions, AutomationSet & deletions) const;
    void scheduleIdleCheck();

    std::shared_ptr<EditorService> m_editorService;
    std::shared_ptr<AutomationService> m_automationService;
    Clock m_clock;
    bool m_armed = false;
    std::vector<Take> m_takes;
    PlaybackLineClock m_lineClock;
    //! Single shot, for when the stillest take will have been still for IdleTimeout
    QTimer m_idleTimer;
};

} // namespace noteahead

#endif // MIDI_RECORDING_SERVICE_HPP
