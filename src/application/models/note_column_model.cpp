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

#include "note_column_model.hpp"

#include "../../application/config.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../domain/line.hpp"
#include "../note_converter.hpp"
#include "note_column_line_container_helper.hpp"

#include <QColor>

namespace noteahead {

NoteColumnModel::NoteColumnModel(ColumnAddressCR columnAddress, EditorServiceS editorService, NoteColumnLineContainerHelperS helper, ConfigS config, QObject * parent)
  : QAbstractListModel { parent }
  , m_columnAddress { columnAddress }
  , m_editorService { editorService }
  , m_helper { helper }
  , m_config { config }
{
    connect(m_config.get(), &Config::visibleLinesChanged, this, [this]() {
        beginResetModel();
        endResetModel();
    });
}

int NoteColumnModel::rowCount(const QModelIndex & parent) const
{
    // As ListView cannot scroll beyond its last line, we'll need a hack
    const int virtualLineCount = m_config->visibleLines();
    return parent.isValid() ? 0 : static_cast<int>(m_lines.size()) + virtualLineCount;
}

QString NoteColumnModel::noDataString() const
{
    return "---";
}

QString NoteColumnModel::displayNote(const Line & line) const
{
    if (const auto noteData = line.noteData(); noteData->type() != NoteData::Type::None) {
        return noteData->type() == NoteData::Type::NoteOff ? "OFF" : QString::fromStdString(NoteConverter::midiToString(*noteData->note()));
    } else {
        return noDataString();
    }
}

QString NoteColumnModel::padVelocityToThreeDigits(const QString & velocity) const
{
    return velocity.rightJustified(3, '0', true);
}

QString NoteColumnModel::displayVelocity(const Line & line) const
{
    if (const auto noteData = line.noteData(); noteData->type() != NoteData::Type::None) {
        return noteData->type() == NoteData::Type::NoteOff ? noDataString() : padVelocityToThreeDigits(QString::number(noteData->velocity()));
    } else {
        return noDataString();
    }
}

QVariant NoteColumnModel::lineColor(quint64 lineIndex) const
{
    const auto [pattern, track, column] = m_columnAddress;
    return m_helper->lineColorAndBorderWidth(pattern, track, column, static_cast<size_t>(lineIndex)).at(0);
}

QVariant NoteColumnModel::borderWidth(quint64 lineIndex) const
{
    const auto [pattern, track, column] = m_columnAddress;
    return m_helper->lineColorAndBorderWidth(pattern, track, column, lineIndex).at(1);
}

QVariant NoteColumnModel::virtualLineData(int role) const
{
    using enum DataRole;
    switch (static_cast<DataRole>(role)) {
    case Border:
        return 0;
    case Color:
        return QColor { Qt::black };
    case LineColumn:
        return 0;
    case Note:
        return "";
    case Velocity:
        return "";
    case IsFocused:
        return false;
    case IsVirtualRow:
        return true;
    }
    return {};
}

QVariant NoteColumnModel::data(const QModelIndex & index, int role) const
{
    if (const int shiftedIndex = index.row() - static_cast<int>(m_editorService->positionBarLine());
        shiftedIndex < 0 || shiftedIndex >= static_cast<int>(m_lines.size())) {
        return virtualLineData(role);
    } else {
        const auto & line = m_lines.at(static_cast<size_t>(shiftedIndex));
        using enum DataRole;
        switch (static_cast<DataRole>(role)) {
        case Border:
            return borderWidth(static_cast<size_t>(shiftedIndex));
        case Color:
            return lineColor(static_cast<size_t>(shiftedIndex));
        case LineColumn:
            return m_focusedLines.contains(static_cast<size_t>(index.row())) ? m_focusedLines.at(static_cast<size_t>(index.row())) : 0;
        case Note:
            return displayNote(*line);
        case Velocity:
            return displayVelocity(*line);
        case IsFocused:
            return m_focusedLines.contains(static_cast<size_t>(index.row()));
        case IsVirtualRow:
            return false;
        }
    }
    return {};
}

QHash<int, QByteArray> NoteColumnModel::roleNames() const
{
    using enum DataRole;
    return {
        { static_cast<int>(Border), "border" },
        { static_cast<int>(Color), "color" },
        { static_cast<int>(IsFocused), "isFocused" },
        { static_cast<int>(IsVirtualRow), "isVirtualRow" },
        { static_cast<int>(LineColumn), "lineColumn" },
        { static_cast<int>(Note), "note" },
        { static_cast<int>(Velocity), "velocity" },
    };
}

void NoteColumnModel::setColumnData(LineListCR lines)
{
    beginResetModel();
    m_lines = lines;
    m_focusedLines.clear();
    endResetModel();
}

void NoteColumnModel::setColumnAddress(const ColumnAddress & columnAddress)
{
    m_columnAddress = columnAddress;
}

void NoteColumnModel::clear()
{
    beginResetModel();
    m_lines.clear();
    m_focusedLines.clear();
    endResetModel();
}

void NoteColumnModel::notifyDataChanged(int startLine, int endLine, const QList<int> & roles)
{
    if (!m_lines.empty()) {
        if (startLine > endLine) {
            std::swap(startLine, endLine);
        }
        const auto topIndex = index(startLine + static_cast<int>(m_editorService->positionBarLine()), 0);
        const auto bottomIndex = index(endLine + static_cast<int>(m_editorService->positionBarLine()), 0);
        emit dataChanged(topIndex, bottomIndex, roles);
    }
}

void NoteColumnModel::updateIndexHighlights()
{
    notifyDataChanged(0, rowCount() - 1, { static_cast<int>(DataRole::Color), static_cast<int>(DataRole::Border) });
}

void NoteColumnModel::updateIndexHighlightAtPosition(quint64 line)
{
    notifyDataChanged(static_cast<int>(line), static_cast<int>(line), { static_cast<int>(DataRole::Color), static_cast<int>(DataRole::Border) });
}

void NoteColumnModel::updateIndexHighlightRange(quint64 startLine, quint64 endLine)
{
    notifyDataChanged(static_cast<int>(startLine), static_cast<int>(endLine), { static_cast<int>(DataRole::Color), static_cast<int>(DataRole::Border) });
}

void NoteColumnModel::updateNoteDataAtPosition(quint64 line)
{
    notifyDataChanged(static_cast<int>(line), static_cast<int>(line), { static_cast<int>(DataRole::Note), static_cast<int>(DataRole::Velocity) });
}

void NoteColumnModel::setLineFocused(quint64 line, quint64 column)
{
    if (!m_lines.empty()) {
        m_focusedLines[line + m_editorService->positionBarLine()] = column;
        notifyDataChanged(static_cast<int>(line), static_cast<int>(line), { static_cast<int>(DataRole::LineColumn), static_cast<int>(DataRole::IsFocused) });
    }
}

void NoteColumnModel::setLineUnfocused(quint64 line)
{
    if (!m_lines.empty()) {
        m_focusedLines.erase(line + m_editorService->positionBarLine());
        notifyDataChanged(static_cast<int>(line), static_cast<int>(line), { static_cast<int>(DataRole::LineColumn), static_cast<int>(DataRole::IsFocused) });
    }
}

} // namespace noteahead
