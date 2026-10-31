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

#include "../../domain/tracker/drawn_curve_sync.hpp"
#include "automation_curve_editor.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"

namespace noteahead {

namespace {
//! Whether an edit of @p role reshapes the curve: its preview, and a drawn curve's ends with it
bool reshapes(PitchBendAutomationsModel::DataRole role)
{
    using enum PitchBendAutomationsModel::DataRole;
    return role == Line0 || role == Line1 || role == Value0 || role == Value1 || role == Curve || role == DrawnCurve;
}
} // namespace

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
    m_filter.columns = Range { column, column };

    emit pitchBendAutomationsRequested();
}

void PitchBendAutomationsModel::requestPitchBendAutomationsByLine(quint64 pattern, quint64 track, quint64 column, quint64 line)
{
    m_filter = {};
    m_filter.pattern = pattern;
    m_filter.track = track;
    m_filter.columns = Range { column, column };
    m_filter.lines = Range { line, line };

    emit pitchBendAutomationsRequested();
}

void PitchBendAutomationsModel::requestPitchBendAutomationById(quint64 automationId)
{
    m_filter = {};
    m_filter.id = automationId;

    emit pitchBendAutomationsRequested();
}

void PitchBendAutomationsModel::requestPitchBendAutomationsBySelection(quint64 pattern, quint64 track, quint64 minColumn, quint64 maxColumn, quint64 minLine, quint64 maxLine)
{
    m_filter = {};
    m_filter.pattern = pattern;
    m_filter.track = track;
    m_filter.columns = Range { minColumn, maxColumn };
    m_filter.lines = Range { minLine, maxLine };

    emit pitchBendAutomationsRequested();
}

PitchBendAutomationsModel::PitchBendAutomationList PitchBendAutomationsModel::filteredPitchBendAutomations(const PitchBendAutomationList & PitchBendAutomations) const
{
    PitchBendAutomationList filtered;
    std::ranges::copy_if(PitchBendAutomations, std::back_inserter(filtered),
                         [this](const auto & automation) {
                             if (m_filter.id.has_value() && automation.id() != m_filter.id.value()) {
                                 return false;
                             }
                             if (m_filter.pattern.has_value() && automation.location().pattern() != m_filter.pattern.value()) {
                                 return false;
                             }
                             if (m_filter.track.has_value() && automation.location().track() != m_filter.track.value()) {
                                 return false;
                             }
                             if (m_filter.columns.has_value() && (automation.location().column() < m_filter.columns->first || automation.location().column() > m_filter.columns->second)) {
                                 return false;
                             }
                             if (m_filter.lines.has_value()) {
                                 if (automation.interpolation().line0 > m_filter.lines->second || automation.interpolation().line1 < m_filter.lines->first) {
                                     return false;
                                 }
                             }
                             return true;
                         });
    return filtered;
}

