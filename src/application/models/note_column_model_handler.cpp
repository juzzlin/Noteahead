#include "note_column_model_handler.hpp"

#include "../../application/position.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/util_service.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "note_column_line_container_helper.hpp"
#include "note_column_model.hpp"

namespace noteahead {

static const auto TAG = "NoteColumnModelHandler";

NoteColumnModelHandler::NoteColumnModelHandler(EditorServiceS editorService, SelectionServiceS selectionService, AutomationServiceS automationService, QObject * parent)
  : QObject { parent }
  , m_editorService { editorService }
  , m_selectionService { selectionService }
  , m_automationService { automationService }
  , m_helper { std::make_unique<NoteColumnLineContainerHelper>(
      m_automationService, m_editorService, m_selectionService, std::make_unique<UtilService>()) }

{
    connect(m_automationService.get(), &AutomationService::lineDataChanged, this, &NoteColumnModelHandler::updateIndexHighlightAtPosition);

    connect(m_editorService.get(), &EditorService::lineDataChanged, this, &NoteColumnModelHandler::updateIndexHighlightAtPosition);
    connect(m_editorService.get(), &EditorService::noteDataAtPositionChanged, this, &NoteColumnModelHandler::updateNoteDataAtPosition);
    connect(m_editorService.get(), &EditorService::positionChanged, this, &NoteColumnModelHandler::updatePosition);

    connect(m_selectionService.get(), &SelectionService::selectionCleared, this, &NoteColumnModelHandler::updateIndexHighlightRange);
    connect(m_selectionService.get(), &SelectionService::selectionChanged, this, &NoteColumnModelHandler::updateIndexHighlightRange);
}

NoteColumnModelHandler::ColumnAddress NoteColumnModelHandler::positionToColumnAddress(const Position & position) const
{
    return {
        position.pattern, position.track, position.column
    };
}

void NoteColumnModelHandler::updateIndexHighlightAtPosition(const Position & position)
{
    if (const auto columnAddress = positionToColumnAddress(position); m_noteColumnModels.contains(columnAddress)) {
        m_noteColumnModels.at(columnAddress)->updateIndexHighlightAtPosition(position.line);
    }
}

void NoteColumnModelHandler::updateIndexHighlightRange(const Position & startPosition, const Position & endPosition)
{
    if (const auto columnAddress = positionToColumnAddress(startPosition); m_noteColumnModels.contains(columnAddress)) {
        m_noteColumnModels.at(columnAddress)->updateIndexHighlightRange(startPosition.line, endPosition.line);
    }
}

void NoteColumnModelHandler::updateNoteDataAtPosition(const Position & position)
{
    if (const auto columnAddress = positionToColumnAddress(position); m_noteColumnModels.contains(columnAddress)) {
        m_noteColumnModels.at(columnAddress)->updateNoteDataAtPosition(position.line);
    }
}

void NoteColumnModelHandler::updatePosition(const Position & newPosition, const Position & oldPosition)
{
    if (const auto columnAddress = positionToColumnAddress(oldPosition); m_noteColumnModels.contains(columnAddress)) {
        juzzlin::L(TAG).info() << "Unfocusing position " << oldPosition.toString();
        m_noteColumnModels.at(columnAddress)->setLineUnfocused(oldPosition.line);
    }

    if (const auto columnAddress = positionToColumnAddress(newPosition); m_noteColumnModels.contains(columnAddress)) {
        juzzlin::L(TAG).info() << "Focusing position " << newPosition.toString();
        m_noteColumnModels.at(columnAddress)->setLineFocused(newPosition.line, newPosition.lineColumn);
    }
}

QAbstractListModel * NoteColumnModelHandler::columnModel(quint64 pattern, quint64 track, quint64 column)
{
    if (const auto columnAddress = ColumnAddress { pattern, track, column }; !m_noteColumnModels.contains(columnAddress)) {
        juzzlin::L(TAG).info() << "Creating note column model for pattern=" << pattern << ", track=" << track << ", column=" << column;
        const auto noteColumnModel = new NoteColumnModel { columnAddress, m_editorService, m_helper, this };
        connectColumnModel(noteColumnModel);
        m_noteColumnModels[columnAddress] = noteColumnModel;
        return noteColumnModel;
    } else {
        juzzlin::L(TAG).info() << "Reusing note column model for pattern=" << pattern << ", track=" << track << ", column=" << column;
        m_noteColumnModels.at(columnAddress)->clear();
        return m_noteColumnModels.at(columnAddress);
    }
}

void NoteColumnModelHandler::connectColumnModel(NoteColumnModelP noteColumnModel)
{
    connect(this, &NoteColumnModelHandler::columnDataUpdateRequested, noteColumnModel, &NoteColumnModel::requestColumnData);
    connect(noteColumnModel, &NoteColumnModel::columnDataRequested, m_editorService.get(), &EditorService::requestColumnData);
    connect(m_editorService.get(), &EditorService::columnDataUpdated, noteColumnModel, &NoteColumnModel::setColumnData);
    connect(m_editorService.get(), &EditorService::linesPerBeatChanged, noteColumnModel, &NoteColumnModel::updateIndexHighlights);
}

void NoteColumnModelHandler::updateColumnData()
{
    emit columnDataUpdateRequested();
}

} // namespace noteahead
