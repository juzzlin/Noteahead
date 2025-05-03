// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef AUTOMATION_SERVICE_HPP
#define AUTOMATION_SERVICE_HPP

#include <QObject>

#include <memory>
#include <vector>

#include "../../domain/event.hpp"
#include "../../domain/midi_cc_automation.hpp"
#include "../../domain/pitch_bend_automation.hpp"

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class Position;

class AutomationService : public QObject
{
    Q_OBJECT

public:
    AutomationService();

    void clear();

    // <-- API for QML/UI -->

    //! Adds automation for MIDI CC events.
    //! \param value0 Start value from 0 to 127.
    //! \param value1 End value from 0 to 127.
    Q_INVOKABLE quint64 addMidiCcAutomation(quint64 pattern, quint64 track, quint64 column, quint8 controller, quint64 line0, quint64 line1, quint8 value0, quint8 value1, QString comment, bool enabled);
    Q_INVOKABLE quint64 addMidiCcAutomation(quint64 pattern, quint64 track, quint64 column, quint8 controller, quint64 line0, quint64 line1, quint8 value0, quint8 value1, QString comment);

    //! Adds automation for MIDI pitch bend.
    //! \param value0 Start value from -100% to +100%.
    //! \param value1 End value from -100% to +100%.
    Q_INVOKABLE quint64 addPitchBendAutomation(quint64 pattern, quint64 track, quint64 column, quint64 line0, quint64 line1, int value0, int value1, QString comment, bool enabled);
    Q_INVOKABLE quint64 addPitchBendAutomation(quint64 pattern, quint64 track, quint64 column, quint64 line0, quint64 line1, int value0, int value1, QString comment);
    Q_INVOKABLE bool hasAutomations(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    Q_INVOKABLE double automationWeight(quint64 pattern, quint64 track, quint64 column, quint64 line) const;

    using MidiCcAutomationList = std::vector<MidiCcAutomation>;
    MidiCcAutomationList midiCcAutomationsByLine(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    MidiCcAutomationList midiCcAutomationsByColumn(quint64 pattern, quint64 track, quint64 column) const;
    MidiCcAutomationList midiCcAutomationsByTrack(quint64 pattern, quint64 track) const;
    MidiCcAutomationList midiCcAutomationsByPattern(quint64 pattern) const;
    MidiCcAutomationList midiCcAutomations() const;

    using PitchBendAutomationList = std::vector<PitchBendAutomation>;
    PitchBendAutomationList pitchBendAutomationsByLine(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    PitchBendAutomationList pitchBendAutomationsByColumn(quint64 pattern, quint64 track, quint64 column) const;
    PitchBendAutomationList pitchBendAutomationsByTrack(quint64 pattern, quint64 track) const;
    PitchBendAutomationList pitchBendAutomationsByPattern(quint64 pattern) const;
    PitchBendAutomationList pitchBendAutomations() const;

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    EventList renderToEventsByLine(size_t pattern, size_t track, size_t column, size_t line, size_t tick) const;
    EventList renderToEventsByColumn(size_t pattern, size_t track, size_t column, size_t tick, size_t ticksPerLine) const;

    void deserializeFromXml(QXmlStreamReader & reader);
    void serializeToXml(QXmlStreamWriter & writer) const;

public slots:
    void deleteMidiCcAutomation(const MidiCcAutomation & automationToDelete);
    void updateMidiCcAutomation(const MidiCcAutomation & updatedAutomation);
    void deletePitchBendAutomation(const PitchBendAutomation & automationToDelete);
    void updatePitchBendAutomation(const PitchBendAutomation & updatedAutomation);

signals:
    void lineDataChanged(const Position & position);

private:
    bool hasMidiCcAutomations(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    bool hasPitchBendAutomations(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    double midiCcAutomationWeight(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    double pitchBendAutomationWeight(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    void notifyChangedLines(quint64 pattern, quint64 track, quint64 column, quint64 line0, quint64 line1);
    void notifyChangedLines(const MidiCcAutomation & automation);
    void notifyChangedLinesMerged(const MidiCcAutomation & automation1, const MidiCcAutomation & automation2);
    void notifyChangedLines(const PitchBendAutomation & automation);
    void notifyChangedLinesMerged(const PitchBendAutomation & automation1, const PitchBendAutomation & automation2);
    EventList renderMidiCcToEventsByLine(size_t pattern, size_t track, size_t column, size_t line, size_t tick) const;
    EventList renderPitchBendToEventsByLine(size_t pattern, size_t track, size_t column, size_t line, size_t tick) const;
    EventList renderMidiCcToEventsByColumn(size_t pattern, size_t track, size_t column, size_t tick, size_t ticksPerLine) const;
    EventList renderPitchBendToEventsByColumn(size_t pattern, size_t track, size_t column, size_t tick, size_t ticksPerLine) const;

    struct Automations
    {
        MidiCcAutomationList midiCc;
        PitchBendAutomationList pitchBend;
    };

    Automations m_automations;
};

} // namespace noteahead

#endif // AUTOMATION_SERVICE_HPP
