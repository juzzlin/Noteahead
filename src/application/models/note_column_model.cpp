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

#include "../../application/service/editor_service.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/line.hpp"
#include "../note_converter.hpp"
#include "note_column_line_container_helper.hpp"

#include <QColor>

namespace noteahead {

static const auto TAG = "NoteColumnModel";

NoteColumnModel::NoteColumnModel(PatternTrackColumn location, EditorServiceS editorService, NoteColumnLineContainerHelperS helper, QObject * parent)
  : QAbstractListModel { parent }
  , m_location { location }
  , m_editorService { editorService }
  , m_helper { helper }
{
}

int NoteColumnModel::rowCount(const QModelIndex & parent) const
{
    const int virtualLineCount = 64; // TODO: This should be calculated from config.visibleLines
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
    const auto [pattern, track, column] = m_location;
    return m_helper->lineColorAndBorderWidth(pattern, track, column, static_cast<size_t>(lineIndex)).at(0);
}

QVariant NoteColumnModel::borderWidth(quint64 lineIndex) const
{
    const auto [pattern, track, column] = m_location;
    return m_helper->lineColorAndBorderWidth(pattern, track, column, lineIndex).at(1);
}

QVariant NoteColumnModel::data(const QModelIndex & index, int role) const
{
    const int shiftedIndex = index.row() - static_cast<int>(m_editorService->positionBarLine());
    if (shiftedIndex < 0 || shiftedIndex >= static_cast<int>(m_lines.size())) {
        using enum DataRole;
        switch (static_cast<DataRole>(role)) {
        case Border:
            return 0;
        case Color:
            return QColor { Qt::black };
        case Note:
            return "";
        case Velocity:
            return "";
        case IsVirtualRow:
            return true;
        }
    } else {
        const auto & line = m_lines.at(static_cast<size_t>(shiftedIndex));
        using enum DataRole;
        switch (static_cast<DataRole>(role)) {
        case Border:
            return borderWidth(static_cast<size_t>(shiftedIndex));
        case Color:
            return lineColor(static_cast<size_t>(shiftedIndex));
        case Note:
            return displayNote(*line);
        case Velocity:
            return displayVelocity(*line);
        case IsVirtualRow:
            return false;
        }
    }
    return {};
}

Qt::ItemFlags NoteColumnModel::flags(const QModelIndex & index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QHash<int, QByteArray> NoteColumnModel::roleNames() const
{
    using enum DataRole;
    return {
        { static_cast<int>(Border), "border" },
        { static_cast<int>(Color), "color" },
        { static_cast<int>(Note), "note" },
        { static_cast<int>(Velocity), "velocity" },
        { static_cast<int>(IsVirtualRow), "isVirtualRow" },
    };
}

void NoteColumnModel::requestColumnData()
{
    emit columnDataRequested(m_location);
}

void NoteColumnModel::setColumnData(PatternTrackColumn location, LineListCR lines)
{
    if (m_location == location) {
        beginResetModel();
        m_lines = lines;
        endResetModel();
    }
}

void NoteColumnModel::clear()
{
    beginResetModel();
    m_lines.clear();
    endResetModel();
}

void NoteColumnModel::updateIndexHighlights()
{
    if (m_lines.empty()) {
        return;
    }

    const auto topIndex = index(0, 0);
    const auto bottomIndex = index(rowCount() - 1, 0);
    emit dataChanged(topIndex, bottomIndex, { static_cast<int>(DataRole::Color), static_cast<int>(DataRole::Border) });
}

void NoteColumnModel::updateIndexHighlightAtPosition(quint64 line)
{
    if (m_lines.empty()) {
        return;
    }

    const auto lineIndex = index(static_cast<int>(line + m_editorService->positionBarLine()), 0);
    emit dataChanged(lineIndex, lineIndex, { static_cast<int>(DataRole::Color), static_cast<int>(DataRole::Border) });
}

void NoteColumnModel::updateIndexHighlightRange(quint64 startLine, quint64 endLine)
{
    if (m_lines.empty()) {
        return;
    }

    if (startLine > endLine) {
        std::swap(startLine, endLine);
    }

    const auto topIndex = index(static_cast<int>(startLine + m_editorService->positionBarLine()), 0);
    const auto bottomIndex = index(static_cast<int>(endLine + m_editorService->positionBarLine()), 0);
    emit dataChanged(topIndex, bottomIndex, { static_cast<int>(DataRole::Color), static_cast<int>(DataRole::Border) });
}

void NoteColumnModel::updateNoteDataAtPosition(quint64 line)
{
    if (m_lines.empty()) {
        return;
    }

    const auto lineIndex = index(static_cast<int>(line + m_editorService->positionBarLine()), 0);
    emit dataChanged(lineIndex, lineIndex, { static_cast<int>(DataRole::Note), static_cast<int>(DataRole::Velocity) });
}

} // namespace noteahead
