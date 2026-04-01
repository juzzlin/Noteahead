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

#include "xml_serialization_test.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../domain/column_settings.hpp"
#include "../../domain/instrument.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/song.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

void XmlSerializationTest::test_toXmlFromXml_playOrder()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceOut.setPatternAtSongPosition(1, 11);
    editorServiceOut.setPatternAtSongPosition(2, 22);
    editorServiceOut.setPatternAtSongPosition(3, 33);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 0);
    QCOMPARE(editorServiceIn.patternAtSongPosition(1), 11);
    QCOMPARE(editorServiceIn.patternAtSongPosition(2), 22);
    QCOMPARE(editorServiceIn.patternAtSongPosition(3), 33);
}

void XmlSerializationTest::test_toXmlFromXml_songProperties()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceOut.setBeatsPerMinute(666);
    editorServiceOut.setLinesPerBeat(42);
    editorServiceOut.setPatternName(0, "patternName");
    editorServiceOut.setSongLength(16);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    QSignalSpy songChangedSpy { &editorServiceIn, &EditorService::songChanged };
    QSignalSpy positionChangedSpy { &editorServiceIn, &EditorService::positionChanged };
    QSignalSpy beatsPerMinuteChangedSpy { &editorServiceIn, &EditorService::beatsPerMinuteChanged };
    QSignalSpy linesPerBeatChangedSpy { &editorServiceIn, &EditorService::linesPerBeatChanged };
    editorServiceIn.fromXml(xml);

    QCOMPARE(songChangedSpy.count(), 1);
    QCOMPARE(positionChangedSpy.count(), 2);
    QCOMPARE(beatsPerMinuteChangedSpy.count(), 1);
    QCOMPARE(linesPerBeatChangedSpy.count(), 1);
    QCOMPARE(editorServiceIn.beatsPerMinute(), editorServiceOut.beatsPerMinute());
    QCOMPARE(editorServiceIn.linesPerBeat(), editorServiceOut.linesPerBeat());
    QCOMPARE(editorServiceIn.patternName(0), editorServiceOut.patternName(0));
    QCOMPARE(editorServiceIn.songLength(), editorServiceOut.songLength());
}

void XmlSerializationTest::test_toXmlFromXml_columnName_shouldLoadColumnName()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceOut.setColumnName(0, 0, "columnName0_0");
    editorServiceOut.setColumnName(1, 0, "columnName1_0");

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.columnName(0, 0), editorServiceOut.columnName(0, 0));
    QCOMPARE(editorServiceIn.columnName(1, 0), editorServiceOut.columnName(1, 0));
}

void XmlSerializationTest::test_toXmlFromXml_trackName_shouldLoadTrackName()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceOut.setTrackName(0, "trackName0");
    editorServiceOut.setTrackName(1, "trackName1");

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.trackName(0), editorServiceOut.trackName(0));
    QCOMPARE(editorServiceIn.trackName(1), editorServiceOut.trackName(1));
}

void XmlSerializationTest::test_toXmlFromXml_columnSettings_shouldSaveAndLoad()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    auto settingsOut = std::make_shared<ColumnSettings>();
    settingsOut->chordAutomationSettings.note1.offset = 4;
    settingsOut->chordAutomationSettings.note1.velocity = 80;
    settingsOut->chordAutomationSettings.note2.offset = 7;
    settingsOut->chordAutomationSettings.note2.velocity = 60;
    settingsOut->chordAutomationSettings.note3.offset = 12;
    settingsOut->chordAutomationSettings.note3.velocity = 90;
    settingsOut->chordAutomationSettings.arpeggiator.enabled = true;
    settingsOut->chordAutomationSettings.arpeggiator.pattern = Arpeggiator::Pattern::Down;
    settingsOut->chordAutomationSettings.arpeggiator.eventsPerBeat = 8;
    editorServiceOut.setColumnSettings(1, 0, settingsOut);

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);

    const auto settingsIn = editorServiceIn.columnSettings(1, 0);

    QVERIFY(settingsIn);
    QCOMPARE(settingsIn->chordAutomationSettings.note1.offset, settingsOut->chordAutomationSettings.note1.offset);
    QCOMPARE(settingsIn->chordAutomationSettings.note1.velocity, settingsOut->chordAutomationSettings.note1.velocity);
    QCOMPARE(settingsIn->chordAutomationSettings.note2.offset, settingsOut->chordAutomationSettings.note2.offset);
    QCOMPARE(settingsIn->chordAutomationSettings.note2.velocity, settingsOut->chordAutomationSettings.note2.velocity);
    QCOMPARE(settingsIn->chordAutomationSettings.note3.offset, settingsOut->chordAutomationSettings.note3.offset);
    QCOMPARE(settingsIn->chordAutomationSettings.note3.velocity, settingsOut->chordAutomationSettings.note3.velocity);
    QCOMPARE(settingsIn->chordAutomationSettings.arpeggiator.enabled, settingsOut->chordAutomationSettings.arpeggiator.enabled);
    QCOMPARE(settingsIn->chordAutomationSettings.arpeggiator.pattern, settingsOut->chordAutomationSettings.arpeggiator.pattern);
    QCOMPARE(settingsIn->chordAutomationSettings.arpeggiator.eventsPerBeat, settingsOut->chordAutomationSettings.arpeggiator.eventsPerBeat);
}

