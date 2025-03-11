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

#ifndef UTILS_HPP
#define UTILS_HPP

#include <QString>
#include <optional>
#include <string>

class QXmlStreamReader;

namespace noteahead::Utils {
double portNameMatchScore(const std::string & s1, const std::string & s2);
namespace Xml {
std::optional<bool> readBoolAttribute(QXmlStreamReader & reader, QString name, bool required = true);
std::optional<int> readIntAttribute(QXmlStreamReader & reader, QString name, bool required = true);
std::optional<size_t> readUIntAttribute(QXmlStreamReader & reader, QString name, bool required = true);
std::optional<QString> readStringAttribute(QXmlStreamReader & reader, QString name, bool required = true);

} // namespace Xml
} // namespace noteahead::Utils

#endif // UTILS_HPP
