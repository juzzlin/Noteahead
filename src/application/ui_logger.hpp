// This file is part of Cacophony.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef UI_LOGGER_HPP
#define UI_LOGGER_HPP

#include <QObject>

namespace cacophony {

class UiLogger : public QObject
{
    Q_OBJECT

public:
    explicit UiLogger(QObject * parent = nullptr);

    Q_INVOKABLE void debug(QString tag, QString message) const;

    Q_INVOKABLE void info(QString tag, QString message) const;

    Q_INVOKABLE void warning(QString tag, QString message) const;

    Q_INVOKABLE void error(QString tag, QString message) const;
};

} // namespace cacophony

#endif // UI_LOGGER_HPP
