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

#include <vector>

namespace noteahead {

class MidiCcAutomationsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum class DataRole
    {
        Column,
        Comment,
        Controller,
        Id,
        Line0,
        Line1,
        Pattern,
        Track,
        Value0,
        Value1
    };

    MidiCcAutomationsModel();

    Q_INVOKABLE void requestMidiCcAutomations();
    using MidiCcAutomationList = std::vector<MidiCcAutomation>;
    void setMidiCcAutomations(MidiCcAutomationList midiCcAutomations);

    Q_INVOKABLE int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole) override;
    Q_INVOKABLE QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void applyAll();

signals:
    void midiCcAutomationChanged(const MidiCcAutomation & midiCcAutomation);
    void midiCcAutomationsRequested();

private:
    MidiCcAutomationList m_midiCcAutomations;
};

} // namespace noteahead

#endif // MIDI_CC_AUTOMATIONS_MODEL_HPP
