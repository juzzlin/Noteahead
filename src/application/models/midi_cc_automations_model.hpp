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

#ifndef MIDI_CC_AUTOMATIONS_MODEL_HPP
#define MIDI_CC_AUTOMATIONS_MODEL_HPP

#include "../../domain/midi_cc_automation.hpp"

#include <QAbstractListModel>

#include <set>
#include <vector>

namespace noteahead {

class MidiCcAutomationsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(quint64 linesPerBeat READ linesPerBeat WRITE setLinesPerBeat NOTIFY linesPerBeatChanged)

public:
    enum class DataRole
    {
        Column,
        Comment,
        Controller,
        Enabled,
        Id,
        Line0,
        Line1,
        Pattern,
        Track,
        Value0,
        Value1,
        Modulation_Sine_Cycles,
        Modulation_Sine_Amplitude,
        Modulation_Sine_Inverted,
        EventsPerBeat,
        LineOffset
    };

    MidiCcAutomationsModel();

    Q_INVOKABLE void requestMidiCcAutomations();
    Q_INVOKABLE void requestMidiCcAutomationsByPattern(quint64 pattern);
    Q_INVOKABLE void requestMidiCcAutomationsByTrack(quint64 pattern, quint64 track);
    Q_INVOKABLE void requestMidiCcAutomationsByColumn(quint64 pattern, quint64 track, quint64 column);
    Q_INVOKABLE void requestMidiCcAutomationsByLine(quint64 pattern, quint64 track, quint64 column, quint64 line);
    using MidiCcAutomationList = std::vector<MidiCcAutomation>;
    void setMidiCcAutomations(MidiCcAutomationList midiCcAutomations);

    Q_INVOKABLE int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    Q_INVOKABLE bool removeAt(int row);
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
    Q_INVOKABLE QHash<int, QByteArray> roleNames() const override;

    quint64 linesPerBeat() const;
    void setLinesPerBeat(quint64 linesPerBeat);

    Q_INVOKABLE void applyAll();
    Q_INVOKABLE void changeController(int index, quint8 controller);

signals:
    void midiCcAutomationChanged(const MidiCcAutomation & midiCcAutomation);
    void midiCcAutomationDeleted(const MidiCcAutomation & midiCcAutomation);
    void midiCcAutomationsRequested();
    void linesPerBeatChanged();

private:
    MidiCcAutomationList filteredMidiCcAutomations(const MidiCcAutomationList & midiCcAutomations) const;

    MidiCcAutomationList m_midiCcAutomations;
    using MidiCcAutomationSet = std::set<MidiCcAutomation>;
    MidiCcAutomationSet m_midiCcAutomationsChanged;
    MidiCcAutomationSet m_midiCcAutomationsDeleted;

    struct Filter
    {
        std::optional<quint64> pattern;
        std::optional<quint64> track;
        std::optional<quint64> column;
        std::optional<quint64> line;
    };

    Filter m_filter;

    quint64 m_linesPerBeat = 8;
};

} // namespace noteahead

#endif // MIDI_CC_AUTOMATIONS_MODEL_HPP
