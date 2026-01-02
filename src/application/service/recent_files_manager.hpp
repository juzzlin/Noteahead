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

#ifndef RECENT_FILES_MANAGER_HPP
#define RECENT_FILES_MANAGER_HPP

#include <QList>
#include <QObject>
#include <QString>

#include <optional>

namespace noteahead {

class RecentFilesManager : public QObject
{
    Q_OBJECT

public:
    RecentFilesManager();

    virtual std::optional<QString> recentFile() const;
    virtual QStringList recentFiles() const;
    virtual bool hasRecentFiles() const;

    virtual QString selectedFile() const;

signals:
    void recentFilesChanged(const QStringList & recentFiles);

public slots:
    virtual void addRecentFile(QString filePath);
    virtual void setSelectedFile(QString filePath);

private:
    QStringList m_recentFiles;
    QString m_selectedFile;
};

} // namespace noteahead

#endif // RECENT_FILES_MANAGER_HPP