void XmlSerializationTest::test_toXmlFromXml_automationService_midiCc_shouldLoadAutomationService()
{
    quint8 controller = 64;
    quint8 line0 = 4;
    quint8 line1 = 12;
    quint8 value0 = 0;
    quint8 value1 = 100;
    const auto comment = "MIDI CC Automation Test";

    AutomationService automationServiceOut { std::make_shared<PropertyService>() };
    size_t id = 1;
    for (size_t pattern = 0; pattern < 10; pattern++) {
        for (size_t track = 0; track < 8; track++) {
            for (size_t column = 0; column < 3; column++) {
                const auto newId = automationServiceOut.addMidiCcAutomation(pattern, track, column, controller, line0, line1, value0, value1, comment, track % 2 == 0, 8, 0);
                QCOMPARE(newId, id);
                id++;
            }
        }
    }

    AutomationService automationServiceIn { std::make_shared<PropertyService>() };
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    connect(&editorService, &EditorService::automationSerializationRequested, &automationServiceOut, &AutomationService::serializeToXml);
    connect(&editorService, &EditorService::automationDeserializationRequested, &automationServiceIn, &AutomationService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    for (size_t pattern = 0; pattern < 10; pattern++) {
        for (size_t track = 0; track < 8; track++) {
            for (size_t column = 0; column < 3; column++) {
                QVERIFY(automationServiceIn.hasAutomations(pattern, track, column, line0));
                QVERIFY(automationServiceIn.hasAutomations(pattern, track, column, line1));
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).size(), 1);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).controller(), controller);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).interpolation().line0, line0);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).interpolation().line1, line1);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).interpolation().value0, value0);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).interpolation().value1, value1);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).comment(), comment);
                QCOMPARE(automationServiceIn.midiCcAutomationsByLine(pattern, track, column, line0).at(0).enabled(), track % 2 == 0);
            }
        }
    }
}

