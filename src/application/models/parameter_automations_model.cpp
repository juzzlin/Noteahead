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

#include "parameter_automations_model.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"

#include <algorithm>

namespace noteahead {

static const auto TAG = "ParameterAutomationsModel";

ParameterAutomationsModel::ParameterAutomationsModel() = default;

void ParameterAutomationsModel::requestParameterAutomations()
{
    m_filter = {};
    emit parameterAutomationsRequested();
}

void ParameterAutomationsModel::requestParameterAutomationsByPattern(quint64 pattern)
{
    m_filter = { pattern, {}, {}, {} };
    emit parameterAutomationsRequested();
}

void ParameterAutomationsModel::requestParameterAutomationsByTrack(quint64 pattern, quint64 track)
{
    m_filter = { pattern, track, {}, {} };
    emit parameterAutomationsRequested();
}

void ParameterAutomationsModel::requestParameterAutomationsByColumn(quint64 pattern, quint64 track, quint64 column)
{
    m_filter = { pattern, track, Range { column, column }, {} };
    emit parameterAutomationsRequested();
}

void ParameterAutomationsModel::requestParameterAutomationsByLine(quint64 pattern, quint64 track, quint64 column, quint64 line)
{
    m_filter = { pattern, track, Range { column, column }, Range { line, line } };
    emit parameterAutomationsRequested();
}

void ParameterAutomationsModel::requestParameterAutomationsBySelection(quint64 pattern, quint64 track, quint64 minColumn, quint64 maxColumn, quint64 minLine, quint64 maxLine)
{
    m_filter = { pattern, track, Range { minColumn, maxColumn }, Range { minLine, maxLine } };
    emit parameterAutomationsRequested();
}

ParameterAutomationsModel::ParameterAutomationList ParameterAutomationsModel::filtered(const ParameterAutomationList & parameterAutomations) const
{
    ParameterAutomationList result;
    std::ranges::copy_if(parameterAutomations, std::back_inserter(result), [this](const auto & automation) {
        const auto & location = automation.location();
        const auto & interpolation = automation.interpolation();
        return (!m_filter.pattern || location.pattern() == *m_filter.pattern) && //
          (!m_filter.track || location.track() == *m_filter.track) && //
          (!m_filter.columns || (location.column() >= m_filter.columns->first && location.column() <= m_filter.columns->second)) && //
          (!m_filter.lines || (interpolation.line0 <= m_filter.lines->second && interpolation.line1 >= m_filter.lines->first));
    });
    return result;
}

void ParameterAutomationsModel::setParameterAutomations(ParameterAutomationList parameterAutomations)
{
    juzzlin::L(TAG).info() << "Setting parameter automations: " << parameterAutomations.size() << " found";
    beginResetModel();
    m_changed.clear();
    m_deleted.clear();
    m_parameterAutomations = filtered(parameterAutomations);
    endResetModel();
}

int ParameterAutomationsModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(m_parameterAutomations.size());
}