void PitchBendAutomationsModel::setPitchBendAutomations(PitchBendAutomationList PitchBendAutomations)
{
    juzzlin::L(TAG).info() << "Setting pitch bend automations: " << PitchBendAutomations.size() << " found";
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
        case DataRole::Curve:
            return static_cast<int>(PitchBendAutomation.interpolation().curve);
        case DataRole::CurveShape:
            return AutomationCurveEditor::toVariantList(curveShape(PitchBendAutomation.interpolation()));
        case DataRole::DrawnCurve:
            return PitchBendAutomation.interpolation().curve == Interpolator::CurveType::Drawn ? AutomationCurveEditor::toVariantList(PitchBendAutomation.interpolation().drawnCurve) : QVariantList {};
        case DataRole::Pattern:
            return static_cast<quint64>(PitchBendAutomation.location().pattern());
        case DataRole::Track:
            return static_cast<quint64>(PitchBendAutomation.location().track());
        case DataRole::Column:
            return static_cast<quint64>(PitchBendAutomation.location().column());
        case DataRole::Modulation_Cycles:
            return PitchBendAutomation.modulation().cycles;
        case DataRole::Modulation_Amplitude:
            return PitchBendAutomation.modulation().amplitude;
        case DataRole::Modulation_Offset:
            return PitchBendAutomation.modulation().offset;
        case DataRole::Modulation_Inverted:
            return PitchBendAutomation.modulation().inverted;
        case DataRole::Modulation_Type:
            return static_cast<int>(PitchBendAutomation.modulation().type);
        case DataRole::EventsPerBeat:
            return PitchBendAutomation.eventsPerBeat() ? static_cast<int>(*PitchBendAutomation.eventsPerBeat()) : -1;
        case DataRole::LineOffset:
            return static_cast<int>(PitchBendAutomation.lineOffset());
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
    case DataRole::Line0:
    case DataRole::Line1:
    case DataRole::Value0:
    case DataRole::Value1:
    case DataRole::Curve:
    case DataRole::DrawnCurve: {
        const auto previous = PitchBendAutomation.interpolation();
        auto interpolation = previous;
        switch (static_cast<DataRole>(role)) {
        case DataRole::Line0:
            interpolation.line0 = static_cast<uint16_t>(value.toUInt());
            break;
        case DataRole::Line1:
            interpolation.line1 = static_cast<uint16_t>(value.toUInt());
            break;
        case DataRole::Value0:
            interpolation.value0 = value.toInt();
            break;
        case DataRole::Value1:
            interpolation.value1 = value.toInt();
            break;
        case DataRole::Curve:
            interpolation.curve = static_cast<Interpolator::CurveType>(value.toInt());
            break;
        default:
            if (const auto drawnCurve = AutomationCurveEditor::fromVariantList(value.toList()); !drawnCurve.isEmpty()) {
                interpolation.curve = Interpolator::CurveType::Drawn;
                interpolation.drawnCurve = drawnCurve;
            }
            break;
        }
        syncDrawnCurve(interpolation, previous);
        if (!(interpolation == previous)) {
            PitchBendAutomation.setInterpolation(interpolation);
            changed = true;
        }
    } break;
    case DataRole::Modulation_Cycles: {
        auto modulation = PitchBendAutomation.modulation();
        if (const auto newCycles = value.toUInt(); modulation.cycles != newCycles) {
            modulation.cycles = newCycles;
            PitchBendAutomation.setModulation(modulation);
            changed = true;
        }
    } break;
    case DataRole::Modulation_Amplitude: {
        auto modulation = PitchBendAutomation.modulation();
        if (const auto newAmplitude = value.toFloat(); modulation.amplitude != newAmplitude) {
            modulation.amplitude = newAmplitude;
            PitchBendAutomation.setModulation(modulation);
            changed = true;
        }
    } break;
    case DataRole::Modulation_Offset: {
        auto modulation = PitchBendAutomation.modulation();
        if (const auto newOffset = value.toFloat(); modulation.offset != newOffset) {
            modulation.offset = newOffset;
            PitchBendAutomation.setModulation(modulation);
            changed = true;
        }
    } break;
    case DataRole::Modulation_Inverted: {
        auto modulation = PitchBendAutomation.modulation();
        if (const auto newInverted = value.toBool(); modulation.inverted != newInverted) {
            modulation.inverted = newInverted;
            PitchBendAutomation.setModulation(modulation);
            changed = true;
        }
    } break;
    case DataRole::Modulation_Type: {
        auto modulation = PitchBendAutomation.modulation();
        if (const auto newType = static_cast<ModulationParameters::ModulationType>(value.toInt()); modulation.type != newType) {
            modulation.type = newType;
            PitchBendAutomation.setModulation(modulation);
            changed = true;
        }
    } break;
    case DataRole::EventsPerBeat: {
        const auto newEventsPerBeat = value.toInt() < 0 ? std::nullopt : std::optional { static_cast<uint8_t>(value.toInt()) };
        if (PitchBendAutomation.eventsPerBeat() != newEventsPerBeat) {
            PitchBendAutomation.setEventsPerBeat(newEventsPerBeat);
            changed = true;
        }
    } break;
    case DataRole::LineOffset:
        if (const auto newLineOffset = static_cast<uint8_t>(value.toUInt()); PitchBendAutomation.lineOffset() != newLineOffset) {
            PitchBendAutomation.setLineOffset(newLineOffset);
            changed = true;
        }
        break;
    case DataRole::Pattern:
    case DataRole::Track:
    case DataRole::Column:
    case DataRole::Id:
    case DataRole::CurveShape:
        break;
    }

    if (changed) {
        m_pitchBendAutomations[static_cast<size_t>(index.row())] = PitchBendAutomation;
        m_pitchBendAutomationsChanged.erase(PitchBendAutomation);
        m_pitchBendAutomationsChanged.insert(PitchBendAutomation);
        // Every role for a new shape, which the preview draws and which can move a drawn curve's ends
        emit dataChanged(index, index, reshapes(static_cast<DataRole>(role)) ? QList<int> {} : QList<int> { role });
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
        { static_cast<int>(DataRole::Value1), "value1" },
        { static_cast<int>(DataRole::Curve), "curve" },
        { static_cast<int>(DataRole::DrawnCurve), "drawnCurve" },
        { static_cast<int>(DataRole::CurveShape), "curveShape" },
        { static_cast<int>(DataRole::Modulation_Cycles), "modulationCycles" },
        { static_cast<int>(DataRole::Modulation_Amplitude), "modulationAmplitude" },
        { static_cast<int>(DataRole::Modulation_Offset), "modulationOffset" },
        { static_cast<int>(DataRole::Modulation_Inverted), "modulationInverted" },
        { static_cast<int>(DataRole::Modulation_Type), "modulationType" },
        { static_cast<int>(DataRole::EventsPerBeat), "eventsPerBeat" },
        { static_cast<int>(DataRole::LineOffset), "lineOffset" }
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

void PitchBendAutomationsModel::applyValues(int index, const QVariantMap & values)
{
    if (index < 0 || static_cast<size_t>(index) >= m_pitchBendAutomations.size()) {
        return;
    }

    // Routed through setData() rather than written here, so the per-role rules stay in one place.
    // Pitch bend has no controller, so there is none to carry.
    static const std::vector<std::pair<QString, DataRole>> fields {
        { "line0", DataRole::Line0 },
        { "line1", DataRole::Line1 },
        { "value0", DataRole::Value0 },
        { "value1", DataRole::Value1 },
        { "curve", DataRole::Curve },
        { "modulationType", DataRole::Modulation_Type },
        { "modulationCycles", DataRole::Modulation_Cycles },
        { "modulationAmplitude", DataRole::Modulation_Amplitude },
        { "modulationOffset", DataRole::Modulation_Offset },
        { "modulationInverted", DataRole::Modulation_Inverted },
        { "eventsPerBeat", DataRole::EventsPerBeat },
        { "lineOffset", DataRole::LineOffset },
        { "comment", DataRole::Comment },
        // Last, as the lines and the curve type above would otherwise reshape it
        { "drawnCurve", DataRole::DrawnCurve }
    };

    const auto modelIndex = this->index(index);
    for (auto && [key, role] : fields) {
        if (const auto value = values.find(key); value != values.end()) {
            setData(modelIndex, *value, static_cast<int>(role));
        }
    }

    emit automationReplaced(index);
}

void PitchBendAutomationsModel::changeModulationType(int index, int type)
{
    if (index >= 0 && static_cast<size_t>(index) < m_pitchBendAutomations.size()) {
        auto && pitchBendAutomation = m_pitchBendAutomations[static_cast<size_t>(index)];
        auto modulation = pitchBendAutomation.modulation();
        if (static_cast<int>(modulation.type) != type) {
            modulation.type = static_cast<ModulationParameters::ModulationType>(type);
            pitchBendAutomation.setModulation(modulation);
            m_pitchBendAutomations[static_cast<size_t>(index)] = pitchBendAutomation;
            m_pitchBendAutomationsChanged.erase(pitchBendAutomation);
            m_pitchBendAutomationsChanged.insert(pitchBendAutomation);
            emit dataChanged(this->index(index), this->index(index), { static_cast<int>(DataRole::Modulation_Type) });
            juzzlin::L(TAG).info() << "Pitch bend automation modulation type changed via invokable: " << pitchBendAutomation.toString().toStdString();
        }
    }
}

void PitchBendAutomationsModel::changeCurve(int index, int curve)
{
    // Through setData(), which keeps a drawn curve in step with its type
    setData(this->index(index), curve, static_cast<int>(DataRole::Curve));
}

} // namespace noteahead
