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

#include <algorithm>
#include <array>
#include <charconv>
#include <string>

#include <QXmlStreamReader>

namespace noteahead::Utils {
namespace Midi {

uint8_t scaleVelocityByKey(uint8_t velocity, uint8_t note, int keyTrackPercentage, int keyTrackOffset)
{
    if (keyTrackPercentage <= 0) {
        return velocity;
    }
    const double effectiveNote = std::max(0.0, static_cast<double>(note) - static_cast<double>(keyTrackOffset));
    const double factor = 1.0 - (static_cast<double>(keyTrackPercentage) / 100.0) * (effectiveNote / 127.0);
    return static_cast<uint8_t>(std::clamp(static_cast<double>(velocity) * factor, 0.0, 127.0));
}

double portNameMatchScore(const std::string & s1, const std::string & s2)
{
    if (s1.empty() || s2.empty()) {
        return 0;
    } else if (s1 == s2) {
        return 1;
    } else {
        size_t count = 0;
        while (count < s1.size() && count < s2.size() && s1.at(count) == s2.at(count)) {
            count++;
        }
        return static_cast<double>(count) / static_cast<double>(std::max(s1.size(), s2.size()));
    }
}
} // namespace Midi
namespace Misc {
void ensureFileExists(const std::filesystem::path & filePath)
{
    if (!filePath.empty() && !std::filesystem::exists(filePath)) {
        throw std::runtime_error("File does not exist: " + filePath.string());
    }
}
std::optional<double> parseDecimal(std::string_view str)
{
    double value;
    if (const auto [ptr, ec] = std::from_chars(str.begin(), str.end(), value); ec == std::errc()) {
        return value;
    } else {
        return std::nullopt;
    }
}
QStringList stdStringVectorToQStringList(const std::vector<std::string> & stringVector)
{
    QStringList stringList;
    std::ranges::transform(stringVector, std::back_inserter(stringList),
                           [](const auto & string) { return QString::fromStdString(string); });
    return stringList;
}
} // namespace Misc
namespace Xml {
std::optional<bool> readBoolAttribute(QXmlStreamReader & reader, QString name, bool required)
{
    if (!reader.attributes().hasAttribute(name)) {
        if (required) {
            throw std::runtime_error { "Attribute '" + name.toStdString() + "' not found!" };
        }
        return {};
    } else {
        return reader.attributes().value(name).toString() == Constants::NahdXml::xmlValueTrue();
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

std::optional<std::chrono::milliseconds> readMSecAttribute(QXmlStreamReader & reader, QString name, bool required)
{
    if (!reader.attributes().hasAttribute(name)) {
        if (required) {
            throw std::runtime_error { "Attribute '" + name.toStdString() + "' not found!" };
        }
        return {};
    } else {
        return static_cast<std::chrono::milliseconds>(reader.attributes().value(name).toInt());
    }
}

} // namespace Xml
} // namespace noteahead::Utils
