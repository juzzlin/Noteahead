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

#include "property_service.hpp"

#include "../../infra/midi/midi_cc_mapping.hpp"

#include <QVariantMap>

namespace noteahead {

PropertyService::PropertyService(QObject * parent)
  : QObject { parent }
{
}

QVariantList PropertyService::availableMidiControllers() const
{
    QVariantList list;
    for (uint8_t i { 0 }; i < 128; ++i) {
        QString name { MidiCcMapping::controllerToString(static_cast<MidiCcMapping::Controller>(i)) };
        if (name == "Undefined") {
            name = QString { "%1" }.arg(i);
        } else {
            name = QString { "%1: %2" }.arg(i).arg(name);
        }
        list.append(QVariantMap {
            { "number", i },
            { "name", name },
            { "minValue", 0 },
            { "maxValue", 127 }
        });
    }
    return list;
}

int PropertyService::minValue(int controller) const
{
    Q_UNUSED(controller)

    return 0;
}

int PropertyService::maxValue(int controller) const
{
    Q_UNUSED(controller)

    return 127;
}

} // namespace noteahead
