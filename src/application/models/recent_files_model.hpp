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

#ifndef RECENT_FILES_MODEL_HPP
#define RECENT_FILES_MODEL_HPP

#include <QAbstractListModel>
#include <QObject>

namespace noteahead {

class RecentFilesModel : public QAbstractListModel
{
    Q_OBJECT
    //! Only the files whose path contains every word of it, in any order and case
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    enum class Role
    {
        FilePath = Qt::UserRole + 1,
        Exists
    };

    explicit RecentFilesModel(QObject * parent = nullptr);

    void setRecentFiles(const QStringList & list);

    QString filter() const;
    void setFilter(const QString & filter);

    //! The first listed file that exists, or empty: what Enter in the filter opens
    Q_INVOKABLE QString firstExistingFile() const;

    Q_INVOKABLE int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE QHash<int, QByteArray> roleNames() const override;

signals:
    void filterChanged();

private:
    void applyFilter();

    QStringList m_recentFiles;
    QString m_filter;
    //! What is listed: m_recentFiles narrowed down by m_filter
    QStringList m_listedFiles;
};

} // namespace noteahead

#endif // RECENT_FILES_MODEL_HPP
