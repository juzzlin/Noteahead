// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY;} without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

#include "constants.hpp"

namespace noteahead::Constants {

QString applicationName()
{
    return "Noteahead";
}

QString applicationVersion()
{
    return VERSION;
}

QString copyright()
{
    return "Copyright (c) 2020-2025 Jussi Lind <jussi.lind@iki.fi>";
}

QString license()
{
    return "The GNU General Public License v3.0";
}

QString fileFormatVersion()
{
    return "1.0";
}

QString fileFormatExtension()
{
    return ".nahd";
}

QString qSettingsCompanyName()
{
    return applicationName();
}

QString webSiteUrl()
{
    return "https://github.com/juzzlin/Noteahead";
}

QString qSettingSoftwareName()
{
    return applicationName();
}

QString xmlKeyFileFormatVersion()
{
    return "fileFormatVersion";
}

QString xmlKeyApplicationName()
{
    return "applicationName";
}

QString xmlKeyApplicationVersion()
{
    return "applicationVersion";
}

QString xmlKeyCreatedDate()
{
    return "createdDate";
}

QString xmlKeyBeatsPerMinute()
{
    return "beatsPerMinute";
}

QString xmlKeyBankEnabled()
{
    return "bankEnabled";
}

QString xmlKeyBankLsb()
{
    return "bankLsb";
}

QString xmlKeyBankMsb()
{
    return "bankMsb";
}

QString xmlKeyBankByteOrderSwapped()
{
    return "bankByteOrderSwapped";
}

QString xmlKeyChannel()
{
    return "channel";
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

QString xmlKeyController()
{
    return "controller";
}

QString xmlKeyCutoff()
{
    return "cutoff";
}

QString xmlKeyIndex()
{
    return "index";
}

QString xmlKeyInstrument()
{
    return "Instrument";
}

QString xmlKeyInstrumentSettings()
{
    return "InstrumentSettings";
}

QString xmlKeyPatternAttr()
{
    return "pattern";
}

QString xmlKeyPlayOrder()
{
    return "PlayOrder";
}

QString xmlKeyPosition()
{
    return "Position";
}

QString xmlKeyLength()
{
    return "length";
}

QString xmlKeyLine()
{
    return "Line";
}

QString xmlKeyLineEvent()
{
    return "LineEvent";
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

QString xmlKeyMidiCcSetting()
{
    return "MidiCcSetting";
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

QString xmlKeyPan()
{
    return "pan";
}

QString xmlKeyPatch()
{
    return "patch";
}

QString xmlKeyPattern()
{
    return "Pattern";
}

QString xmlKeyPatterns()
{
    return "Patterns";
}

QString xmlKeyPortName()
{
    return "portName";
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

QString xmlKeyValue()
{
    return "value";
}

QString xmlKeyVelocity()
{
    return "velocity";
}

QString xmlKeyVolume()
{
    return "volume";
}

QString xmlKeyProject()
{
    return "Project";
}

QString xmlKeySong()
{
    return "Song";
}

} // namespace noteahead::constants
