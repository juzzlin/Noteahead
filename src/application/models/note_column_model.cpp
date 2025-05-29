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

namespace noteahead {

static const auto TAG = "NoteColumnModel";

NoteColumnModel::NoteColumnModel(PatternTrackColumn location, EditorServiceS editorService, QObject * parent)
  : QAbstractListModel { parent }
  , m_location { location }
  , m_editorService { editorService }
{
}

int NoteColumnModel::rowCount(const QModelIndex & parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_lines.size()) + 256;
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

QVariant NoteColumnModel::data(const QModelIndex & index, int role) const
{
    const int shiftedIndex = index.row() - static_cast<int>(m_editorService->positionBarLine());
    if (shiftedIndex < 0 || shiftedIndex >= static_cast<int>(m_lines.size())) {
        using enum DataRole;
        switch (static_cast<DataRole>(role)) {
        case Note:
            return "";
        case Velocity:
            return "";
        case Padding:
            return true;
        }
    } else {
        const auto & line = m_lines.at(shiftedIndex);
        using enum DataRole;
        switch (static_cast<DataRole>(role)) {
        case Note:
            return displayNote(*line);
        case Velocity:
            return displayVelocity(*line);
        case Padding:
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
        { static_cast<int>(Note), "note" },
        { static_cast<int>(Velocity), "velocity" },
        { static_cast<int>(Padding), "padding" },
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

} // namespace noteahead
