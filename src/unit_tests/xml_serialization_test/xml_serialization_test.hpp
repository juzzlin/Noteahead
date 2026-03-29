// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef XML_SERIALIZATION_TEST_HPP
#define XML_SERIALIZATION_TEST_HPP

#include <QObject>

namespace noteahead {

class XmlSerializationTest : public QObject
{
    Q_OBJECT

private slots:

    void test_toXmlFromXml_addTrack_shouldLoadSong();

    void test_toXmlFromXml_columnName_shouldLoadColumnName();
    void test_toXmlFromXml_columnSettings_shouldSaveAndLoad();

    void test_toXmlFromXml_instrumentSettings_shouldParseInstrumentSettings();
    void test_toXmlFromXml_instrument_shouldParseInstrument();

    void test_toXmlFromXml_sideChainService_shouldLoadSideChainService();

    void test_toXmlFromXml_automationService_midiCc_noModulation_shouldLoadAutomationService();
    void test_toXmlFromXml_automationService_midiCc_shouldLoadAutomationService();
    void test_toXmlFromXml_automationService_midiCc_withModulation_shouldLoadAutomationService();
    void test_toXmlFromXml_automationService_pitchBend_shouldLoadAutomationService();
    void test_toXmlFromXml_automationService_pitchBend_withModulation_shouldLoadAutomationService();

    void test_toXmlFromXml_mixerService_shouldLoadMixerService();

    void test_toXmlFromXml_noteData_noteOff();
    void test_toXmlFromXml_noteData_noteOn();
    void test_toXmlFromXml_noteData_delay_shouldSaveAndLoadDelay();

    void test_toXmlFromXml_playOrder();
    void test_toXmlFromXml_removeTrack_shouldLoadSong();
    void test_toXmlFromXml_songProperties();
    void test_toXmlFromXml_trackName_shouldLoadTrackName();
    void test_toXmlFromXml_trackDrumTrack_shouldLoadTrackDrumTrack();

    void test_toXmlFromXml_differentSongs_shouldLoadSongs();
    void test_toXmlFromXml_template_shouldLoadTemplate();
};

} // namespace noteahead

#endif // XML_SERIALIZATION_TEST_HPP
