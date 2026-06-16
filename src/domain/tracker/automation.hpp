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

#ifndef AUTOMATION_HPP
#define AUTOMATION_HPP

#include "automation_location.hpp"

#include <QString>

namespace noteahead {

//! Base class for automations.
class Automation
{
public:
    Automation();
    Automation(size_t id, AutomationLocation location, QString comment, bool enabled);

    size_t id() const;
    void setId(size_t id);

    QString comment() const;
    void setComment(QString comment);

    bool enabled() const;
    void setEnabled(bool enabled);

    const AutomationLocation & location() const;
    void setLocation(const AutomationLocation & location);

private:
    size_t m_id = 0;

    QString m_comment;

    bool m_enabled = true;

    AutomationLocation m_location;
};

} // namespace noteahead

#endif // AUTOMATION_HPP
