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

#include "utils.hpp"

#include "constants.hpp"

#include <QXmlStreamReader>

namespace noteahead::Utils {
double portNameMatchScore(const std::string & s1, const std::string & s2)
{
    if (s1.empty() || s2.empty()) {
        return 0;
    }

    if (s1 == s2) {
        return 1;
    }

    size_t count = 0;
    while (count < s1.size() && count < s2.size() && s1.at(count) == s2.at(count)) {
        count++;
    }

    return static_cast<double>(count) / static_cast<double>(std::max(s1.size(), s2.size()));
}
namespace Xml {
std::optional<bool> readBoolAttribute(QXmlStreamReader & reader, QString name, bool required)
{
    if (!reader.attributes().hasAttribute(name)) {
        if (required) {
            throw std::runtime_error { "Attribute '" + name.toStdString() + "' not found!" };
        }
        return {};
    } else {
        return reader.attributes().value(name).toString() == Constants::xmlValueTrue();
    }
}

std::optional<int> readIntAttribute(QXmlStreamReader & reader, QString name, bool required)
{
    if (!reader.attributes().hasAttribute(name)) {
        if (required) {
            throw std::runtime_error { "Attribute '" + name.toStdString() + "' not found!" };
        }
        return {};
    } else {
        return reader.attributes().value(name).toInt();
    }
}

std::optional<size_t> readUIntAttribute(QXmlStreamReader & reader, QString name, bool required)
{
    if (!reader.attributes().hasAttribute(name)) {
        if (required) {
            throw std::runtime_error { "Attribute '" + name.toStdString() + "' not found!" };
        }
        return {};
    } else {
        return reader.attributes().value(name).toUInt();
    }
}

std::optional<QString> readStringAttribute(QXmlStreamReader & reader, QString name, bool required)
{
    if (!reader.attributes().hasAttribute(name)) {
        if (required) {

            throw std::runtime_error { "Attribute '" + name.toStdString() + "' not found!" };
        }
        return {};
    } else {
        return reader.attributes().value(name).toString();
    }
}
} // namespace Xml
} // namespace noteahead::Utils
