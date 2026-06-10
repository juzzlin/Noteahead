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

#include "application/service/application_service.hpp"
#include "application/service/audio_service.hpp"
#include "application/service/automation_service.hpp"
#include "application/service/device_service.hpp"
#include "application/service/editor_service.hpp"
#include "application/service/jack_service.hpp"
#include "application/service/mixer_service.hpp"
#include "application/service/property_service.hpp"
#include "application/service/selection_service.hpp"
#include "application/service/settings_service.hpp"
#include "application/service/side_chain_service.hpp"
#include "common/constants.hpp"
#include "domain/devices/device_factory.hpp"
#include "domain/devices/sampler_device.hpp"
#include "domain/devices/synth_device.hpp"
#include "domain/dsp/chorus_effect.hpp"
#include "domain/dsp/reverb_effect.hpp"
#include "domain/effects/effect_factory.hpp"
#include "domain/tracker/column_settings.hpp"
#include "domain/tracker/instrument.hpp"
#include "domain/tracker/note_data.hpp"
#include "domain/tracker/track.hpp"
#include "infra/audio/audio_engine.hpp"
#include "infra/audio/backend/audio_file_reader.hpp"
#include "infra/data_service.hpp"

#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>

namespace noteahead {

void XmlSerializationTest::initTestCase()
{
    EffectFactory::init();
}

void XmlSerializationTest::cleanupTestCase()
{
    EffectFactory::clear();
}

class MockAudioFileReader : public AudioFileReader
{
public:
    bool open(const std::string &, Mode, Info & info) override
    {
        info.frames = 1000;
        info.samplerate = static_cast<uint32_t>(Constants::defaultSampleRate());
        info.channels = 2;
        m_info = info;
        m_isOpen = true;
        return true;
    }

    void close() override
    {
        m_isOpen = false;
    }

    int64_t readFloat(std::span<float> data) override
    {
        std::fill(data.begin(), data.end(), 0.0f);
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }

    int64_t readDouble(std::span<double> data) override
    {
        std::fill(data.begin(), data.end(), 0.0);
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }

    int64_t readInt(std::span<int32_t> data) override
    {
        std::fill(data.begin(), data.end(), 0);
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }

    int64_t writeFloat(std::span<const float> data) override
    {
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }

    int64_t writeInt(std::span<const int32_t> data) override
    {
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }

    bool seek(int64_t, int) override
    {
        return true;
    }

    bool isOpen() const override
    {
        return m_isOpen;
    }

    Info info() const override
    {
        return m_info;
    }

private:
    bool m_isOpen = false;
    Info m_info;
};

void XmlSerializationTest::test_toXmlFromXml_playOrder_shouldBeCorrect()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.setPatternAtSongPosition(1, 11);
    editorServiceOut.setPatternAtSongPosition(2, 22);
    editorServiceOut.setPatternAtSongPosition(3, 33);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 0);
    QCOMPARE(editorServiceIn.patternAtSongPosition(1), 11);
    QCOMPARE(editorServiceIn.patternAtSongPosition(2), 22);
    QCOMPARE(editorServiceIn.patternAtSongPosition(3), 33);
}

void XmlSerializationTest::test_toXmlFromXml_songProperties_shouldBeCorrect()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.setBeatsPerMinute(666);
    editorServiceOut.setLinesPerBeat(42);
    editorServiceOut.setPatternName(0, "patternName");
    editorServiceOut.setSongLength(16);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
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
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.setColumnName(0, 0, "columnName0_0");
    editorServiceOut.setColumnName(1, 0, "columnName1_0");

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.columnName(0, 0), editorServiceOut.columnName(0, 0));
    QCOMPARE(editorServiceIn.columnName(1, 0), editorServiceOut.columnName(1, 0));
}

void XmlSerializationTest::test_toXmlFromXml_trackName_shouldLoadTrackName()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.setTrackName(0, "trackName0");
    editorServiceOut.setTrackName(1, "trackName1");

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.trackName(0), editorServiceOut.trackName(0));
    QCOMPARE(editorServiceIn.trackName(1), editorServiceOut.trackName(1));
}

