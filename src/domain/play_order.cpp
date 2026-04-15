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
#include <ranges>

#include <QXmlStreamWriter>

namespace noteahead {

static const auto TAG = "PlayOrder";

PlayOrder::PlayOrder(size_t defaultPatternIndex)
  : m_defaultPatternIndex { defaultPatternIndex }
{
    setPatternAtPosition(0, m_defaultPatternIndex);
}

size_t PlayOrder::length() const
{
    return m_playOrder.size();
}

PlayOrder::PatternList PlayOrder::getPatterns(size_t songLength, size_t startPosition) const
{
    PatternList patternList;

    // Ensure startPosition is within bounds
    if (startPosition >= m_playOrder.size()) {
        return patternList;
    }

    // Use views to get the subrange of interest
    const auto selectedPatterns = m_playOrder
      | std::views::drop(startPosition)
      | std::views::take(songLength);

    // Copy selected pattern indices into patternList
    for (auto && item : selectedPatterns) {
        patternList.push_back(item.first);
    }

    // Extend if needed
    patternList.resize(songLength, m_defaultPatternIndex);

    return patternList;
}

void PlayOrder::insertPattern(size_t position, size_t pattern)
{
    juzzlin::L(TAG).info() << "Inserting pattern " << pattern << " at position " << position;

    while (m_playOrder.size() < position) {
        m_playOrder.push_back({ m_defaultPatternIndex, false });
    }

    m_playOrder.insert(m_playOrder.begin() + static_cast<int>(position), { pattern, false });
}

void PlayOrder::removePattern(size_t position)
{
    juzzlin::L(TAG).info() << "Removing pattern at position " << position;

    if (position < m_playOrder.size()) {
        m_playOrder.erase(m_playOrder.begin() + static_cast<int>(position));
    }
}

bool PlayOrder::hasPattern(size_t pattern) const
{
    return std::ranges::any_of(m_playOrder, [pattern](const auto & item) {
        return item.first == pattern;
    });
}

void PlayOrder::setLength(size_t length, size_t defaultPattern)
{
    juzzlin::L(TAG).info() << "Setting length to " << length;

    if (length > m_playOrder.size()) {
        m_playOrder.resize(length, { defaultPattern, false });
    } else {
        m_playOrder.resize(length);
    }
}

void PlayOrder::setPatternAtPosition(size_t position, size_t pattern)
{
    juzzlin::L(TAG).info() << "Position " << position << " mapped to pattern " << pattern;

    while (m_playOrder.size() <= position) {
        m_playOrder.push_back({ m_defaultPatternIndex, false });
    }

    m_playOrder[position].first = pattern;
}

size_t PlayOrder::positionToPattern(size_t position) const
{
    return position < m_playOrder.size() ? m_playOrder.at(position).first : m_defaultPatternIndex;
}

bool PlayOrder::isSkipped(size_t position) const
{
    return position < m_playOrder.size() ? m_playOrder.at(position).second : false;
}

void PlayOrder::setSkipped(size_t position, bool skipped)
{
    juzzlin::L(TAG).info() << "Position " << position << " skipped state: " << skipped;

    while (m_playOrder.size() <= position) {
        m_playOrder.push_back({ m_defaultPatternIndex, false });
    }

    m_playOrder[position].second = skipped;
}

void PlayOrder::removeMissingPatterns(const PatternSet & validPatterns)
{
    auto it = std::remove_if(m_playOrder.begin(), m_playOrder.end(), [&validPatterns](const auto & item) {
        if (!validPatterns.contains(item.first)) {
            juzzlin::L(TAG).warning() << "Removing missing pattern " << item.first << " from play order";
            return true;
        }
        return false;
    });
    m_playOrder.erase(it, m_playOrder.end());
}

void PlayOrder::serializePosition(QXmlStreamWriter & writer, size_t position) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyPosition());
    writer.writeAttribute(Constants::NahdXml::xmlKeyIndex(), QString::number(position));
    writer.writeAttribute(Constants::NahdXml::xmlKeyPatternAttr(), QString::number(m_playOrder.at(position).first));
    if (m_playOrder.at(position).second) {
        writer.writeAttribute(Constants::NahdXml::xmlKeySkipped(), Constants::NahdXml::xmlValueTrue());
    }
    writer.writeEndElement();
}

void PlayOrder::serializeToXml(QXmlStreamWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyPlayOrder());
    for (size_t i = 0; i < m_playOrder.size(); i++) {
        serializePosition(writer, i);
    }
    writer.writeEndElement();
}

void PlayOrder::serializeToXml(QXmlStreamWriter & writer, size_t lastPosition) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyPlayOrder());
    for (size_t i = 0; i <= lastPosition && i < m_playOrder.size(); i++) {
        serializePosition(writer, i);
    }
    writer.writeEndElement();
}

} // namespace noteahead
