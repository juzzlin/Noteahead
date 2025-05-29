#ifndef NOTE_COLUMN_MODEL_HANDLER_HPP
#define NOTE_COLUMN_MODEL_HANDLER_HPP

#include <QObject>

#include <map>
#include <memory>
#include <tuple>

class QAbstractListModel;

namespace noteahead {

class AutomationService;
class Config;
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
    using ConfigS = std::shared_ptr<Config>;
    using EditorServiceS = std::shared_ptr<EditorService>;
    using SelectionServiceS = std::shared_ptr<SelectionService>;
    using AutomationServiceS = std::shared_ptr<AutomationService>;
    using NoteColumnLineContainerHelperS = std::shared_ptr<NoteColumnLineContainerHelper>;
    explicit NoteColumnModelHandler(EditorServiceS editorService, SelectionServiceS selectionService, AutomationServiceS automationService, ConfigS config, QObject * parent = nullptr);
    virtual ~NoteColumnModelHandler() override;

    Q_INVOKABLE QAbstractListModel * columnModel(quint64 pattern, quint64 track, quint64 column);
    Q_INVOKABLE void updateAll();
    Q_INVOKABLE void updateColumns(quint64 track);
    Q_INVOKABLE void updatePattern(quint64 pattern);
    Q_INVOKABLE void clear();

signals:
    void updateAllRequested();

private:
    using NoteColumnModelP = NoteColumnModel *;
    void connectColumnModel(NoteColumnModelP noteColumnModel);
    void disconnectColumnModel(NoteColumnModelP noteColumnModel);

    using ColumnAddress = std::tuple<quint64, quint64, quint64>; // Pattern, Track, Column
    ColumnAddress positionToColumnAddress(const Position & position) const;

    void updateIndexHighlightAtPosition(const Position & position);
    void updateIndexHighlightRange(const Position & startPosition, const Position & endPosition);
    void updateNoteDataAtPosition(const Position & position);
    void updatePosition(const Position & newPosition, const Position & oldPosition);

    EditorServiceS m_editorService;
    SelectionServiceS m_selectionService;
    AutomationServiceS m_automationService;
    NoteColumnLineContainerHelperS m_helper;
    ConfigS m_config;

    using NoteColumnModelMap = std::map<ColumnAddress, NoteColumnModelP>;
    NoteColumnModelMap m_noteColumnModels;
};

} // namespace noteahead

#endif // NOTE_COLUMN_MODEL_HANDLER_HPP
