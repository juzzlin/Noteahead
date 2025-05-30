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

#include "automation.hpp"

#include "../common/constants.hpp"

#include <QXmlStreamWriter>

namespace noteahead {

Automation::Automation() = default;

Automation::Automation(size_t id, AutomationLocation location, QString comment, bool enabled)
  : m_id { id }
  , m_comment { comment }
  , m_enabled { enabled }
  , m_location { location }
{
}

size_t Automation::id() const
{
    return m_id;
}

void Automation::setId(size_t id)
{
    m_id = id;
}

QString Automation::comment() const
{
    return m_comment;
}

void Automation::setComment(QString comment)
{
    m_comment = comment;
}

bool Automation::enabled() const
{
    return m_enabled;
}

void Automation::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

const AutomationLocation & Automation::location() const
{
    return m_location;
}

void Automation::setLocation(const AutomationLocation & location)
{
    m_location = location;
}

} // namespace noteahead