void XmlSerializationTest::test_toXmlFromXml_columnSettings_shouldSaveAndLoad()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    const auto settingsOut = std::make_shared<ColumnSettings>();
    settingsOut->delay = std::chrono::milliseconds { 123 };
    settingsOut->midiDelayEnabled = true;
    settingsOut->midiDelayLines = 1.5;
    settingsOut->midiDelayFeedback = 75;
    settingsOut->midiDelayMaxRepetitions = 12;
    settingsOut->transpose = -12;
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
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    const auto settingsIn = editorServiceIn.columnSettings(1, 0);

    QVERIFY(settingsIn);
    QCOMPARE(settingsIn->delay, settingsOut->delay);
    QCOMPARE(settingsIn->midiDelayEnabled, settingsOut->midiDelayEnabled);
    QCOMPARE(settingsIn->midiDelayLines, settingsOut->midiDelayLines);
    QCOMPARE(settingsIn->midiDelayFeedback, settingsOut->midiDelayFeedback);
    QCOMPARE(settingsIn->midiDelayMaxRepetitions, settingsOut->midiDelayMaxRepetitions);
    QCOMPARE(settingsIn->transpose, settingsOut->transpose);
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
    const quint8 controller = 64;
    const quint8 line0 = 4;
    const quint8 line1 = 12;
    const quint8 value0 = 0;
    const quint8 value1 = 100;
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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
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
    const quint8 line0 = 4;
    const quint8 line1 = 12;
    const int value0 = -100;
    const int value1 = +100;
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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
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
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
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

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
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
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
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

void XmlSerializationTest::test_toXmlFromXml_noteData_noteOn_shouldBeCorrect()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };

    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(1, 3, 64);

    editorServiceOut.requestPosition(0, 0, 0, 2, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(3, 3, 80);

    editorServiceOut.requestPosition(0, 1, 0, 0, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(2, 4, 100);

    editorServiceOut.requestPosition(0, 1, 0, 2, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(3, 4, 127);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
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
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(1, 3, 64);
    editorServiceOut.setDelayOnCurrentLine(12);

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);
    QCOMPARE(editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 })->delay(), 12);

    editorServiceIn.requestPosition(0, 0, 0, 0, 0);
    QCOMPARE(editorServiceIn.delayAtCurrentPosition(), 12);

    editorServiceIn.requestPosition(0, 0, 0, 1, 0);
    QCOMPARE(editorServiceIn.delayAtCurrentPosition(), 0);
}

void XmlSerializationTest::test_toXmlFromXml_noteData_noteOff_shouldBeCorrect()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };

    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNoteOffAtCurrentPosition();

    editorServiceOut.requestPosition(0, 0, 0, 2, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(3, 3, 80);

    editorServiceOut.requestPosition(0, 0, 0, 4, 0);
    editorServiceOut.requestNoteOffAtCurrentPosition();

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
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
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };

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
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
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
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNewTrackToRight();

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.trackCount(), editorServiceOut.trackCount());
    QCOMPARE(editorServiceIn.trackIndices(), editorServiceOut.trackIndices());
}

void XmlSerializationTest::test_toXmlFromXml_removeTrack_shouldLoadSong()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestTrackDeletion();

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.trackCount(), editorServiceOut.trackCount());
    QCOMPARE(editorServiceIn.trackIndices(), editorServiceOut.trackIndices());
}

void XmlSerializationTest::test_toXmlFromXml_template_shouldLoadTemplate()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.setPatternAtSongPosition(0, 15);

    const auto xml = editorServiceOut.toXmlAsTemplate();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 0);
    QCOMPARE(editorServiceIn.patternCount(), 1);
}

