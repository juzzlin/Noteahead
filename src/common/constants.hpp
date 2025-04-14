// This file is part of Noteahead.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <QString>

namespace noteahead::Constants {

QString applicationName();
QString applicationVersion();

QString copyright();

QString license();

QString fileFormatVersion();
QString fileFormatExtension();

QString qSettingsCompanyName();
QString qSettingSoftwareName();
QString webSiteUrl();

QString xmlKeyFileFormatVersion();

QString xmlKeyApplicationName();
QString xmlKeyApplicationVersion();

QString xmlKeyCreatedDate();

QString xmlKeyBankEnabled();
QString xmlKeyBankLsb();
QString xmlKeyBankMsb();
QString xmlKeyBankByteOrderSwapped();

QString xmlKeyBeatsPerMinute();

QString xmlKeyChannel();

QString xmlKeyColumn();
QString xmlKeyColumns();
QString xmlKeyColumnCount();

QString xmlKeyController();
QString xmlKeyEnabled();

QString xmlKeyCutoff();

QString xmlKeyDelay();
QString xmlKeyTranspose();

QString xmlKeyIndex();

QString xmlKeyInstrument();
QString xmlKeyInstrumentSettings();

QString xmlKeySendMidiClock();

QString xmlKeyPlayOrder();
QString xmlKeyPatternAttr();
QString xmlKeyPosition();
QString xmlKeyLength();

QString xmlKeyLine();
QString xmlKeyLineEvent();
QString xmlKeyLines();
QString xmlKeyLineCount();
QString xmlKeyLinesPerBeat();

QString xmlKeyMidiCcSetting();

QString xmlKeyMixer();
QString xmlKeyColumnAttr();
QString xmlKeyColumnMuted();
QString xmlKeyColumnSoloed();
QString xmlKeyColumnVelocityScale();
QString xmlKeyTrackAttr();
QString xmlKeyTrackMuted();
QString xmlKeyTrackSoloed();
QString xmlKeyTrackVelocityScale();

QString xmlKeyName();

QString xmlKeyNote();
QString xmlKeyNoteOn();
QString xmlKeyNoteOff();
QString xmlKeyNoteData();

QString xmlKeyPan();

QString xmlKeyPatch();

QString xmlKeyPattern();
QString xmlKeyPatterns();

QString xmlKeyPortName();

QString xmlKeyTrack();
QString xmlKeyTracks();
QString xmlKeyTrackCount();

QString xmlKeyType();
QString xmlKeyValue();

QString xmlKeyVelocity();
QString xmlKeyVolume();

QString xmlKeyProject();
QString xmlKeySong();

QString xmlValueFalse();
QString xmlValueTrue();

} // namespace noteahead::Constants

#endif // CONSTANTS_HPP
