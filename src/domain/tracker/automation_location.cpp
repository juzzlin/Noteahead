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

#include "domain/tracker/automation_location.hpp"

#include "common/constants.hpp"
#include "common/xml/project_reader.hpp"
#include "common/xml/project_writer.hpp"

#include <QVariant>

namespace noteahead {

AutomationLocation::AutomationLocation() = default;

AutomationLocation::AutomationLocation(size_t pattern, size_t track, size_t column)
  : m_pattern { pattern }
  , m_track { track }
  , m_column { column }
{
}

bool AutomationLocation::operator==(const AutomationLocation & other) const
{
    return m_pattern == other.m_pattern && m_track == other.m_track && m_column == other.m_column;
}

bool AutomationLocation::operator!=(const AutomationLocation & other) const
{
    return !(*this == other);
}

void AutomationLocation::serializeToXml(ProjectWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyLocation());
    writer.writeAttribute(Constants::NahdXml::xmlKeyPatternAttr(), QString::number(m_pattern));
    writer.writeAttribute(Constants::NahdXml::xmlKeyTrackAttr(), QString::number(m_track));
    writer.writeAttribute(Constants::NahdXml::xmlKeyColumnAttr(), QString::number(m_column));
    writer.writeEndElement();
}

AutomationLocation::AutomationLocationU AutomationLocation::deserializeFromXml(ProjectReader & reader)
{
    const auto pattern = reader.attribute(Constants::NahdXml::xmlKeyPatternAttr()).toULongLong();
    const auto track = reader.attribute(Constants::NahdXml::xmlKeyTrackAttr()).toULongLong();
    const auto column = reader.attribute(Constants::NahdXml::xmlKeyColumnAttr()).toULongLong();
    return std::make_unique<AutomationLocation>(pattern, track, column);
}

size_t AutomationLocation::pattern() const
{
    return m_pattern;
}

void AutomationLocation::setPattern(size_t pattern)
{
    m_pattern = pattern;
}

size_t AutomationLocation::track() const
{
    return m_track;
}

void AutomationLocation::setTrack(size_t track)
{
    m_track = track;
}

size_t AutomationLocation::column() const
{
    return m_column;
}

void AutomationLocation::setColumn(size_t column)
{
    m_column = column;
}

} // namespace noteahead