void XmlSerializationTest::test_toXmlFromXml_differentSongs_shouldLoadSongs()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 0);
    QCOMPARE(editorServiceIn.patternCount(), 1);

    editorServiceOut.setPatternAtSongPosition(0, 15);
    xml = editorServiceOut.toXml();

    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 15);
    QCOMPARE(editorServiceIn.patternCount(), 2);

    EditorService editorServiceOut2 { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    xml = editorServiceOut2.toXml();

    editorServiceIn.fromXml(xml);
    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 0);
    QCOMPARE(editorServiceIn.patternCount(), 1);
}

void XmlSerializationTest::test_toXmlFromXml_trackDrumTrack_shouldLoadTrackDrumTrack()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    auto instrument = std::make_shared<Instrument>("");
    auto settings = instrument->settings();
    settings.drumTrack = true;
    instrument->setSettings(settings);
    editorServiceOut.setInstrument(0, instrument);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.instrument(0)->settings().drumTrack, true);
}

void XmlSerializationTest::test_toXmlFromXml_samplerDevice_shouldLoadSamplerDevice()
{
    const std::string fileName = "test.wav";
    const auto samplerName = "Noteahead Internal Device 1";

    const auto engine = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engine, std::make_shared<DataService>() };
    const auto samplerOut = std::make_shared<SamplerDevice>(samplerName, std::make_unique<MockAudioFileReader>());
    samplerOut->loadSample(60, fileName);
    samplerOut->setSamplePan(60, 0.75f);
    samplerOut->setSampleVolume(60, 0.8f);
    samplerOut->setSampleCutoff(60, 0.4f);
    deviceServiceOut.setDevice(0, samplerOut);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto deviceServiceIn = std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>());
    const auto samplerIn = std::make_shared<SamplerDevice>(samplerName, std::make_unique<MockAudioFileReader>());
    deviceServiceIn->setDevice(0, samplerIn);

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, deviceServiceIn.get(), &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    QCOMPARE(samplerIn->id(), 0ull);
    const auto sample = samplerIn->sample(60);
    QVERIFY(sample);
    QCOMPARE(sample->filePath, fileName);
    QCOMPARE(samplerIn->samplePan(60), 0.75f);
    QCOMPARE(samplerIn->sampleVolume(60), 0.8f);
    QCOMPARE(samplerIn->sampleCutoff(60), 0.4f);
}

void XmlSerializationTest::test_toXmlFromXml_samplerDevice_relativePath_shouldLoadCorrectly()
{
    const std::string projectPath = "/tmp/noteahead_test";
    const std::string relativePath = "samples/kick.wav";
    const std::string absolutePath = QDir(QString::fromStdString(projectPath)).absoluteFilePath(QString::fromStdString(relativePath)).toStdString();
    const auto samplerName = "Noteahead Internal Device 1";

    const auto engine = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engine, std::make_shared<DataService>() };
    deviceServiceOut.setProjectPath(projectPath);

    const auto samplerOut = std::make_shared<SamplerDevice>(samplerName, std::make_unique<MockAudioFileReader>());
    samplerOut->loadSample(60, absolutePath);
    deviceServiceOut.setDevice(0, samplerOut);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto deviceServiceIn = std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>());
    deviceServiceIn->setProjectPath(projectPath);

    const auto samplerIn = std::make_shared<SamplerDevice>(samplerName, std::make_unique<MockAudioFileReader>());
    deviceServiceIn->setDevice(0, samplerIn);

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, deviceServiceIn.get(), &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    QCOMPARE(samplerIn->absoluteFilePath(60), absolutePath);
}

