// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY;} without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#include "constants.hpp"

namespace cacophony::Constants {

QString applicationName()
{
    return "Cacophony";
}

QString applicationVersion()
{
    return VERSION;
}

QString copyright()
{
    return "Copyright (c) 2020-2025 Jussi Lind";
}

QString fileFormatVersion()
{
    return "1.0";
}

QString fileFormatExtension()
{
    return ".caco";
}

QString qSettingsCompanyName()
{
    return applicationName();
}

QString webSiteUrl()
{
    return "https://github.com/juzzlin/Cacophony";
}

QString qSettingSoftwareName()
{
    return applicationName();
}

QString xmlKeyBeatsPerMinute()
{
    return "beatsPerMinute";
}

QString xmlKeyColumn()
{
    return "Column";
}

QString xmlKeyColumns()
{
    return "Columns";
}

QString xmlKeyColumnCount()
{
    return "columnCount";
}

QString xmlKeyIndex()
{
    return "index";
}

QString xmlKeyLine()
{
    return "Line";
}

QString xmlKeyLines()
{
    return "Lines";
}

QString xmlKeyLineCount()
{
    return "lineCount";
}

QString xmlKeyLinesPerBeat()
{
    return "linesPerBeat";
}

QString xmlKeyName()
{
    return "name";
}

QString xmlKeyNote()
{
    return "note";
}

QString xmlKeyNoteOn()
{
    return "noteOn";
}

QString xmlKeyNoteOff()
{
    return "noteOff";
}

QString xmlKeyNoteData()
{
    return "NoteData";
}

QString xmlKeyPattern()
{
    return "Pattern";
}

QString xmlKeyPatterns()
{
    return "Patterns";
}

QString xmlKeyTrack()
{
    return "Track";
}

QString xmlKeyTracks()
{
    return "Tracks";
}

QString xmlKeyTrackCount()
{
    return "trackCount";
}

QString xmlKeyType()
{
    return "type";
}

QString xmlKeyVelocity()
{
    return "velocity";
}

QString xmlKeyProject()
{
    return "Project";
}

QString xmlKeySong()
{
    return "Song";
}

} // namespace cacophony::constants
