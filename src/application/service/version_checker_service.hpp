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

#ifndef VERSION_CHECKER_SERVICE_HPP
#define VERSION_CHECKER_SERVICE_HPP

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace noteahead {

class VersionCheckerService : public QObject
{
    Q_OBJECT

public:
    explicit VersionCheckerService(QObject * parent = nullptr);

    Q_INVOKABLE void check();

    static bool isNewerVersion(const QString & currentVersion, const QString & latestVersion);

signals:
    void newVersionAvailable(QString version);

private slots:
    void onReplyFinished(QNetworkReply * reply);

private:
    QNetworkAccessManager m_networkAccessManager;
};

} // namespace noteahead

#endif // VERSION_CHECKER_SERVICE_HPP
