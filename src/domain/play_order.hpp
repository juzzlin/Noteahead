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

#ifndef PLAY_ORDER_HPP
#define PLAY_ORDER_HPP

#include <cstddef>
#include <vector>

class QXmlStreamWriter;

namespace noteahead {

class PlayOrder
{
public:
    PlayOrder();

    size_t length() const;

    using PatternList = std::vector<size_t>;
    PatternList flatten(size_t songLength, size_t startPosition = 0) const;

    void insertPattern(size_t position, size_t pattern);
    void removePattern(size_t position);

    void setPatternAtPosition(size_t position, size_t pattern);
    size_t positionToPattern(size_t position) const;

    void serializeToXml(QXmlStreamWriter & writer) const;
    void serializeToXml(QXmlStreamWriter & writer, size_t lastPosition) const;

private:
    void serializePosition(QXmlStreamWriter & writer, size_t position) const;

    std::vector<size_t> m_positionToPattern;
};

} // namespace noteahead

#endif // PLAY_ORDER_HPP
