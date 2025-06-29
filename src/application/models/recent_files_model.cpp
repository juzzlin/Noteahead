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

#include "recent_files_model.hpp"

#include <QFileInfo>

namespace noteahead {

RecentFilesModel::RecentFilesModel(QObject * parent)
  : QAbstractListModel { parent }
{
}

void RecentFilesModel::setRecentFiles(const QStringList & list)
{
    beginResetModel();

    m_recentFiles = list;

    endResetModel();
}

int RecentFilesModel::rowCount(const QModelIndex & parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_recentFiles.size());
}

QVariant RecentFilesModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_recentFiles.size())
        return {};

    const auto path = m_recentFiles.at(index.row());

    switch (static_cast<Role>(role)) {
    case Role::FilePath:
        return path;
    case Role::Exists:
        return QFileInfo::exists(path);
    }
}

QHash<int, QByteArray> RecentFilesModel::roleNames() const
{
    return {
        { static_cast<int>(Role::FilePath), "filePath" },
        { static_cast<int>(Role::Exists), "exists" }
    };
}

} // namespace noteahead