void XmlSerializationTest::test_toXmlFromXml_synthDevice_shouldPreserveValuesAndDiscreteFlags()
{
    const auto synthName = "Noteahead Internal Device 1";

    const auto engine = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engine, std::make_shared<DataService>() };
    const auto synthOut = std::make_shared<SynthDevice>(synthName);
    synthOut->setVco1Waveform(PolyBlepOscillator::Waveform::Saw);
    synthOut->setVco1Octave(1);
    synthOut->setMixVco2(0.75f);
    synthOut->setLpfCutoff(0.3f);
    synthOut->setAmpAttack(0.2f);
    synthOut->setAmpVelocitySensitivity(0.7f);
    synthOut->setMultiType(MultiEngine::Type::Decim);
    synthOut->setMultiShape(0.42f);
    synthOut->setMultiLevel(0.88f);
    synthOut->setMultiKeyTrack(0.5f);
    synthOut->setPan(0.12f);
    synthOut->setDelayType(DelayEffect::Type::PingPong);
    synthOut->setDelaySync(true);
    synthOut->setDelaySyncDivision(0.25f);
    synthOut->setFeedbackLpf(0.6f);
    synthOut->setFeedbackHpf(0.2f);
    deviceServiceOut.setDevice(0, synthOut);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto deviceServiceIn = std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>());
    const auto synthIn = std::make_shared<SynthDevice>(synthName);
    deviceServiceIn->setDevice(0, synthIn);

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, deviceServiceIn.get(), &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    QCOMPARE(synthIn->id(), 0ull);
    QCOMPARE(synthIn->vco1Waveform(), PolyBlepOscillator::Waveform::Saw);
    QCOMPARE(synthIn->vco1Octave(), 1);
    QCOMPARE(synthIn->mixVco2(), 0.75f);
    QCOMPARE(synthIn->lpfCutoff(), 0.3f);
    QCOMPARE(synthIn->ampAttack(), 0.2f);
    QCOMPARE(synthIn->ampVelocitySensitivity(), 0.7f);
    QCOMPARE(synthIn->multiType(), MultiEngine::Type::Decim);
    QCOMPARE(synthIn->multiShape(), 0.42f);
    QCOMPARE(synthIn->multiLevel(), 0.88f);
    QCOMPARE(synthIn->multiKeyTrack(), 0.5f);
    QCOMPARE(synthIn->pan(), 0.12f);
    QCOMPARE(synthIn->delayType(), DelayEffect::Type::PingPong);
    QCOMPARE(synthIn->delaySync(), true);
    QCOMPARE(synthIn->delaySyncDivision(), 0.25f);
    QCOMPARE(synthIn->delayFeedbackLpf(), 0.6f);
    QCOMPARE(synthIn->delayFeedbackHpf(), 0.2f);

    // Verify discrete flags
    const auto vco1Wave = synthIn->parameter(Constants::NahdXml::xmlKeySynthVco1Waveform().toStdString());
    QVERIFY(vco1Wave.has_value());
    QVERIFY(vco1Wave->get().isDiscrete());

    const auto vco1Octave = synthIn->parameter(Constants::NahdXml::xmlKeySynthVco1Octave().toStdString());
    QVERIFY(vco1Octave.has_value());
    QVERIFY(vco1Octave->get().isDiscrete());

    const auto lpfCutoff = synthIn->parameter(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString());
    QVERIFY(lpfCutoff.has_value());
    QVERIFY(!lpfCutoff->get().isDiscrete());

    const auto multiShape = synthIn->parameter(Constants::NahdXml::xmlKeySynthMultiShape().toStdString());
    QVERIFY(multiShape.has_value());
    QVERIFY(!multiShape->get().isDiscrete());
}

void XmlSerializationTest::test_toXmlFromXml_synthUserPresets_shouldSaveAndLoad()
{
    const auto engine = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engine, std::make_shared<DataService>() };
    const auto synthOut = std::make_shared<SynthDevice>("Noteahead Internal Device 1");
    deviceServiceOut.setDevice(0, synthOut);

    UserPresets userPresets;
    SynthPreset preset;
    preset.name = "Test Preset";
    preset.parameters[Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()] = 0.5f;
    userPresets[10] = preset;
    deviceServiceOut.setSynthUserPresets(userPresets);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    // Verify XML contains metadata (min/max/default/scale)
    QVERIFY(xml.contains("min=\"0\""));
    QVERIFY(xml.contains("max=\"10000\""));

    const auto deviceServiceIn = std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>());
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, deviceServiceIn.get(), &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto & userPresetsIn = deviceServiceIn->synthUserPresets();
    QVERIFY(userPresetsIn.count(10));
    QCOMPARE(userPresetsIn.at(10).name, std::string("Test Preset"));
    QCOMPARE(userPresetsIn.at(10).parameters.at(Constants::NahdXml::xmlKeySynthLpfCutoff().toStdString()), 0.5f);
}

