#ifndef NOTE_COLUMN_MODEL_HANDLER_HPP
#define NOTE_COLUMN_MODEL_HANDLER_HPP

#include <QObject>

#include <map>
#include <memory>
#include <tuple>

class QAbstractListModel;

namespace noteahead {

class AutomationService;
class EditorService;
class NoteColumnModel;
class NoteColumnLineContainerHelper;
class SelectionService;
struct Position;

//! Manages all the note column models.
class NoteColumnModelHandler : public QObject
{
    Q_OBJECT

public:
    using EditorServiceS = std::shared_ptr<EditorService>;
    using SelectionServiceS = std::shared_ptr<SelectionService>;
    using AutomationServiceS = std::shared_ptr<AutomationService>;
    using NoteColumnLineContainerHelperS = std::shared_ptr<NoteColumnLineContainerHelper>;
    explicit NoteColumnModelHandler(EditorServiceS editorService, SelectionServiceS selectionService, AutomationServiceS automationService, QObject * parent = nullptr);

    Q_INVOKABLE QAbstractListModel * columnModel(quint64 pattern, quint64 track, quint64 column);
    Q_INVOKABLE void updateColumnData();

signals:
    void columnDataUpdateRequested();

private:
    using NoteColumnModelP = NoteColumnModel *;
    void connectColumnModel(NoteColumnModelP noteColumnModel);

    using ColumnAddress = std::tuple<quint64, quint64, quint64>; // Pattern, Track, Column
    ColumnAddress positionToColumnAddress(const Position & position) const;

    void updateIndexHighlightAtPosition(const Position & position);
    void updateIndexHighlightRange(const Position & startPosition, const Position & endPosition);
    void updateNoteDataAtPosition(const Position & position);

    EditorServiceS m_editorService;
    SelectionServiceS m_selectionService;
    AutomationServiceS m_automationService;
    NoteColumnLineContainerHelperS m_helper;

    using NoteColumnModelMap = std::map<ColumnAddress, NoteColumnModelP>;
    NoteColumnModelMap m_noteColumnModels;
};

} // namespace noteahead

#endif // NOTE_COLUMN_MODEL_HANDLER_HPP
