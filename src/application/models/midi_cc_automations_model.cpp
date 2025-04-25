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

#include "midi_cc_automations_model.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"

namespace noteahead {

static const auto TAG = "MidiCcAutomationsModel";

MidiCcAutomationsModel::MidiCcAutomationsModel()
{
}

void MidiCcAutomationsModel::requestMidiCcAutomations()
{
    emit midiCcAutomationsRequested();
}

void MidiCcAutomationsModel::setMidiCcAutomations(MidiCcAutomationList midiCcAutomations)
{
    juzzlin::L(TAG).info() << "Setting MIDI CC automations: " << midiCcAutomations.size() << " found";
    beginResetModel();
    m_midiCcAutomations = midiCcAutomations;
    endResetModel();
}

int MidiCcAutomationsModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent)

    return static_cast<int>(m_midiCcAutomations.size());
}

QVariant MidiCcAutomationsModel::data(const QModelIndex & index, int role) const
{
    if (static_cast<size_t>(index.row()) < m_midiCcAutomations.size()) {
        const auto midiCcAutomation = m_midiCcAutomations.at(static_cast<size_t>(index.row()));
        switch (static_cast<DataRole>(role)) {
        case DataRole::Comment:
            return midiCcAutomation.comment();
        case DataRole::Controller:
            return midiCcAutomation.controller();
        case DataRole::Enabled:
            return midiCcAutomation.enabled();
        case DataRole::Id:
            return static_cast<quint64>(midiCcAutomation.id());
        case DataRole::Line0:
            return static_cast<quint64>(midiCcAutomation.interpolation().line0);
        case DataRole::Line1:
            return static_cast<quint64>(midiCcAutomation.interpolation().line1);
        case DataRole::Value0:
            return midiCcAutomation.interpolation().value0;
        case DataRole::Value1:
            return midiCcAutomation.interpolation().value1;
        case DataRole::Pattern:
            return static_cast<quint64>(midiCcAutomation.location().pattern);
        case DataRole::Track:
            return static_cast<quint64>(midiCcAutomation.location().track);
        case DataRole::Column:
            return static_cast<quint64>(midiCcAutomation.location().column);
        }
    }
    return "N/A";
}

bool MidiCcAutomationsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (!index.isValid() || static_cast<size_t>(index.row()) >= m_midiCcAutomations.size())
        return false;

    auto & midiCcAutomation = m_midiCcAutomations[static_cast<size_t>(index.row())];
    bool changed = false;

    switch (static_cast<DataRole>(role)) {
    case DataRole::Comment:
        if (midiCcAutomation.comment() != value.toString()) {
            midiCcAutomation.setComment(value.toString());
            changed = true;
        }
        break;
    case DataRole::Controller:
        if (const auto newController = static_cast<uint8_t>(value.toInt()); midiCcAutomation.controller() != newController) {
            midiCcAutomation.setController(newController);
            changed = true;
        }
        break;
    case DataRole::Enabled:
        if (midiCcAutomation.enabled() != value.toBool()) {
            midiCcAutomation.setEnabled(value.toBool());
            changed = true;
        }
        break;
    case DataRole::Line0: {
        auto interpolation = midiCcAutomation.interpolation();
        if (const auto newLine0 = static_cast<uint16_t>(value.toUInt()); interpolation.line0 != newLine0) {
            interpolation.line0 = newLine0;
            midiCcAutomation.setInterpolation(interpolation);
            changed = true;
        }
    } break;
    case DataRole::Line1: {
        auto interpolation = midiCcAutomation.interpolation();
        if (const auto newLine1 = static_cast<uint16_t>(value.toUInt()); interpolation.line1 != newLine1) {
            interpolation.line1 = newLine1;
            midiCcAutomation.setInterpolation(interpolation);
            changed = true;
        }
    } break;
    case DataRole::Value0: {
        auto interpolation = midiCcAutomation.interpolation();
        if (const auto newValue0 = static_cast<uint8_t>(value.toUInt()); interpolation.value0 != newValue0) {
            interpolation.value0 = newValue0;
            midiCcAutomation.setInterpolation(interpolation);
            changed = true;
        }
    } break;
    case DataRole::Value1: {
        auto interpolation = midiCcAutomation.interpolation();
        if (const auto newValue1 = static_cast<uint8_t>(value.toUInt()); interpolation.value1 != newValue1) {
            interpolation.value1 = newValue1;
            midiCcAutomation.setInterpolation(interpolation);
            changed = true;
        }
    } break;
    case DataRole::Pattern:
    case DataRole::Track:
    case DataRole::Column:
    case DataRole::Id:
        break;
    }

    if (changed) {
        emit dataChanged(index, index, { role });
        return true;
    }

    return false;
}

QHash<int, QByteArray> MidiCcAutomationsModel::roleNames() const
{
    return {
        { static_cast<int>(DataRole::Column), "column" },
        { static_cast<int>(DataRole::Comment), "comment" },
        { static_cast<int>(DataRole::Controller), "controller" },
        { static_cast<int>(DataRole::Enabled), "enabled" },
        { static_cast<int>(DataRole::Line0), "line0" },
        { static_cast<int>(DataRole::Line1), "line1" },
        { static_cast<int>(DataRole::Pattern), "pattern" },
        { static_cast<int>(DataRole::Track), "track" },
        { static_cast<int>(DataRole::Value0), "value0" },
        { static_cast<int>(DataRole::Value1), "value1" }
    };
}

void MidiCcAutomationsModel::applyAll()
{
    for (auto && midiCcAutomation : m_midiCcAutomations) {
        emit midiCcAutomationChanged(midiCcAutomation);
    }
}

} // namespace noteahead
