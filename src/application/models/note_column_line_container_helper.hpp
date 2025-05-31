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

#ifndef NOTE_COLUMN_LINE_CONTAINER_HELPER_HPP
#define NOTE_COLUMN_LINE_CONTAINER_HELPER_HPP

#include <QList>
#include <QObject>
#include <QVariant>

namespace noteahead {

class AutomationService;
class EditorService;
class SelectionService;
class UtilService;

class NoteColumnLineContainerHelper : public QObject
{
    Q_OBJECT

public:
    using AutomationServiceS = std::shared_ptr<AutomationService>;
    using EditorServiceS = std::shared_ptr<EditorService>;
    using SelectionServiceS = std::shared_ptr<SelectionService>;
    using UtilServiceS = std::shared_ptr<UtilService>;
    explicit NoteColumnLineContainerHelper(
      AutomationServiceS automationService, EditorServiceS editorService, SelectionServiceS selectionService, UtilServiceS utilService, QObject * parent = nullptr);

    Q_INVOKABLE QList<QVariant> lineColorAndBorderWidth(quint64 patternIndex, quint64 trackIndex, quint64 columnIndex, quint64 lineIndex) const;

private:
    AutomationServiceS m_automationService;
    EditorServiceS m_editorService;
    SelectionServiceS m_selectionService;
    UtilServiceS m_utilService;
};

} // namespace noteahead

#endif // NOTE_COLUMN_LINE_CONTAINER_HELPER_HPP
