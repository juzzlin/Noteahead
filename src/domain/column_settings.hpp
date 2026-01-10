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

#ifndef COLUMN_SETTINGS_HPP
#define COLUMN_SETTINGS_HPP

#include <cstdint>
#include <chrono>
#include <memory>

#include <QString>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class ColumnSettings
{
public:
    ColumnSettings();

    std::chrono::milliseconds delay { 0 };

    struct ChordAutomationSettings
    {
        struct ChordNote
        {
            //! Offset (semi-tones relative to the root note).
            int8_t offset = 0;
            //! Percentage of root note's velocity.
            uint8_t velocity = 100;
            //! Delay in milliseconds.
            int16_t delay = 0;
        };

        ChordNote note1;
        ChordNote note2;
        ChordNote note3;

        bool isEnabled() const
        {
            return note1.offset || note2.offset || note3.offset;
        }
    };

    ChordAutomationSettings chordAutomationSettings;

    void serializeToXml(QXmlStreamWriter & writer) const;
    using ColumnSettingsU = std::unique_ptr<ColumnSettings>;
    static ColumnSettingsU deserializeFromXml(QXmlStreamReader & reader);

    QString toString() const;
};

} // namespace noteahead

#endif // COLUMN_SETTINGS_HPP
