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

#ifndef PROPERTY_SERVICE_HPP
#define PROPERTY_SERVICE_HPP

#include <QObject>
#include <QVariantList>

namespace noteahead {

class PropertyService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList availableMidiControllers READ availableMidiControllers CONSTANT)

public:
    explicit PropertyService(QObject * parent = nullptr);

    QVariantList availableMidiControllers() const;

    Q_INVOKABLE int minValue(int controller) const;
    Q_INVOKABLE int maxValue(int controller) const;
};

} // namespace noteahead

#endif // PROPERTY_SERVICE_HPP