QVariant ParameterAutomationsModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid() || static_cast<size_t>(index.row()) >= m_parameterAutomations.size()) {
        return {};
    }

    const auto & automation = m_parameterAutomations.at(static_cast<size_t>(index.row()));
    const auto & interpolation = automation.interpolation();
    const auto & modulation = automation.modulation();
    switch (static_cast<DataRole>(role)) {
    case DataRole::Column:
        return static_cast<quint64>(automation.location().column());
    case DataRole::Comment:
        return automation.comment();
    case DataRole::Curve:
        return static_cast<int>(interpolation.curve);
    case DataRole::Owner:
        return ParameterAutomation::Target::ownerToString(automation.target().owner);
    case DataRole::OwnerId:
        return QString::fromStdString(automation.target().ownerId);
    case DataRole::Enabled:
        return automation.enabled();
    case DataRole::EventsPerBeat:
        return automation.eventsPerBeat();
    case DataRole::Id:
        return static_cast<quint64>(automation.id());
    case DataRole::Line0:
        return static_cast<quint64>(interpolation.line0);
    case DataRole::Line1:
        return static_cast<quint64>(interpolation.line1);
    case DataRole::LineOffset:
        return automation.lineOffset();
    case DataRole::Mode:
        return static_cast<int>(automation.mode());
    case DataRole::Modulation_Amplitude:
        return modulation.amplitude;
    case DataRole::Modulation_Cycles:
        return modulation.cycles;
    case DataRole::Modulation_Inverted:
        return modulation.inverted;
    case DataRole::Modulation_Offset:
        return modulation.offset;
    case DataRole::Modulation_Type:
        return static_cast<int>(modulation.type);
    case DataRole::Parameter:
        return QString::fromStdString(automation.target().parameter);
    case DataRole::Pattern:
        return static_cast<quint64>(automation.location().pattern());
    case DataRole::Track:
        return static_cast<quint64>(automation.location().track());
    case DataRole::Value0:
        return interpolation.value0;
    case DataRole::Value1:
        return interpolation.value1;
    }
    return {};
}

bool ParameterAutomationsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (!index.isValid() || static_cast<size_t>(index.row()) >= m_parameterAutomations.size()) {
        return false;
    }

    auto automation = m_parameterAutomations.at(static_cast<size_t>(index.row()));
    auto interpolation = automation.interpolation();
    auto modulation = automation.modulation();

    switch (static_cast<DataRole>(role)) {
    case DataRole::Comment:
        automation.setComment(value.toString());
        break;
    case DataRole::Curve:
        interpolation.curve = static_cast<Interpolator::CurveType>(value.toInt());
        break;
    case DataRole::Enabled:
        automation.setEnabled(value.toBool());
        break;
    case DataRole::EventsPerBeat:
        automation.setEventsPerBeat(static_cast<uint8_t>(value.toUInt()));
        break;
    case DataRole::Line0:
        interpolation.line0 = value.toULongLong();
        break;
    case DataRole::Line1:
        interpolation.line1 = value.toULongLong();
        break;
    case DataRole::LineOffset:
        automation.setLineOffset(static_cast<uint8_t>(value.toUInt()));
        break;
    case DataRole::Mode:
        automation.setMode(static_cast<ParameterAutomation::Mode>(value.toInt()));
        break;
    case DataRole::Modulation_Amplitude:
        modulation.amplitude = value.toFloat();
        break;
    case DataRole::Modulation_Cycles:
        modulation.cycles = value.toFloat();
        break;
    case DataRole::Modulation_Inverted:
        modulation.inverted = value.toBool();
        break;
    case DataRole::Modulation_Offset:
        modulation.offset = value.toFloat();
        break;
    case DataRole::Modulation_Type:
        modulation.type = static_cast<ModulationParameters::ModulationType>(value.toInt());
        break;
    case DataRole::Value0:
        interpolation.value0 = std::clamp(value.toDouble(), 0.0, 1.0);
        break;
    case DataRole::Value1:
        interpolation.value1 = std::clamp(value.toDouble(), 0.0, 1.0);
        break;
    // Where an automation sits and what it drives are not edited here: a different target is a
    // different automation.
    case DataRole::Column:
    case DataRole::Owner:
    case DataRole::OwnerId:
    case DataRole::Id:
    case DataRole::Parameter:
    case DataRole::Pattern:
    case DataRole::Track:
        return false;
    }

    automation.setInterpolation(interpolation);
    automation.setModulation(modulation);
    if (automation == m_parameterAutomations.at(static_cast<size_t>(index.row()))) {
        return false;
    }

    m_parameterAutomations[static_cast<size_t>(index.row())] = automation;
    // Keyed by id, so the newest edit replaces the one held before it
    m_changed.erase(automation);
    m_changed.insert(automation);
    emit dataChanged(index, index, { role });
    return true;
}

