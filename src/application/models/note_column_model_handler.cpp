#include "note_column_model_handler.hpp"

#include "../../application/service/editor_service.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "note_column_model.hpp"

namespace noteahead {

static const auto TAG = "NoteColumnModelHandler";

NoteColumnModelHandler::NoteColumnModelHandler(EditorServiceS editorService, QObject * parent)
  : QObject { parent }
  , m_editorService { editorService }
{
}

// TODO: Don't store to any map. Just connect to editorservice
QAbstractListModel * NoteColumnModelHandler::columnModel(quint64 pattern, quint64 track, quint64 column)
{
    const auto location = PatternTrackColumn { pattern, track, column };
    if (!m_noteColumnModels.contains(location)) {
        juzzlin::L(TAG).info() << "Creating note column model for pattern=" << pattern << ", track=" << track << ", column=" << column;
        const auto noteColumnModel = new NoteColumnModel { location, m_editorService, this };
        connectColumnModel(noteColumnModel);
        m_noteColumnModels[location] = noteColumnModel;
        return noteColumnModel;
    } else {
        m_noteColumnModels.at(location)->clear();
        return m_noteColumnModels.at(location);
    }
}

void NoteColumnModelHandler::connectColumnModel(NoteColumnModelP noteColumnModel)
{
    connect(this, &NoteColumnModelHandler::columnDataUpdateRequested, noteColumnModel, &NoteColumnModel::requestColumnData);
    connect(noteColumnModel, &NoteColumnModel::columnDataRequested, m_editorService.get(), &EditorService::requestColumnData);
    connect(m_editorService.get(), &EditorService::columnDataUpdated, noteColumnModel, &NoteColumnModel::setColumnData);
}

void NoteColumnModelHandler::updateColumnData()
{
    emit columnDataUpdateRequested();
}

} // namespace noteahead