void XmlSerializationTest::test_toXmlFromXml_synthUserPresets_discreteValues_shouldSaveAndLoad()
{
    const auto engine = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engine, std::make_shared<DataService>() };
    const auto synthOut = std::make_shared<SynthDevice>("Noteahead Internal Device 1");
    deviceServiceOut.setDevice(0, synthOut);

    UserPresets userPresets;
    SynthPreset preset;
    preset.name = "Discrete Test";
    // Waveform: Saw (1.0f)
    preset.parameters[Constants::NahdXml::xmlKeySynthVco1Waveform().toStdString()] = 1.0f;
    // Pitch: 0 cents
    preset.parameters[Constants::NahdXml::xmlKeySynthVco1Pitch().toStdString()] = 0.0f;
    userPresets[5] = preset;
    deviceServiceOut.setSynthUserPresets(userPresets);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto deviceServiceIn = std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>());
    const auto synthIn = std::make_shared<SynthDevice>("Noteahead Internal Device 1");
    deviceServiceIn->setDevice(0, synthIn);

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, deviceServiceIn.get(), &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto & userPresetsIn = deviceServiceIn->synthUserPresets();
    QVERIFY(userPresetsIn.count(5));
    // Verify internal values are preserved
    QCOMPARE(userPresetsIn.at(5).parameters.at(Constants::NahdXml::xmlKeySynthVco1Waveform().toStdString()), 1.0f);
    QCOMPARE(userPresetsIn.at(5).parameters.at(Constants::NahdXml::xmlKeySynthVco1Pitch().toStdString()), 0.0f);
}

void XmlSerializationTest::test_toXmlFromXml_masterSendEffects_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    // Add a reverb effect to slot 0 of master send rack
    auto reverb = std::make_shared<ReverbEffect>();
    reverb->setDecay(0.75f); // internal value
    reverb->setLpfCutoff(0.72f);
    reverb->setHpfCutoff(0.28f);
    deviceServiceOut.sendEffectRack().setEffect(0, reverb);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), ReverbEffect::typeIdString());
    auto restoredReverb = std::dynamic_pointer_cast<ReverbEffect>(effect);
    QVERIFY(restoredReverb);
    QCOMPARE(restoredReverb->decay(), 0.75f);
    QCOMPARE(restoredReverb->lpfCutoff(), 0.72f);
    QCOMPARE(restoredReverb->hpfCutoff(), 0.28f);
}

void XmlSerializationTest::test_toXmlFromXml_chorusEffect_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    // Add a chorus effect to slot 0 of master send rack
    auto chorus = std::make_shared<ChorusEffect>();
    chorus->setRate(0.5f);
    chorus->setDepth(0.6f);
    chorus->setDelay(0.4f);
    chorus->setMix(0.7f);
    chorus->setWidth(0.8f);
    chorus->setLpfCutoff(0.9f);
    chorus->setHpfCutoff(0.1f);
    deviceServiceOut.sendEffectRack().setEffect(0, chorus);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), ChorusEffect::typeIdString());
    auto restoredChorus = std::dynamic_pointer_cast<ChorusEffect>(effect);
    QVERIFY(restoredChorus);
    QCOMPARE(restoredChorus->rate(), 0.5f);
    QCOMPARE(restoredChorus->depth(), 0.6f);
    QCOMPARE(restoredChorus->delay(), 0.4f);
    QCOMPARE(restoredChorus->mix(), 0.7f);
    QCOMPARE(restoredChorus->width(), 0.8f);
    QCOMPARE(restoredChorus->lpfCutoff(), 0.9f);
    QCOMPARE(restoredChorus->hpfCutoff(), 0.1f);
}

