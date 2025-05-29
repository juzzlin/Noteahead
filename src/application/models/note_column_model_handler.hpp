#ifndef NOTE_COLUMN_MODEL_HANDLER_HPP
#define NOTE_COLUMN_MODEL_HANDLER_HPP

#include <QObject>

#include <map>
#include <memory>
#include <mutex>
#include <tuple>

class QAbstractListModel;

namespace noteahead {

class EditorService;
class NoteColumnModel;

class NoteColumnModelHandler : public QObject
{
    Q_OBJECT

public:
    using EditorServiceS = std::shared_ptr<EditorService>;
    explicit NoteColumnModelHandler(EditorServiceS editorService, QObject * parent = nullptr);

    Q_INVOKABLE QAbstractListModel * columnModel(quint64 pattern, quint64 track, quint64 column);

    Q_INVOKABLE void updateColumnData();

signals:
    void columnDataUpdateRequested();

private:
    using NoteColumnModelP = NoteColumnModel *;
    void connectColumnModel(NoteColumnModelP noteColumnModel);

    EditorServiceS m_editorService;

    using PatternTrackColumn = std::tuple<quint64, quint64, quint64>;
    using NoteColumnModelMap = std::map<PatternTrackColumn, NoteColumnModelP>;
    NoteColumnModelMap m_noteColumnModels;

    std::mutex m_mutex;
};

} // namespace noteahead

#endif // NOTE_COLUMN_MODEL_HANDLER_HPP
