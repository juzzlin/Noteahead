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

NoteColumnModelHandler::NoteColumnModelHandler(EditorServiceS editorService, SelectionServiceS selectionService, AutomationServiceS automationService, SettingsServiceS settingsService, QObject * parent)
  : QObject { parent }
  , m_editorService { editorService }
  , m_selectionService { selectionService }
  , m_automationService { automationService }
  , m_helper { std::make_unique<NoteColumnLineContainerHelper>(
      m_automationService, m_editorService, m_selectionService, std::make_unique<UtilService>()) }
  , m_settingsService { settingsService }
{
    connect(m_automationService.get(), &AutomationService::lineDataChanged, this, &NoteColumnModelHandler::updateIndexHighlightAtPosition);

    connect(m_editorService.get(), &EditorService::currentLineCountChanged, this, &NoteColumnModelHandler::updateCurrentLineCount);
    connect(m_editorService.get(), &EditorService::lineDataChanged, this, &NoteColumnModelHandler::updateIndexHighlightAtPosition);
    connect(m_editorService.get(), &EditorService::noteDataAtPositionChanged, this, &NoteColumnModelHandler::updateNoteDataAtPosition);
    connect(m_editorService.get(), &EditorService::positionChanged, this, &NoteColumnModelHandler::updatePosition);

    connect(m_selectionService.get(), &SelectionService::selectionCleared, this, &NoteColumnModelHandler::updateIndexHighlightRange);
    connect(m_selectionService.get(), &SelectionService::selectionChanged, this, &NoteColumnModelHandler::updateIndexHighlightRange);
}

void NoteColumnModelHandler::updateCurrentLineCount()
{
    updatePattern(m_editorService->position().pattern);
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
        m_noteColumnModels.at(columnAddress)->setLineUnfocused(oldPosition.line);
    }

    if (const auto columnAddress = positionToColumnAddress(newPosition); m_noteColumnModels.contains(columnAddress)) {
        m_noteColumnModels.at(columnAddress)->setLineFocused(newPosition.line, newPosition.lineColumn);
    }
}

QAbstractListModel * NoteColumnModelHandler::columnModel(quint64 pattern, quint64 track, quint64 column)
{
    if (const auto columnAddress = ColumnAddress { pattern, track, column }; !m_noteColumnModels.contains(columnAddress)) {
        juzzlin::L(TAG).debug() << "Creating note column model for pattern=" << pattern << ", track=" << track << ", column=" << column;
        const auto model = new NoteColumnModel { columnAddress, m_editorService, m_helper, m_settingsService, this };
        connectColumnModel(model);
        m_noteColumnModels[columnAddress] = model;
        return model;
    } else {
        return m_noteColumnModels.at(columnAddress);
    }
}

void NoteColumnModelHandler::connectColumnModel(NoteColumnModelP noteColumnModel)
{
    connect(m_editorService.get(), &EditorService::linesPerBeatChanged, noteColumnModel, &NoteColumnModel::updateIndexHighlights);
}

void NoteColumnModelHandler::disconnectColumnModel(NoteColumnModelP noteColumnModel)
{
    disconnect(m_editorService.get(), &EditorService::linesPerBeatChanged, noteColumnModel, &NoteColumnModel::updateIndexHighlights);
}

void NoteColumnModelHandler::updateAll()
{
    for (auto && [address, model] : m_noteColumnModels) {
        model->setColumnData(m_editorService->columnData(address));
    }
}

void NoteColumnModelHandler::updateColumns(quint64 track)
{
    for (auto && [address, model] : m_noteColumnModels) {
        if (std::get<1>(address) == track) {
            model->setColumnData(m_editorService->columnData(address));
        }
    }
}

void NoteColumnModelHandler::updatePattern(quint64 pattern)
{
    for (auto && [address, model] : m_noteColumnModels) {
        if (std::get<0>(address) == pattern) {
            model->setColumnData(m_editorService->columnData(address));
        }
    }
}

void NoteColumnModelHandler::clear()
{
    for (auto && [address, model] : m_noteColumnModels) {
        model->clear();
        delete model;
    }
    m_noteColumnModels.clear();
}

NoteColumnModelHandler::~NoteColumnModelHandler()
{
    clear();
}

} // namespace noteahead
