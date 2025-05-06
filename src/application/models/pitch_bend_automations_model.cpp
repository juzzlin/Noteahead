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

#include "pitch_bend_automations_model.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"

namespace noteahead {

static const auto TAG = "PitchBendAutomationsModel";

PitchBendAutomationsModel::PitchBendAutomationsModel()
{
}

void PitchBendAutomationsModel::requestPitchBendAutomations()
{
    m_filter = {};

    emit pitchBendAutomationsRequested();
}

void PitchBendAutomationsModel::requestPitchBendAutomationsByPattern(quint64 pattern)
{
    m_filter = {};
    m_filter.pattern = pattern;

    emit pitchBendAutomationsRequested();
}

void PitchBendAutomationsModel::requestPitchBendAutomationsByTrack(quint64 pattern, quint64 track)
{
    m_filter = {};
    m_filter.pattern = pattern;
    m_filter.track = track;

    emit pitchBendAutomationsRequested();
}

void PitchBendAutomationsModel::requestPitchBendAutomationsByColumn(quint64 pattern, quint64 track, quint64 column)
{
    m_filter = {};
    m_filter.pattern = pattern;
    m_filter.track = track;
    m_filter.column = column;

    emit pitchBendAutomationsRequested();
}

PitchBendAutomationsModel::PitchBendAutomationList PitchBendAutomationsModel::filteredPitchBendAutomations(const PitchBendAutomationList & PitchBendAutomations) const
{
    PitchBendAutomationList filtered;
    std::ranges::copy_if(PitchBendAutomations, std::back_inserter(filtered),
                         [this](const auto & automation) {
                             const auto & loc = automation.location();
                             if (m_filter.pattern.has_value() && loc.pattern != m_filter.pattern.value()) {
                                 return false;
                             }
                             if (m_filter.track.has_value() && loc.track != m_filter.track.value()) {
                                 return false;
                             }
                             if (m_filter.column.has_value() && loc.column != m_filter.column.value()) {
                                 return false;
                             }
                             return true;
                         });
    return filtered;
}

void PitchBendAutomationsModel::setPitchBendAutomations(PitchBendAutomationList PitchBendAutomations)
{
    juzzlin::L(TAG).info() << "Setting MIDI CC automations: " << PitchBendAutomations.size() << " found";
    beginResetModel();
    m_pitchBendAutomationsChanged.clear();
    m_pitchBendAutomationsDeleted.clear();
    m_pitchBendAutomations = filteredPitchBendAutomations(PitchBendAutomations);
    endResetModel();
}

int PitchBendAutomationsModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent)

    return static_cast<int>(m_pitchBendAutomations.size());
}

QVariant PitchBendAutomationsModel::data(const QModelIndex & index, int role) const
{
    if (static_cast<size_t>(index.row()) < m_pitchBendAutomations.size()) {
        const auto PitchBendAutomation = m_pitchBendAutomations.at(static_cast<size_t>(index.row()));
        switch (static_cast<DataRole>(role)) {
        case DataRole::Comment:
            return PitchBendAutomation.comment();
        case DataRole::Enabled:
            return PitchBendAutomation.enabled();
        case DataRole::Id:
            return static_cast<quint64>(PitchBendAutomation.id());
        case DataRole::Line0:
            return static_cast<quint64>(PitchBendAutomation.interpolation().line0);
        case DataRole::Line1:
            return static_cast<quint64>(PitchBendAutomation.interpolation().line1);
        case DataRole::Value0:
            return PitchBendAutomation.interpolation().value0;
        case DataRole::Value1:
            return PitchBendAutomation.interpolation().value1;
        case DataRole::Pattern:
            return static_cast<quint64>(PitchBendAutomation.location().pattern);
        case DataRole::Track:
            return static_cast<quint64>(PitchBendAutomation.location().track);
        case DataRole::Column:
            return static_cast<quint64>(PitchBendAutomation.location().column);
        }
    }
    return "N/A";
}

