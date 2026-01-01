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

#include "midi_cc_selection_model.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"

#include <algorithm>
#include <map>

namespace noteahead {

static const auto TAG = "MidiCcSelectionModel";

MidiCcSelectionModel::MidiCcSelectionModel(QObject * parent)
  : QAbstractListModel { parent }
{
}

int MidiCcSelectionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return static_cast<int>(m_settings.size());
}

QVariant MidiCcSelectionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    const auto & setting = m_settings[static_cast<size_t>(index.row())];

    switch (static_cast<Roles>(role)) {
    case Roles::ControllerRole:
        return setting.controller();
    case Roles::ValueRole:
        return setting.value();
    case Roles::EnabledRole:
        return setting.enabled();
    }

    return {};
}

bool MidiCcSelectionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;

    auto &setting = m_settings[static_cast<size_t>(index.row())];
    bool changed = false;

    switch (static_cast<Roles>(role)) {
    case Roles::ControllerRole:
        if (setting.controller() != value.toUInt()) {
            setting.setController(value.toUInt());
            changed = true;
        }
        break;
    case Roles::ValueRole:
        if (setting.value() != value.toUInt()) {
            setting.setValue(value.toUInt());
            changed = true;
        }
        break;
    case Roles::EnabledRole:
        if (setting.enabled() != value.toBool()) {
            setting.setEnabled(value.toBool());
            changed = true;
        }
        break;
    }

    if (changed) {
        emit dataChanged(index, index, { role });
        return true;
    }
    return false;
}

QHash<int, QByteArray> MidiCcSelectionModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[static_cast<int>(Roles::ControllerRole)] = "controller";
    roles[static_cast<int>(Roles::ValueRole)] = "value";
    roles[static_cast<int>(Roles::EnabledRole)] = "enabled";
    return roles;
}

void MidiCcSelectionModel::addMidiCcSetting(quint32 controller, quint32 value)
{
    juzzlin::L(TAG).info() << "Adding MIDI CC setting: controller=" << static_cast<int>(controller) << ", value=" << static_cast<int>(value);

    auto it = std::find_if(m_settings.begin(), m_settings.end(), [controller](const auto & setting) {
        return setting.controller() == controller;
    });

    if (it != m_settings.end()) {
        juzzlin::L(TAG).info() << "Updating existing MIDI CC setting for controller " << static_cast<int>(controller);
        it->setValue(value);
        it->setEnabled(true);
        const int index = static_cast<int>(std::distance(m_settings.begin(), it));
        emit dataChanged(this->index(index), this->index(index), { static_cast<int>(Roles::ValueRole), static_cast<int>(Roles::EnabledRole) });
    } else {
        beginInsertRows(QModelIndex {}, rowCount(), rowCount());
        m_settings.emplace_back(true, controller, value);
        endInsertRows();
    }
}

void MidiCcSelectionModel::removeMidiCcSetting(int index)
{
    juzzlin::L(TAG).info() << "Removing MIDI CC setting at index " << index;
    if (index >= 0 && index < rowCount()) {
        beginRemoveRows(QModelIndex(), index, index);
        m_settings.erase(m_settings.begin() + index);
        endRemoveRows();
    }
}

QString MidiCcSelectionModel::midiCcToString(quint32 controller) const
{
    return MidiCcMapping::controllerToString(static_cast<MidiCcMapping::Controller>(controller));
}

MidiCcSelectionModel::MidiCcSettingList MidiCcSelectionModel::midiCcSettings() const
{
    return m_settings;
}

void MidiCcSelectionModel::setMidiCcSettings(const MidiCcSettingList & midiCcSettings)
{
    beginResetModel();

    std::map<quint32, MidiCcSetting> uniqueSettings;
    for (const auto& setting : midiCcSettings) {
        uniqueSettings[setting.controller()] = setting;
    }

    m_settings.clear();
    for (const auto & pair : uniqueSettings) {
        m_settings.push_back(pair.second);
    }
    std::sort(m_settings.begin(), m_settings.end(), [](const auto& a, const auto& b){ return a.controller() < b.controller(); });

    endResetModel();
}

} // namespace noteahead