void XmlSerializationTest::test_toXmlFromXml_automationService_midiCc_withModulation_shouldLoadAutomationService()
{
    AutomationService automationServiceOut { std::make_shared<PropertyService>() };
    const auto automationId = automationServiceOut.addMidiCcAutomation(0, 0, 0, 0, 0, 1, 0, 1, {}, true, 8, 0);
    automationServiceOut.addMidiCcModulation(automationId, 0, 1, 50.0f, 0.0f, true);

    AutomationService automationServiceIn { std::make_shared<PropertyService>() };
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    connect(&editorService, &EditorService::automationSerializationRequested, &automationServiceOut, &AutomationService::serializeToXml);
    connect(&editorService, &EditorService::automationDeserializationRequested, &automationServiceIn, &AutomationService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    const auto automation = automationServiceIn.midiCcAutomations().at(0);
    QCOMPARE(automation.modulation().cycles, 1.0f);
    QCOMPARE(automation.modulation().amplitude, 50.0f);
    QCOMPARE(automation.modulation().inverted, true);
}

void XmlSerializationTest::test_toXmlFromXml_automationService_midiCc_noModulation_shouldLoadAutomationService()
{
    AutomationService automationServiceOut { std::make_shared<PropertyService>() };
    automationServiceOut.addMidiCcAutomation(0, 0, 0, 0, 0, 1, 0, 1, {}, true, 8, 0);

    AutomationService automationServiceIn { std::make_shared<PropertyService>() };
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    connect(&editorService, &EditorService::automationSerializationRequested, &automationServiceOut, &AutomationService::serializeToXml);
    connect(&editorService, &EditorService::automationDeserializationRequested, &automationServiceIn, &AutomationService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    const auto automation = automationServiceIn.midiCcAutomations().at(0);
    QCOMPARE(automation.modulation().cycles, 0.0f);
    QCOMPARE(automation.modulation().amplitude, 0.0f);
    QCOMPARE(automation.modulation().inverted, false);
}

void XmlSerializationTest::test_toXmlFromXml_automationService_pitchBend_shouldLoadAutomationService()
{
    quint8 line0 = 4;
    quint8 line1 = 12;
    int value0 = -100;
    int value1 = +100;
    const auto comment = "Pitch Bend Automation Test";

    AutomationService automationServiceOut { std::make_shared<PropertyService>() };
    size_t id = 1;
    for (size_t pattern = 0; pattern < 10; pattern++) {
        for (size_t track = 0; track < 8; track++) {
            for (size_t column = 0; column < 3; column++) {
                const auto newId = automationServiceOut.addPitchBendAutomation(pattern, track, column, line0, line1, value0, value1, comment, track % 2 == 0);
                QCOMPARE(newId, id);
                id++;
            }
        }
    }

    AutomationService automationServiceIn { std::make_shared<PropertyService>() };
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    connect(&editorService, &EditorService::automationSerializationRequested, &automationServiceOut, &AutomationService::serializeToXml);
    connect(&editorService, &EditorService::automationDeserializationRequested, &automationServiceIn, &AutomationService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    for (size_t pattern = 0; pattern < 10; pattern++) {
        for (size_t track = 0; track < 8; track++) {
            for (size_t column = 0; column < 3; column++) {
                QVERIFY(automationServiceIn.hasAutomations(pattern, track, column, line0));
                QVERIFY(automationServiceIn.hasAutomations(pattern, track, column, line1));
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).size(), 1);
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).interpolation().line0, line0);
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).interpolation().line1, line1);
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).interpolation().value0, value0);
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).interpolation().value1, value1);
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).comment(), comment);
                QCOMPARE(automationServiceIn.pitchBendAutomationsByLine(pattern, track, column, line0).at(0).enabled(), track % 2 == 0);
            }
        }
    }
}