bool PitchBendAutomationsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (!index.isValid() || static_cast<size_t>(index.row()) >= m_pitchBendAutomations.size())
        return false;

    auto & PitchBendAutomation = m_pitchBendAutomations[static_cast<size_t>(index.row())];
    bool changed = false;

    switch (static_cast<DataRole>(role)) {
    case DataRole::Comment:
        if (PitchBendAutomation.comment() != value.toString()) {
            PitchBendAutomation.setComment(value.toString());
            changed = true;
        }
        break;
    case DataRole::Enabled:
        if (PitchBendAutomation.enabled() != value.toBool()) {
            PitchBendAutomation.setEnabled(value.toBool());
            changed = true;
        }
        break;
    case DataRole::Line0: {
        auto interpolation = PitchBendAutomation.interpolation();
        if (const auto newLine0 = static_cast<uint16_t>(value.toUInt()); interpolation.line0 != newLine0) {
            interpolation.line0 = newLine0;
            PitchBendAutomation.setInterpolation(interpolation);
            changed = true;
        }
    } break;
    case DataRole::Line1: {
        auto interpolation = PitchBendAutomation.interpolation();
        if (const auto newLine1 = static_cast<uint16_t>(value.toUInt()); interpolation.line1 != newLine1) {
            interpolation.line1 = newLine1;
            PitchBendAutomation.setInterpolation(interpolation);
            changed = true;
        }
    } break;
    case DataRole::Value0: {
        auto interpolation = PitchBendAutomation.interpolation();
        if (const auto newValue0 = static_cast<uint8_t>(value.toUInt()); interpolation.value0 != newValue0) {
            interpolation.value0 = newValue0;
            PitchBendAutomation.setInterpolation(interpolation);
            changed = true;
        }
    } break;
    case DataRole::Value1: {
        auto interpolation = PitchBendAutomation.interpolation();
        if (const auto newValue1 = static_cast<uint8_t>(value.toUInt()); interpolation.value1 != newValue1) {
            interpolation.value1 = newValue1;
            PitchBendAutomation.setInterpolation(interpolation);
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
        m_pitchBendAutomationsChanged.erase(PitchBendAutomation);
        m_pitchBendAutomationsChanged.insert(PitchBendAutomation);
        emit dataChanged(index, index, { role });
        return true;
    }

    return false;
}

bool PitchBendAutomationsModel::removeAt(int row)
{
    return removeRows(row, 1);
}

bool PitchBendAutomationsModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (row < 0 || row + count > static_cast<int>(m_pitchBendAutomations.size())) {
        return false;
    }
    beginRemoveRows(parent, row, row + count - 1);
    m_pitchBendAutomationsDeleted.insert(m_pitchBendAutomations.at(static_cast<size_t>(row)));
    m_pitchBendAutomations.erase(m_pitchBendAutomations.begin() + row,
                                 m_pitchBendAutomations.begin() + row + count);
    endRemoveRows();
    return true;
}

QHash<int, QByteArray> PitchBendAutomationsModel::roleNames() const
{
    return {
        { static_cast<int>(DataRole::Column), "column" },
        { static_cast<int>(DataRole::Comment), "comment" },
        { static_cast<int>(DataRole::Enabled), "enabled" },
        { static_cast<int>(DataRole::Line0), "line0" },
        { static_cast<int>(DataRole::Line1), "line1" },
        { static_cast<int>(DataRole::Pattern), "pattern" },
        { static_cast<int>(DataRole::Track), "track" },
        { static_cast<int>(DataRole::Value0), "value0" },
        { static_cast<int>(DataRole::Value1), "value1" }
    };
}

void PitchBendAutomationsModel::applyAll()
{
    for (auto && PitchBendAutomation : m_pitchBendAutomationsChanged) {
        emit pitchBendAutomationChanged(PitchBendAutomation);
    }
    m_pitchBendAutomationsChanged.clear();
    for (auto && PitchBendAutomation : m_pitchBendAutomationsDeleted) {
        emit pitchBendAutomationDeleted(PitchBendAutomation);
    }
    m_pitchBendAutomationsDeleted.clear();
}

} // namespace noteahead
