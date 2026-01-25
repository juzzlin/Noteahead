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
#include <unordered_set>

#include <QAbstractListModel>
#include <QObject>

namespace noteahead {

class SettingsService;
class EditorService;
class Line;
class NoteColumnLineContainerHelper;

class NoteColumnModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum class DataRole
    {
        Note = Qt::UserRole + 1,
        Border,
        Color,
        Delay,
        IsFocused,
        IsVirtualRow,
        Line,
        LineColumn,
        Velocity,
    };
    Q_ENUM(DataRole)

    using ColumnAddress = std::tuple<quint64, quint64, quint64>;
    using ColumnAddressCR = const ColumnAddress &;
    using SettingsServiceS = std::shared_ptr<SettingsService>;
    using EditorServiceS = std::shared_ptr<EditorService>;
    using NoteColumnLineContainerHelperS = std::shared_ptr<NoteColumnLineContainerHelper>;
    explicit NoteColumnModel(ColumnAddressCR columnAddress, EditorServiceS editorService, NoteColumnLineContainerHelperS helper, SettingsServiceS settingsService, QObject * parent = nullptr);

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    using LineS = std::shared_ptr<Line>;
    using LineList = std::vector<LineS>;
    using LineListCR = const LineList &;
    void setColumnData(LineListCR lines);
    void setColumnAddress(const ColumnAddress & columnAddress);
    void clear();

    void setLineFocused(quint64 line, quint64 column);
    void setLineUnfocused(quint64 line);

    void updateIndexHighlights();
    void updateIndexHighlightAtPosition(quint64 line);
    void updateIndexHighlightRange(quint64 startLine, quint64 endLine);
    void updateNoteDataAtPosition(quint64 line);

private:
    QString displayNote(const Line & line) const;
    QString displayVelocity(const Line & line) const;
    QString displayDelay(const Line & line) const;
    QString displayLine(const Line & line) const;
    QString noDataString() const;
    QString padVelocityToThreeDigits(const QString & velocity) const;
    QString padDelayToTwoDigits(const QString & delay) const;
    QVariant lineColor(quint64 lineIndex) const;
    QVariant borderWidth(quint64 lineIndex) const;

    void notifyDataChanged(int startLine, int endLine, const QList<int> & roles = QList<int> {});
    QVariant virtualLineData(int role) const;

    void updateRowCount();

    ColumnAddress m_columnAddress;
    EditorServiceS m_editorService;
    NoteColumnLineContainerHelperS m_helper;
    SettingsServiceS m_settingsService;

    LineList m_lines;
    std::unordered_map<quint64, quint64> m_focusedLines;
};

} // namespace noteahead

#endif // NOTE_COLUMN_MODEL_HPP
