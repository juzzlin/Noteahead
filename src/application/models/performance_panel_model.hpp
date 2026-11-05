// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef PERFORMANCE_PANEL_MODEL_HPP
#define PERFORMANCE_PANEL_MODEL_HPP

#include <QAbstractListModel>

#include <vector>

namespace noteahead {

class ProjectReader;
class ProjectWriter;

//! What the Performance Panel shows, left to right: one element per row, each a type (see
//! Constants::PanelElementType) and the id of what it shows, such as a device's uuid. Saved with
//! the project, since the elements point into it.
class PerformancePanelModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum class Roles
    {
        TypeRole = Qt::UserRole + 1,
        SourceIdRole
    };

    struct Element
    {
        QString type;
        QString sourceId;
        bool operator==(const Element & other) const = default;
    };

    using ElementList = std::vector<Element>;

    explicit PerformancePanelModel(QObject * parent = nullptr);

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    //! Appends the element unless the panel already shows it. Returns whether it was added.
    Q_INVOKABLE bool addElement(const QString & type, const QString & sourceId);
    Q_INVOKABLE void removeElement(int row);
    //! Moves the element at @p from so that it ends up at row @p to, the others closing up behind
    //! it. Returns whether anything moved.
    Q_INVOKABLE bool moveElement(int from, int to);
    Q_INVOKABLE bool contains(const QString & type, const QString & sourceId) const;

    const ElementList & elements() const;

    //! Empties the panel, as for a new project. Not a user edit, so modified() is not emitted.
    void clear();

    //! Writes nothing for an empty panel, so a project that never used one is saved as before.
    void serializeToXml(ProjectWriter & writer) const;
    void deserializeFromXml(ProjectReader & reader);

signals:
    void countChanged();
    //! The user changed what the panel shows: the project has unsaved changes.
    void modified();

private:
    ElementList m_elements;
};

} // namespace noteahead

#endif // PERFORMANCE_PANEL_MODEL_HPP
