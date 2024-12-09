// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#ifndef APPLICATION_SERVICE_HPP
#define APPLICATION_SERVICE_HPP

#include <QObject>

namespace cacophony {

class ApplicationService : public QObject
{
    Q_OBJECT

public:
    ApplicationService();

    Q_INVOKABLE QString applicationName() const;

    Q_INVOKABLE QString applicationVersion() const;
};

} // namespace cacophony

#endif // APPLICATION_SERVICE_HPP
