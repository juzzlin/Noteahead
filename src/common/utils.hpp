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

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <QString>
#include <QStringList>

namespace noteahead {
class ProjectReader;
}

namespace noteahead::Utils {
namespace Misc {
void ensureFileExists(const std::filesystem::path & filePath);
QStringList stdStringVectorToQStringList(const std::vector<std::string> & stringVector);
std::optional<double> parseDecimal(std::string_view string);
} // namespace Misc

namespace Midi {
uint8_t scaleVelocityByKey(uint8_t velocity, uint8_t note, int keyTrackPercentage, int keyTrackOffset = 0);
double portNameMatchScore(const std::string & s1, const std::string & s2);
} // namespace Midi

namespace Xml {
std::optional<bool> readBoolAttribute(ProjectReader & reader, QString name, bool required = true);
std::optional<int> readIntAttribute(ProjectReader & reader, QString name, bool required = true);
std::optional<double> readDoubleAttribute(ProjectReader & reader, QString name, bool required = true);
std::optional<size_t> readUIntAttribute(ProjectReader & reader, QString name, bool required = true);
std::optional<QString> readStringAttribute(ProjectReader & reader, QString name, bool required = true);
std::optional<std::chrono::milliseconds> readMSecAttribute(ProjectReader & reader, QString name, bool required = true);
} // namespace Xml

namespace Dsp {
float cutoffToHz(float cutoff, float sampleRate);
float dbToLinear(float db);
float linearToDb(float linear);
} // namespace Dsp
} // namespace noteahead::Utils

#endif // UTILS_HPP
