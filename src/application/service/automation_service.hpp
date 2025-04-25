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

    //! API for QML/UI
    Q_INVOKABLE void addMidiCcAutomation(quint64 pattern, quint64 track, quint64 column, quint8 controller, quint64 line0, quint64 line1, quint8 value0, quint8 value1, QString comment, bool enabled);
    Q_INVOKABLE void addMidiCcAutomation(quint64 pattern, quint64 track, quint64 column, quint8 controller, quint64 line0, quint64 line1, quint8 value0, quint8 value1, QString comment);
    Q_INVOKABLE bool hasAutomations(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    Q_INVOKABLE double automationWeight(quint64 pattern, quint64 track, quint64 column, quint64 line) const;

    using MidiCcAutomationList = std::vector<MidiCcAutomation>;
    MidiCcAutomationList midiCcAutomationsByLine(quint64 pattern, quint64 track, quint64 column, quint64 line) const;
    MidiCcAutomationList midiCcAutomationsByColumn(quint64 pattern, quint64 track, quint64 column) const;
    MidiCcAutomationList midiCcAutomationsByTrack(quint64 pattern, quint64 track) const;
    MidiCcAutomationList midiCcAutomationsByPattern(quint64 pattern) const;
    MidiCcAutomationList midiCcAutomations() const;

    using EventS = std::shared_ptr<Event>;
    using EventList = std::vector<EventS>;
    EventList renderToEventsByLine(size_t pattern, size_t track, size_t column, size_t line, size_t tick) const;
    EventList renderToEventsByColumn(size_t pattern, size_t track, size_t column, size_t tick, size_t ticksPerLine) const;

    void deserializeFromXml(QXmlStreamReader & reader);
    void serializeToXml(QXmlStreamWriter & writer) const;

public slots:
    void deleteMidiCcAutomation(const MidiCcAutomation & midiCcAutomationToDelete);
    void updateMidiCcAutomation(const MidiCcAutomation & updatedMidiCcAutomation);

signals:
    void lineDataChanged(const Position & position);

private:
    void notifyChangedLines(quint64 pattern, quint64 track, quint64 column, quint64 line0, quint64 line1);
    void notifyChangedLines(const MidiCcAutomation & midiCcAutomation);
    void notifyChangedLinesMerged(const MidiCcAutomation & midiCcAutomation1, const MidiCcAutomation & midiCcAutomation2);

    MidiCcAutomationList m_midiCcAutomations;
};

} // namespace noteahead

#endif // AUTOMATION_SERVICE_HPP