void XmlSerializationTest::test_fromXml_samplerDevice_missingId_shouldNotThrow()
{
    const auto xml = QString(R"XML(
<Project applicationName="Noteahead" applicationVersion="2.0.0" fileFormatVersion="1.0">
    <Song beatsPerMinute="120" linesPerBeat="8">
        <Devices>
            <Sampler>
                <Sample note="60" path="test.wav"/>
            </Sampler>
        </Devices>
        <Patterns>
            <Pattern index="0" lineCount="64" name="" trackCount="8">
                <Tracks/>
            </Pattern>
        </Patterns>
    </Song>
</Project>
)XML");

    DeviceService deviceServiceIn { std::make_shared<AudioEngine>(), std::make_shared<DataService>() };
    auto samplerIn = std::make_shared<SamplerDevice>(Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>());
    deviceServiceIn.setDevice(0, samplerIn);

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    // This should not throw anymore
    editorServiceIn.fromXml(xml);

    QCOMPARE(samplerIn->id(), 0); // Default value
}

void XmlSerializationTest::test_toXmlFromXml_audioRecorder_shouldLoadAudioRecorder()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    const auto fileName = tempFile.fileName();
    const quint64 startTick = 480;
    const quint64 endTick = 960;

    auto settingsService = std::make_shared<SettingsService>();
    auto engine = std::make_shared<AudioEngine>();
    auto jackService = std::make_shared<JackService>(settingsService, engine);
    auto audioServiceOut = std::make_shared<AudioService>(settingsService, jackService, engine, nullptr, false);
    audioServiceOut->setLatestRecordingInfo(fileName, startTick, endTick);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    QObject::connect(&editorServiceOut, &EditorService::audioRecorderSerializationRequested, audioServiceOut.get(), &AudioService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    auto audioServiceIn = std::make_shared<AudioService>(settingsService, jackService, engine, nullptr, false);
    EditorService editorServiceIn { std::make_shared<SelectionService>(), settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    QObject::connect(&editorServiceIn, &EditorService::audioRecorderDeserializationRequested, audioServiceIn.get(), &AudioService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    QCOMPARE(audioServiceIn->latestRecordingFileName(), fileName);
    QCOMPARE(audioServiceIn->latestRecordingStartTick(), startTick);
    QCOMPARE(audioServiceIn->latestRecordingEndTick(), endTick);
}

void XmlSerializationTest::test_fromXml_missingPatterns_shouldRemoveThemFromPlayOrder()
{
    // Construct XML with PlayOrder pointing to pattern 1, but pattern 1 is missing
    const auto xml = QString(R"XML(
<Project applicationName="Noteahead" applicationVersion="2.0.0" fileFormatVersion="1.0">
    <Song beatsPerMinute="120" linesPerBeat="8">
        <PlayOrder>
            <Position index="0" pattern="0"/>
            <Position index="1" pattern="1"/>
        </PlayOrder>
        <Patterns>
            <Pattern index="0" lineCount="64" name="" trackCount="8">
                <Tracks/>
            </Pattern>
        </Patterns>
    </Song>
</Project>
)XML");

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    // Pattern 1 should be removed from PlayOrder, so length should be 1
    QCOMPARE(editorServiceIn.songLength(), 1);
    QCOMPARE(editorServiceIn.patternAtSongPosition(0), 0);
}

void XmlSerializationTest::test_fromXml_legacyLength_shouldBeSupported()
{
    // Construct XML with legacy 'length' attribute
    const auto xml = QString(R"XML(
<Project applicationName="Noteahead" applicationVersion="2.0.0" fileFormatVersion="1.0">
    <Song beatsPerMinute="120" linesPerBeat="8" length="5">
        <Patterns>
            <Pattern index="0" lineCount="64" name="" trackCount="8">
                <Tracks/>
            </Pattern>
        </Patterns>
    </Song>
</Project>
)XML");

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    // length="5" should be respected
    QCOMPARE(editorServiceIn.songLength(), 5);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::XmlSerializationTest)