void XmlSerializationTest::test_toXmlFromXml_automationService_pitchBend_withModulation_shouldLoadAutomationService()
{
    AutomationService automationServiceOut { std::make_shared<PropertyService>() };
    const auto automationId = automationServiceOut.addPitchBendAutomation(0, 0, 0, 0, 1, 0, 1, {}, true);
    automationServiceOut.addPitchBendModulation(automationId, 1, 5, 25.0f, 10.0f, true);

    AutomationService automationServiceIn { std::make_shared<PropertyService>() };
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    connect(&editorService, &EditorService::automationSerializationRequested, &automationServiceOut, &AutomationService::serializeToXml);
    connect(&editorService, &EditorService::automationDeserializationRequested, &automationServiceIn, &AutomationService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    const auto automation = automationServiceIn.pitchBendAutomations().at(0);
    QCOMPARE(static_cast<int>(automation.modulation().type), 1);
    QCOMPARE(automation.modulation().cycles, 5.0f);
    QCOMPARE(automation.modulation().amplitude, 25.0f);
    QCOMPARE(automation.modulation().offset, 10.0f);
    QCOMPARE(automation.modulation().inverted, true);
}

void XmlSerializationTest::test_toXmlFromXml_mixerService_shouldLoadMixerService()
{
    MixerService mixerServiceOut;
    mixerServiceOut.muteTrack(1, true);
    mixerServiceOut.soloTrack(2, true);
    mixerServiceOut.muteColumn(3, 0, true);
    mixerServiceOut.soloColumn(4, 1, true);
    mixerServiceOut.setColumnVelocityScale(1, 2, 42);
    mixerServiceOut.setTrackVelocityScale(3, 66);
    connect(&mixerServiceOut, &MixerService::trackIndicesRequested, this, [&]() {
        mixerServiceOut.setTrackIndices({ 0, 1, 2, 3, 4 });
    });
    connect(&mixerServiceOut, &MixerService::columnCountOfTrackRequested, this, [&](auto && trackIndex) {
        mixerServiceOut.setColumnCount(trackIndex, 3);
    });

    MixerService mixerServiceIn;
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    connect(&editorService, &EditorService::mixerSerializationRequested, &mixerServiceOut, &MixerService::serializeToXml);
    connect(&editorService, &EditorService::mixerDeserializationRequested, &mixerServiceIn, &MixerService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    QVERIFY(mixerServiceIn.isTrackMuted(1));
    QVERIFY(mixerServiceIn.isTrackSoloed(2));
    QVERIFY(mixerServiceIn.isColumnMuted(3, 0));
    QVERIFY(mixerServiceIn.isColumnSoloed(4, 1));
    QCOMPARE(mixerServiceIn.columnVelocityScale(1, 2), 42);
    QCOMPARE(mixerServiceIn.trackVelocityScale(3), 66);
}

void XmlSerializationTest::test_toXmlFromXml_instrumentSettings_shouldParseInstrumentSettings()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);

    auto instrumentSettingsOut = std::make_shared<InstrumentSettings>();
    instrumentSettingsOut->patch = 42;
    instrumentSettingsOut->bank = { 10, 20, true };
    instrumentSettingsOut->transpose = -12;
    instrumentSettingsOut->timing.sendMidiClock = true;
    instrumentSettingsOut->timing.autoNoteOffOffset = std::chrono::milliseconds { 666 };
    instrumentSettingsOut->timing.delay = std::chrono::milliseconds { -666 };
    instrumentSettingsOut->midiEffects.velocityJitter = 42;
    instrumentSettingsOut->midiCcSettings = {
        { true, 7, 80 },
        { false, 10, 127 }
    };

    editorServiceOut.setInstrumentSettingsAtCurrentPosition(instrumentSettingsOut);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);

    editorServiceIn.requestPosition(0, 0, 0, 0, 0);
    const auto instrumentSettingsIn = editorServiceIn.instrumentSettingsAtCurrentPosition();

    QVERIFY(instrumentSettingsIn);

    QCOMPARE(instrumentSettingsIn->bank->byteOrderSwapped, instrumentSettingsOut->bank->byteOrderSwapped);
    QCOMPARE(instrumentSettingsIn->bank->lsb, instrumentSettingsOut->bank->lsb);
    QCOMPARE(instrumentSettingsIn->bank->msb, instrumentSettingsOut->bank->msb);
    QCOMPARE(instrumentSettingsIn->bank.has_value(), true);

    QCOMPARE(instrumentSettingsIn->patch, instrumentSettingsOut->patch);

    QCOMPARE(instrumentSettingsIn->transpose, instrumentSettingsOut->transpose);

    QCOMPARE(instrumentSettingsIn->timing.autoNoteOffOffset, instrumentSettingsOut->timing.autoNoteOffOffset);
    QCOMPARE(instrumentSettingsIn->timing.delay, instrumentSettingsOut->timing.delay);
    QCOMPARE(instrumentSettingsIn->timing.sendMidiClock, instrumentSettingsOut->timing.sendMidiClock);

    QCOMPARE(instrumentSettingsIn->midiEffects.velocityJitter, instrumentSettingsOut->midiEffects.velocityJitter);

    QCOMPARE(instrumentSettingsIn->midiCcSettings.size(), instrumentSettingsOut->midiCcSettings.size());
    for (size_t i = 0; i < instrumentSettingsOut->midiCcSettings.size(); ++i) {
        QCOMPARE(instrumentSettingsIn->midiCcSettings.at(i).enabled(), instrumentSettingsOut->midiCcSettings.at(i).enabled());
        QCOMPARE(instrumentSettingsIn->midiCcSettings.at(i).controller(), instrumentSettingsOut->midiCcSettings.at(i).controller());
        QCOMPARE(instrumentSettingsIn->midiCcSettings.at(i).value(), instrumentSettingsOut->midiCcSettings.at(i).value());
    }
}

