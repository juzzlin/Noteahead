// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <QString>

namespace cacophony::Constants {

QString applicationName();

QString applicationVersion();

QString copyright();

QString license();

QString fileFormatVersion();

QString fileFormatExtension();

QString qSettingsCompanyName();

QString webSiteUrl();

QString qSettingSoftwareName();

QString xmlKeyBankEnabled();

QString xmlKeyBankLsb();

QString xmlKeyBankMsb();

QString xmlKeyBankByteOrderSwapped();

QString xmlKeyBeatsPerMinute();

QString xmlKeyChannel();

QString xmlKeyColumn();

QString xmlKeyColumns();

QString xmlKeyColumnCount();

QString xmlKeyIndex();

QString xmlKeyInstrument();

QString xmlKeyLine();

QString xmlKeyLines();

QString xmlKeyLineCount();

QString xmlKeyLinesPerBeat();

QString xmlKeyName();

QString xmlKeyNote();

QString xmlKeyNoteOn();

QString xmlKeyNoteOff();

QString xmlKeyNoteData();

QString xmlKeyPatchEnabled();

QString xmlKeyPatch();

QString xmlKeyPattern();

QString xmlKeyPatterns();

QString xmlKeyPortName();

QString xmlKeyTrack();

QString xmlKeyTracks();

QString xmlKeyTrackCount();

QString xmlKeyType();

QString xmlKeyVelocity();

QString xmlKeyProject();

QString xmlKeySong();

} // namespace cacophony::Constants

#endif // CONSTANTS_HPP
