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

#ifndef XY_PAD_RECORDING_SERVICE_HPP
#define XY_PAD_RECORDING_SERVICE_HPP

#include <QObject>
#include <QTimer>

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../../../application/service/playback_line_clock.hpp"
#include "../../../domain/tracker/automation_set.hpp"
#include "../../../domain/tracker/automation_take.hpp"

namespace noteahead {

class AutomationService;
class DeviceService;
class EditorService;
class XyPadDevice;

//! Records the XY pads while the song plays in edit mode, as notes are recorded.
//!
//! A take lasts from a hand going on a pad until the pad is home again, spring back included, since
//! that is heard too. Each axis the hand moved becomes a drawn automation of the pad's own position
//! on the column under the cursor, one per pattern played through, so a recorded move is edited as
//! any drawn curve is and follows the pad wherever its axes are pointed later. A take replaces what
//! automated the same axis over the lines it covers, and is undone in one step.
class XyPadRecordingService : public QObject
{
    Q_OBJECT

    //! Whether a hand on a pad records now.
    Q_PROPERTY(bool armed READ armed NOTIFY armedChanged)

public:
    using Clock = std::function<std::chrono::steady_clock::time_point()>;

    XyPadRecordingService(std::shared_ptr<DeviceService> deviceService, std::shared_ptr<EditorService> editorService, std::shared_ptr<AutomationService> automationService, QObject * parent = nullptr);
    ~XyPadRecordingService() override;

    //! What time it is. The steady clock unless a test says otherwise.
    void setClock(Clock clock);

    bool armed() const;
    //! Armed while the song plays in edit mode. Disarming ends every take.
    void setArmed(bool armed);

    //! The song has reached @p tick, which is due @p lookahead from now. What the takes are timed by.
    void setTick(quint64 tick, std::chrono::steady_clock::duration lookahead);
    //! A hand went on pad @p pad of the XY pad device with uuid @p deviceUuid: starts a take of it,
    //! when armed and not already recording it.
    void startTake(const QString & deviceUuid, int pad);
    //! Notes where every pad being recorded stands now, and ends the takes whose hand is done.
    void sample();
    //! Ends every take, keeping what was recorded.
    void finishTakes();

    bool isRecording() const;

    //! How far a recorded curve may stray from the take, in knob travel.
    static constexpr double Tolerance { 0.005 };
    //! How often a take is sampled while one is going.
    static constexpr std::chrono::milliseconds SampleInterval { 16 };

signals:
    void armedChanged();

private:
    struct Take
    {
        std::weak_ptr<XyPadDevice> device;
        std::string deviceUuid;
        size_t pad = 0;
        size_t track = 0;
        size_t column = 0;
        AutomationTake::SampleList x;
        AutomationTake::SampleList y;
    };

    void sample(Take & take, std::chrono::steady_clock::time_point time);
    void finish(const Take & take);
    //! Takes what automates @p target in @p pattern off lines @p line0 to @p line1: out of the
    //! song into @p deletions, and out of the take's own earlier pieces in @p additions.
    void clearLines(const ParameterAutomation::Target & target, size_t pattern, size_t line0, size_t line1, AutomationSet & additions, AutomationSet & deletions) const;

    std::shared_ptr<DeviceService> m_deviceService;
    std::shared_ptr<EditorService> m_editorService;
    std::shared_ptr<AutomationService> m_automationService;
    Clock m_clock;
    bool m_armed = false;
    std::vector<Take> m_takes;
    PlaybackLineClock m_lineClock;
    QTimer m_sampleTimer;
};

} // namespace noteahead

#endif // XY_PAD_RECORDING_SERVICE_HPP
