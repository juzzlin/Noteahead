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

#include <algorithm>

#include <QXmlStreamWriter>

namespace noteahead {

static const auto TAG = "PlayOrder";

PlayOrder::PlayOrder()
{
    setPatternAtPosition(0, 0);
}

size_t PlayOrder::length() const
{
    return m_positionToPattern.size();
}

PlayOrder::PatternList PlayOrder::flatten(size_t songLength) const
{
    auto patternList = m_positionToPattern;
    songLength = std::max(static_cast<size_t>(1), songLength);
    if (songLength > patternList.size()) {
        while (songLength > patternList.size()) {
            patternList.push_back(0);
        }
    } else if (songLength < patternList.size()) {
        while (songLength < patternList.size()) {
            patternList.pop_back();
        }
    }
    return patternList;
}

void PlayOrder::insertPattern(size_t position, size_t pattern)
{
    juzzlin::L(TAG).info() << "Inserting pattern " << pattern << " at position " << position;

    while (m_positionToPattern.size() <= position) {
        m_positionToPattern.push_back(0);
    }

    m_positionToPattern.insert(m_positionToPattern.begin() + position, pattern);
}

void PlayOrder::removePattern(size_t position)
{
    juzzlin::L(TAG).info() << "Removing pattern at position " << position;

    if (position < m_positionToPattern.size()) {
        m_positionToPattern.erase(m_positionToPattern.begin() + position);
    }
}

void PlayOrder::setPatternAtPosition(size_t position, size_t pattern)
{
    juzzlin::L(TAG).info() << "Position " << position << " mapped to pattern " << pattern;

    while (m_positionToPattern.size() <= position) {
        m_positionToPattern.push_back(0);
    }

    m_positionToPattern[position] = pattern;
}

size_t PlayOrder::positionToPattern(size_t position) const
{
    return position < m_positionToPattern.size() ? m_positionToPattern.at(position) : 0;
}

void PlayOrder::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::xmlKeyPlayOrder());
    for (size_t i = 0; i < m_positionToPattern.size(); i++) {
        writer.writeStartElement(Constants::xmlKeyPosition());
        writer.writeAttribute(Constants::xmlKeyIndex(), std::to_string(i));
        writer.writeAttribute(Constants::xmlKeyPatternAttr(), std::to_string(m_positionToPattern.at(i)));
        writer.writeEndElement();
    }
    writer.writeEndElement();
}

} // namespace noteahead