void XmlSerializationTest::test_toXmlFromXml_sideChainService_shouldLoadSideChainService()
{
    SideChainService sideChainServiceOut;
    SideChainSettings settings;
    settings.enabled = true;
    settings.sourceTrackIndex = 1;
    settings.sourceColumnIndex = 2;
    settings.lookahead = std::chrono::milliseconds(10);
    settings.release = std::chrono::milliseconds(100);
    settings.targets.push_back({ true, 7, 127, 0 });
    settings.targets.push_back({ false, 10, 100, 10 });
    sideChainServiceOut.setSettings(0, settings);

    SideChainService sideChainServiceIn;
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    connect(&editorService, &EditorService::sideChainSerializationRequested, &sideChainServiceOut, &SideChainService::serializeToXml);
    connect(&editorService, &EditorService::sideChainDeserializationRequested, &sideChainServiceIn, &SideChainService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    const auto settingsIn = sideChainServiceIn.settings(0);

    QCOMPARE(settingsIn.enabled, settings.enabled);
    QCOMPARE(settingsIn.sourceTrackIndex, settings.sourceTrackIndex);
    QCOMPARE(settingsIn.sourceColumnIndex, settings.sourceColumnIndex);
    QCOMPARE(settingsIn.lookahead, settings.lookahead);
    QCOMPARE(settingsIn.release, settings.release);
    QCOMPARE(settingsIn.targets.size(), settings.targets.size());
    for (size_t i = 0; i < settings.targets.size(); ++i) {
        QCOMPARE(settingsIn.targets.at(i).enabled, settings.targets.at(i).enabled);
        QCOMPARE(settingsIn.targets.at(i).controller, settings.targets.at(i).controller);
        QCOMPARE(settingsIn.targets.at(i).targetValue, settings.targets.at(i).targetValue);
        QCOMPARE(settingsIn.targets.at(i).releaseValue, settings.targets.at(i).releaseValue);
    }
}

void XmlSerializationTest::test_toXmlFromXml_noteData_noteOn()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(1, 3, 64);

    editorServiceOut.requestPosition(0, 0, 0, 2, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(3, 3, 80);

    editorServiceOut.requestPosition(0, 1, 0, 0, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(2, 4, 100);

    editorServiceOut.requestPosition(0, 1, 0, 2, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(3, 4, 127);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);

    auto noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 0), "C-3");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 0), "064");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 2, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 2), "D-3");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 2), "080");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 1, 0, 0, 0 });
    QCOMPARE(noteData->track(), 1);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 1, 0, 0), "C#4");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 1, 0, 0), "100");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 1, 0, 2, 0 });
    QCOMPARE(noteData->track(), 1);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 1, 0, 2), "D-4");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 1, 0, 2), "127");
}

void XmlSerializationTest::test_toXmlFromXml_noteData_delay_shouldSaveAndLoadDelay()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(1, 3, 64);
    editorServiceOut.setDelayOnCurrentLine(12);

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);
    QCOMPARE(editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 })->delay(), 12);

    editorServiceIn.requestPosition(0, 0, 0, 0, 0);
    QCOMPARE(editorServiceIn.delayAtCurrentPosition(), 12);

    editorServiceIn.requestPosition(0, 0, 0, 1, 0);
    QCOMPARE(editorServiceIn.delayAtCurrentPosition(), 0);
}

void XmlSerializationTest::test_toXmlFromXml_noteData_noteOff()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNoteOffAtCurrentPosition();

    editorServiceOut.requestPosition(0, 0, 0, 2, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(3, 3, 80);

    editorServiceOut.requestPosition(0, 0, 0, 4, 0);
    editorServiceOut.requestNoteOffAtCurrentPosition();

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);

    auto noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 0), "OFF");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 0), "---");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 2, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 2), "D-3");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 2), "080");

    noteData = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 4, 0 });
    QCOMPARE(noteData->track(), 0);
    QCOMPARE(noteData->column(), 0);
    QCOMPARE(editorServiceIn.displayNoteAtPosition(0, 0, 0, 4), "OFF");
    QCOMPARE(editorServiceIn.displayVelocityAtPosition(0, 0, 0, 4), "---");
}

