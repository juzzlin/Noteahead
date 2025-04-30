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

#ifndef PITCH_BEND_AUTOMATION_HPP
#define PITCH_BEND_AUTOMATION_HPP

#include "automation.hpp"

#include <memory>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class PitchBendAutomation : public Automation
{
public:
    struct InterpolationParameters
    {
        size_t line0 = 0;
        size_t line1 = 0;
        int value0 = 0; // -100%..+100%
        int value1 = 0; // -100%..+100%

        bool operator==(const InterpolationParameters & other) const
        {
            return line0 == other.line0 && line1 == other.line1 && value0 == other.value0 && value1 == other.value1;
        }

        bool operator!=(const InterpolationParameters & other) const
        {
            return !(*this == other);
        }
    };

    PitchBendAutomation(size_t id, AutomationLocation location, InterpolationParameters interpolation, QString comment, bool enabled);
    PitchBendAutomation(size_t id, AutomationLocation location, InterpolationParameters interpolation, QString comment);
    PitchBendAutomation();

    bool operator==(const PitchBendAutomation & other) const;
    bool operator!=(const PitchBendAutomation & other) const;
    bool operator<(const PitchBendAutomation & other) const;

    const InterpolationParameters & interpolation() const;
    void setInterpolation(const InterpolationParameters & parameters);

    void serializeToXml(QXmlStreamWriter & writer) const;
    using PitchBendAutomationU = std::unique_ptr<PitchBendAutomation>;
    static PitchBendAutomationU deserializeFromXml(QXmlStreamReader & reader);

    QString toString() const;

private:

    InterpolationParameters m_interpolation;
};

} // namespace noteahead

#endif // PITCH_BEND_AUTOMATION_HPP
