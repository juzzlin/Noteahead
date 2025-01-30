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

#include "play_order.hpp"

#include "../common/constants.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"

#include <QXmlStreamWriter>

namespace noteahead {

static const auto TAG = "PlayOrder";

PlayOrder::PlayOrder()
{
    setPatternAtPosition(0, 0);
}

size_t PlayOrder::length() const
{
    return m_positionToPatternMap.rbegin()->first + 1;
}

PlayOrder::PatternList PlayOrder::flatten() const
{
    PlayOrder::PatternList patterns;
    for (auto && it : m_positionToPatternMap) {
        patterns.push_back(it.second);
    }
    return patterns;
}

void PlayOrder::setPatternAtPosition(size_t position, size_t pattern)
{
    juzzlin::L(TAG).info() << "Position " << position << " mapped to pattern " << pattern;

    m_positionToPatternMap[position] = pattern;
}

size_t PlayOrder::positionToPattern(size_t position) const
{
    if (const auto pattern = m_positionToPatternMap.find(position); pattern != m_positionToPatternMap.end()) {
        return pattern->second;
    } else {
        return 0;
    }
}

void PlayOrder::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyPlayOrder());
    for (auto && position : m_positionToPatternMap) {
        writer.writeStartElement(Constants::xmlKeyPosition());
        writer.writeAttribute(Constants::xmlKeyIndex(), std::to_string(position.first));
        writer.writeAttribute(Constants::xmlKeyPatternAttr(), std::to_string(position.second));
        writer.writeEndElement();
    }
    writer.writeEndElement();
}

} // namespace noteahead