void XmlSerializationTest::test_toXmlFromXml_instrument_shouldParseInstrument()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };

    // Set up the instrument with all possible properties
    const auto instrumentOut = std::make_shared<Instrument>("Test Port");
    auto address = instrumentOut->midiAddress();
    address.setChannel(10); // Example channel
    instrumentOut->setMidiAddress(address);
    auto settings = instrumentOut->settings();
    settings.patch = 42; // Optional patch
    settings.bank = {
        static_cast<uint8_t>(21), // Bank LSB
        static_cast<uint8_t>(34), // Bank MSB
        true // Byte order swapped
    };
    instrumentOut->setSettings(settings);
    editorServiceOut.setInstrument(0, instrumentOut);

    // Serialize to XML
    const auto xml = editorServiceOut.toXml();

    // Deserialize from XML
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);

    // Retrieve the instrument
    const auto instrumentIn = editorServiceIn.instrument(0);

    // Validate the instrument
    QVERIFY(instrumentIn);
    QCOMPARE(instrumentIn->midiAddress().portName(), instrumentOut->midiAddress().portName());
    QCOMPARE(instrumentIn->midiAddress().channel(), instrumentOut->midiAddress().channel());

    // Validate optional properties
    QCOMPARE(instrumentIn->settings().patch.has_value(), instrumentOut->settings().patch.has_value());
    if (instrumentIn->settings().patch && instrumentOut->settings().patch) {
        QCOMPARE(*instrumentIn->settings().patch, *instrumentOut->settings().patch);
    }

    QCOMPARE(instrumentIn->settings().bank.has_value(), instrumentOut->settings().bank.has_value());
    if (instrumentIn->settings().bank && instrumentOut->settings().bank) {
        QCOMPARE(instrumentIn->settings().bank->lsb, instrumentOut->settings().bank->lsb);
        QCOMPARE(instrumentIn->settings().bank->msb, instrumentOut->settings().bank->msb);
        QCOMPARE(instrumentIn->settings().bank->byteOrderSwapped, instrumentOut->settings().bank->byteOrderSwapped);
    }
}

void XmlSerializationTest::test_toXmlFromXml_addTrack_shouldLoadSong()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNewTrackToRight();

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.trackCount(), editorServiceOut.trackCount());
    QCOMPARE(editorServiceIn.trackIndices(), editorServiceOut.trackIndices());
}

void XmlSerializationTest::test_toXmlFromXml_removeTrack_shouldLoadSong()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestTrackDeletion();

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.trackCount(), editorServiceOut.trackCount());
    QCOMPARE(editorServiceIn.trackIndices(), editorServiceOut.trackIndices());
}

void XmlSerializationTest::test_toXmlFromXml_template_shouldLoadTemplate()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceOut.setPatternAtSongPosition(0, 15);

    const auto xml = editorServiceOut.toXmlAsTemplate();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 0);
    QCOMPARE(editorServiceIn.patternCount(), 1);
}

void XmlSerializationTest::test_toXmlFromXml_differentSongs_shouldLoadSongs()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 0);
    QCOMPARE(editorServiceIn.patternCount(), 1);

    editorServiceOut.setPatternAtSongPosition(0, 15);
    xml = editorServiceOut.toXml();

    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 15);
    QCOMPARE(editorServiceIn.patternCount(), 2);

    EditorService editorServiceOut2 { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    xml = editorServiceOut2.toXml();

    editorServiceIn.fromXml(xml);
    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 0);
    QCOMPARE(editorServiceIn.patternCount(), 1);
}

void XmlSerializationTest::test_toXmlFromXml_trackDrumTrack_shouldLoadTrackDrumTrack()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    auto instrument = std::make_shared<Instrument>("");
    auto settings = instrument->settings();
    settings.drumTrack = true;
    instrument->setSettings(settings);
    editorServiceOut.setInstrument(0, instrument);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()) };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.instrument(0)->settings().drumTrack, true);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::XmlSerializationTest)
