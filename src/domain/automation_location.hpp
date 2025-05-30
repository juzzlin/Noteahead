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

#ifndef AUTOMATION_LOCATION_HPP
#define AUTOMATION_LOCATION_HPP

#include <cstddef>
#include <memory>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class AutomationLocation
{
public:
    AutomationLocation();
    AutomationLocation(size_t pattern, size_t track, size_t column);

    bool operator==(const AutomationLocation & other) const;
    bool operator!=(const AutomationLocation & other) const;

    void serializeToXml(QXmlStreamWriter & writer) const;
    using AutomationLocationU = std::unique_ptr<AutomationLocation>;
    static AutomationLocationU deserializeFromXml(QXmlStreamReader & reader);

    size_t pattern() const;
    void setPattern(size_t pattern);

    size_t track() const;
    void setTrack(size_t track);

    size_t column() const;
    void setColumn(size_t column);

private:
    size_t m_pattern = 0;
    size_t m_track = 0;
    size_t m_column = 0;
};

} // namespace noteahead

#endif // AUTOMATION_LOCATION_HPP
