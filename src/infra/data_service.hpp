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

#ifndef DATA_SERVICE_HPP
#define DATA_SERVICE_HPP

#include <QString>
#include <QTemporaryDir>
#include <QXmlStreamWriter>
#include <map>
#include <memory>

namespace noteahead {

class DataService
{
public:
    DataService();
    ~DataService();

    DataService(const DataService &) = delete;
    DataService & operator=(const DataService &) = delete;

    void extractDataFromXml(const QString & xml);
    QString resolvePath(const QString & nahdPath) const;
    void serializeDataToXml(QXmlStreamWriter & writer, const std::map<QString, QString> & embedFiles) const;
    void clear();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    std::map<QString, QString> m_extractedFiles;
};

} // namespace noteahead

#endif // DATA_SERVICE_HPP
