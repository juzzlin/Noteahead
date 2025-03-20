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

#include <vector>

#include "../domain/midi_cc_automation.hpp"

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class Position;

class AutomationService : public QObject
{
    Q_OBJECT

public:
    AutomationService();

    Q_INVOKABLE void addMidiCcAutomation(quint64 pattern, quint64 track, quint64 column, quint8 controller, quint64 line0, quint64 line1, quint8 value0, quint8 value1, QString comment);
    Q_INVOKABLE bool hasAutomations(quint64 pattern, quint64 track, quint64 column, quint64 line) const;

    void deserializeFromXml(QXmlStreamReader & reader);
    void serializeToXml(QXmlStreamWriter & writer) const;

signals:
    void lineDataChanged(const Position & position);

private:
    void notifyChangedLines(quint64 pattern, quint64 track, quint64 column, quint64 line0, quint64 line1);

    std::vector<MidiCcAutomation> m_midiCcAutomations;
};

} // namespace noteahead

#endif // AUTOMATION_SERVICE_HPP
