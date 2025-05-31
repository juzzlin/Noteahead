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

#include "note_column_line_container_helper.hpp"

#include "../service/automation_service.hpp"
#include "../service/editor_service.hpp"
#include "../service/selection_service.hpp"
#include "../service/util_service.hpp"

#include <QColor>

namespace noteahead {

NoteColumnLineContainerHelper::NoteColumnLineContainerHelper(
  AutomationServiceS automationService, EditorServiceS editorService, SelectionServiceS selectionService, UtilServiceS utilService, QObject * parent)
  : QObject { parent }
  , m_automationService { automationService }
  , m_editorService { editorService }
  , m_selectionService { selectionService }
  , m_utilService { utilService }
{
}

QList<QVariant> NoteColumnLineContainerHelper::lineColorAndBorderWidth(quint64 patternIndex, quint64 trackIndex, quint64 columnIndex, quint64 lineIndex) const
{
    QColor color;
    int borderWidth = 0;

    const int linesPerBeat = static_cast<int>(m_editorService->linesPerBeat());
    if (m_selectionService->isSelected(patternIndex, trackIndex, columnIndex, lineIndex)) {
        const QColor baseColor = m_utilService->scaledColor(QColor("#ffffff"), m_utilService->indexHighlightOpacity(lineIndex, linesPerBeat));
        color = m_utilService->blendColors(baseColor, QColor("#ffa500"), 0.5); // Blend 50% with orange
        borderWidth = 0;
    } else {
        if (m_editorService->hasInstrumentSettings(patternIndex, trackIndex, columnIndex, lineIndex)) {
            const QColor baseColor = m_utilService->scaledColor(QColor("#ffffff"), m_utilService->indexHighlightOpacity(lineIndex, linesPerBeat));
            color = m_utilService->blendColors(baseColor, QColor("#3e65ff"), 0.5); // Universal.Cobalt
            borderWidth = 1;
        } else if (m_automationService->hasAutomations(patternIndex, trackIndex, columnIndex, lineIndex)) {
            const QColor baseColor = m_utilService->scaledColor(QColor("#ffffff"), m_utilService->indexHighlightOpacity(lineIndex, linesPerBeat));
            const qreal automationWeight = m_automationService->automationWeight(patternIndex, trackIndex, columnIndex, lineIndex);
            const QColor automationColor = m_utilService->blendColors(QColor("#e51400"), QColor("#60a917"), automationWeight); // Universal.Red -> Universal.Green
            color = m_utilService->blendColors(baseColor, automationColor, 0.75);
            borderWidth = 1;
        } else {
            color = m_utilService->scaledColor(QColor("#ffffff"), m_utilService->indexHighlightOpacity(lineIndex, linesPerBeat));
            borderWidth = 1;
        }
    }

    QList<QVariant> result;
    result.append(QVariant::fromValue(color)); // The color
    result.append(QVariant(borderWidth)); // The border width
    return result;
}

} // namespace noteahead
