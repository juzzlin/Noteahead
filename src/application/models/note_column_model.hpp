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

#ifndef NOTE_COLUMN_MODEL_HPP
#define NOTE_COLUMN_MODEL_HPP

#include <tuple>

#include <QAbstractListModel>
#include <QObject>

namespace noteahead {

class EditorService;
class Line;

class NoteColumnModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum class DataRole
    {
        Note = Qt::UserRole + 1,
        Velocity,
        Padding
    };
    Q_ENUM(DataRole)

    using PatternTrackColumn = std::tuple<quint64, quint64, quint64>;
    using EditorServiceS = std::shared_ptr<EditorService>;
    explicit NoteColumnModel(PatternTrackColumn location, EditorServiceS editorService, QObject * parent = nullptr);

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex & index) const override;
    QHash<int, QByteArray> roleNames() const override;

    using LineS = std::shared_ptr<Line>;
    using LineList = std::vector<LineS>;
    using LineListCR = const LineList &;
    void requestColumnData();
    void setColumnData(PatternTrackColumn location, LineListCR lines);
    void clear();

signals:
    void columnDataRequested(PatternTrackColumn location);

private:
    QString noDataString() const;
    QString displayNote(const Line & line) const;
    QString padVelocityToThreeDigits(const QString & velocity) const;
    QString displayVelocity(const Line & line) const;

    PatternTrackColumn m_location;

    EditorServiceS m_editorService;

    struct NoteItem
    {
        int note;
        int velocity;
    };

    LineList m_lines;
};

} // namespace noteahead

#endif // NOTE_COLUMN_MODEL_HPP