bool ParameterAutomationsModel::removeAt(int row)
{
    return removeRows(row, 1);
}

bool ParameterAutomationsModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if (row < 0 || count <= 0 || row + count > static_cast<int>(m_parameterAutomations.size())) {
        return false;
    }
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = row; i < row + count; i++) {
        const auto & automation = m_parameterAutomations.at(static_cast<size_t>(i));
        m_changed.erase(automation);
        m_deleted.insert(automation);
    }
    m_parameterAutomations.erase(m_parameterAutomations.begin() + row, m_parameterAutomations.begin() + row + count);
    endRemoveRows();
    return true;
}

QHash<int, QByteArray> ParameterAutomationsModel::roleNames() const
{
    return {
        { static_cast<int>(DataRole::Column), "column" },
        { static_cast<int>(DataRole::Comment), "comment" },
        { static_cast<int>(DataRole::Curve), "curve" },
        { static_cast<int>(DataRole::Owner), "owner" },
        { static_cast<int>(DataRole::OwnerId), "ownerId" },
        { static_cast<int>(DataRole::Enabled), "enabled" },
        { static_cast<int>(DataRole::EventsPerBeat), "eventsPerBeat" },
        { static_cast<int>(DataRole::Id), "id" },
        { static_cast<int>(DataRole::Line0), "line0" },
        { static_cast<int>(DataRole::Line1), "line1" },
        { static_cast<int>(DataRole::LineOffset), "lineOffset" },
        { static_cast<int>(DataRole::Mode), "mode" },
        { static_cast<int>(DataRole::Modulation_Amplitude), "modulationAmplitude" },
        { static_cast<int>(DataRole::Modulation_Cycles), "modulationCycles" },
        { static_cast<int>(DataRole::Modulation_Inverted), "modulationInverted" },
        { static_cast<int>(DataRole::Modulation_Offset), "modulationOffset" },
        { static_cast<int>(DataRole::Modulation_Type), "modulationType" },
        { static_cast<int>(DataRole::Parameter), "parameter" },
        { static_cast<int>(DataRole::Pattern), "pattern" },
        { static_cast<int>(DataRole::Track), "track" },
        { static_cast<int>(DataRole::Value0), "value0" },
        { static_cast<int>(DataRole::Value1), "value1" }
    };
}

void ParameterAutomationsModel::applyValues(int index, const QVariantMap & values)
{
    if (index < 0 || static_cast<size_t>(index) >= m_parameterAutomations.size()) {
        return;
    }

    // Through setData(), so that the rules for each role stay in one place
    static const std::vector<std::pair<QString, DataRole>> fields {
        { "mode", DataRole::Mode },
        { "line0", DataRole::Line0 },
        { "line1", DataRole::Line1 },
        { "value0", DataRole::Value0 },
        { "value1", DataRole::Value1 },
        { "curve", DataRole::Curve },
        { "eventsPerBeat", DataRole::EventsPerBeat },
        { "lineOffset", DataRole::LineOffset },
        { "modulationType", DataRole::Modulation_Type },
        { "modulationCycles", DataRole::Modulation_Cycles },
        { "modulationAmplitude", DataRole::Modulation_Amplitude },
        { "modulationOffset", DataRole::Modulation_Offset },
        { "modulationInverted", DataRole::Modulation_Inverted },
        { "comment", DataRole::Comment }
    };

    const auto modelIndex = this->index(index);
    for (auto && [key, role] : fields) {
        if (const auto value = values.find(key); value != values.end()) {
            setData(modelIndex, *value, static_cast<int>(role));
        }
    }

    emit automationReplaced(index);
}

void ParameterAutomationsModel::applyAll()
{
    for (auto && automation : m_changed) {
        emit parameterAutomationChanged(automation);
    }
    m_changed.clear();
    for (auto && automation : m_deleted) {
        emit parameterAutomationDeleted(automation);
    }
    m_deleted.clear();
}

} // namespace noteahead
