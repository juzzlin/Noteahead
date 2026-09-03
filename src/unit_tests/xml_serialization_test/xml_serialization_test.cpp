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

#include "../../application/position.hpp"
#include "../../application/service/application_service.hpp"
#include "../../application/service/audio_service.hpp"
#include "../../application/service/automation_service.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/jack_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device_factory.hpp"
#include "../../domain/devices/drum_synth_constants.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/devices/kick_808_device.hpp"
#include "../../domain/devices/piano_synth_v2_device.hpp"
#include "../../domain/devices/piano_synth_v3_device.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../domain/devices/speech_device.hpp"
#include "../../domain/devices/string_ensemble_device.hpp"
#include "../../domain/devices/string_voice_device.hpp"
#include "../../domain/devices/string_voice_v2_device.hpp"
#include "../../domain/devices/sub_mixer_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/devices/wavetable_synth_device.hpp"
#include "../../domain/effects/air_band_eq.hpp"
#include "../../domain/effects/all_pass_filter.hpp"
#include "../../domain/effects/analog_fuzz.hpp"
#include "../../domain/effects/auto_ducker.hpp"
#include "../../domain/effects/auto_filter.hpp"
#include "../../domain/effects/bass_grinder.hpp"
#include "../../domain/effects/chorus.hpp"
#include "../../domain/effects/clipper.hpp"
#include "../../domain/effects/delay.hpp"
#include "../../domain/effects/dimension.hpp"
#include "../../domain/effects/drive.hpp"
#include "../../domain/effects/early_reflections.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/effects/endless_reverb.hpp"
#include "../../domain/effects/eq_8_band_parametric.hpp"
#include "../../domain/effects/gain.hpp"
#include "../../domain/effects/limiter.hpp"
#include "../../domain/effects/monitor.hpp"
#include "../../domain/effects/multiband_compressor.hpp"
#include "../../domain/effects/phaser.hpp"
#include "../../domain/effects/reverb.hpp"
#include "../../domain/effects/saturator.hpp"
#include "../../domain/effects/simple_eq.hpp"
#include "../../domain/effects/stereo_enhancer.hpp"
#include "../../domain/effects/stereo_exciter.hpp"
#include "../../domain/effects/stereo_widener.hpp"
#include "../../domain/effects/tube_stage.hpp"
#include "../../domain/effects/vintage_passive_eq.hpp"
#include "../../domain/effects/wave_designer.hpp"
#include "../../domain/tracker/auto_note_off_offset.hpp"
#include "../../domain/tracker/column_settings.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../domain/tracker/interpolator.hpp"
#include "../../domain/tracker/note_data.hpp"
#include "../../domain/tracker/song.hpp"
#include "../../domain/tracker/song_settings.hpp"
#include "../../domain/tracker/track.hpp"
#include "../../domain/utility/dbtp_meter.hpp"
#include "../../domain/utility/lufs_meter.hpp"
#include "../../domain/utility/stereo_field_meter.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"
#include "../../infra/data_service.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QTest>

#include <cmath>

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

    void setTag(TagType type, const std::string & value) override
    {
        (void)type;
        (void)value;
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

void XmlSerializationTest::test_toXmlFromXml_songMetadata_shouldRoundTrip()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.setSongMetadataTitle("Song Title");
    editorServiceOut.setSongMetadataArtist("Song Artist");
    editorServiceOut.setSongMetadataAlbum("Song Album");
    editorServiceOut.setSongMetadataDate("2026");
    editorServiceOut.setSongMetadataGenre("Chiptune");
    editorServiceOut.setSongMetadataTrackNumber("3");
    editorServiceOut.setSongMetadataComment("Song Comment");

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.songMetadataTitle(), editorServiceOut.songMetadataTitle());
    QCOMPARE(editorServiceIn.songMetadataArtist(), editorServiceOut.songMetadataArtist());
    QCOMPARE(editorServiceIn.songMetadataAlbum(), editorServiceOut.songMetadataAlbum());
    QCOMPARE(editorServiceIn.songMetadataDate(), editorServiceOut.songMetadataDate());
    QCOMPARE(editorServiceIn.songMetadataGenre(), editorServiceOut.songMetadataGenre());
    QCOMPARE(editorServiceIn.songMetadataTrackNumber(), editorServiceOut.songMetadataTrackNumber());
    QCOMPARE(editorServiceIn.songMetadataComment(), editorServiceOut.songMetadataComment());
}

void XmlSerializationTest::test_toXmlFromXml_songMetadata_empty_shouldRoundTrip()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.songMetadataTitle(), QString {});
    QCOMPARE(editorServiceIn.songMetadataArtist(), QString {});
    QCOMPARE(editorServiceIn.songMetadataComment(), QString {});
}

void XmlSerializationTest::test_toXmlFromXml_songSettings_milliseconds_shouldRoundTrip()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.song()->settings().setAutoNoteOffOffset(AutoNoteOffOffset { std::chrono::milliseconds { 333 } });

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    const auto offsetIn = editorServiceIn.song()->settings().autoNoteOffOffset();
    QVERIFY(offsetIn.has_value());
    QVERIFY(!offsetIn->syncEnabled());
    QCOMPARE(offsetIn->milliseconds(), std::chrono::milliseconds { 333 });
}

void XmlSerializationTest::test_toXmlFromXml_songSettings_sync_shouldRoundTrip()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    AutoNoteOffOffset offsetOut { 64 };
    offsetOut.setMilliseconds(std::chrono::milliseconds { 42 });
    editorServiceOut.song()->settings().setAutoNoteOffOffset(offsetOut);

    const auto xml = editorServiceOut.toXml();

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    const auto offsetIn = editorServiceIn.song()->settings().autoNoteOffOffset();
    QVERIFY(offsetIn.has_value());
    QVERIFY(offsetIn->syncEnabled());
    QCOMPARE(offsetIn->syncDenominator(), 64);
    // The inactive mode's value survives the trip, so switching back in the UI restores it.
    QCOMPARE(offsetIn->milliseconds(), std::chrono::milliseconds { 42 });
}

void XmlSerializationTest::test_fromXml_songSettingsMissing_shouldSeedFromApplicationDefault()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    auto xml = editorServiceOut.toXml();

    // A project saved before the setting moved into the song has no SongSettings element at all.
    xml.remove(QRegularExpression { "<SongSettings[^>]*/>" });
    QVERIFY(!xml.contains(Constants::NahdXml::xmlKeySongSettings()));

    const auto settingsService = std::make_shared<SettingsService>();
    settingsService->setAutoNoteOffOffset(200);
    EditorService editorServiceIn { std::make_shared<SelectionService>(), settingsService, std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    // Such a song used to play with the application-wide offset, so it still does.
    const auto offsetIn = editorServiceIn.song()->settings().autoNoteOffOffset();
    QVERIFY(offsetIn.has_value());
    QVERIFY(!offsetIn->syncEnabled());
    QCOMPARE(offsetIn->milliseconds(), std::chrono::milliseconds { 200 });
}

void XmlSerializationTest::test_fromXml_legacyAutoNoteOffOffset_shouldLoadAsMilliseconds()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);

    auto instrumentSettingsOut = std::make_shared<InstrumentSettings>();
    instrumentSettingsOut->timing.autoNoteOffOffset = AutoNoteOffOffset { std::chrono::milliseconds { 666 } };
    editorServiceOut.setInstrumentSettingsAtCurrentPosition(instrumentSettingsOut);

    // Strip everything sync mode added, leaving the milliseconds attribute a pre-sync project had.
    auto xml = editorServiceOut.toXml();
    xml.remove(QRegularExpression { " autoNoteOffSync[A-Za-z]*=\"[^\"]*\"" });
    QVERIFY(xml.contains(Constants::NahdXml::xmlKeyAutoNoteOffOffset()));
    QVERIFY(!xml.contains(Constants::NahdXml::xmlKeyAutoNoteOffSyncEnabled()));

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);
    editorServiceIn.requestPosition(0, 0, 0, 0, 0);

    const auto instrumentSettingsIn = editorServiceIn.instrumentSettingsAtCurrentPosition();
    QVERIFY(instrumentSettingsIn);
    QVERIFY(instrumentSettingsIn->timing.autoNoteOffOffset.has_value());
    QVERIFY(!instrumentSettingsIn->timing.autoNoteOffOffset->syncEnabled());
    QCOMPARE(instrumentSettingsIn->timing.autoNoteOffOffset->milliseconds(), std::chrono::milliseconds { 666 });
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

void XmlSerializationTest::test_toXmlFromXml_columnDeleted_shouldLoadColumnIndices()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    const auto songOut = editorServiceOut.song();
    songOut->addColumn(0);
    songOut->addColumn(0);
    const Position deletedColumnPosition = { 0, 0, 1, 0, 0 };
    songOut->noteDataAtPosition(deletedColumnPosition)->setAsNoteOn(60, 100);
    const Position lastColumnPosition = { 0, 0, 2, 0, 0 };
    songOut->noteDataAtPosition(lastColumnPosition)->setAsNoteOn(64, 100);
    QVERIFY(songOut->deleteColumn(0, 1));

    // Rendering for playback walks every column of the track and used to trip over the gap
    songOut->renderToEvents(std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<SideChainService>(), 0);

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);
    const auto songIn = editorServiceIn.song();

    QCOMPARE(songIn->columnIndices(0), Song::ColumnIndexList({ 0, 2 }));
    QCOMPARE(songIn->noteDataAtPosition(lastColumnPosition)->note(), std::optional<uint8_t> { 64 });
    songIn->renderToEvents(std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<SideChainService>(), 0);

    // The soft delete survives the save: adding a column back brings its data with it
    songIn->addColumn(0);
    QCOMPARE(songIn->columnIndices(0), Song::ColumnIndexList({ 0, 2, 1 }));
    QCOMPARE(songIn->noteDataAtPosition(deletedColumnPosition)->note(), std::optional<uint8_t> { 60 });
}

void XmlSerializationTest::test_toXmlFromXml_columnMoved_shouldLoadColumnOrder()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    const auto songOut = editorServiceOut.song();
    songOut->addColumn(0);
    songOut->addColumn(0);
    QVERIFY(songOut->moveColumnLeft(0, 2));

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.song()->columnIndices(0), Song::ColumnIndexList({ 0, 2, 1 }));
}

void XmlSerializationTest::test_toXmlFromXml_columnInserted_shouldLoadColumnOrder()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    const auto songOut = editorServiceOut.song();
    songOut->addColumn(0);
    QVERIFY(songOut->addColumnToLeftOf(0, 1));
    QCOMPARE(songOut->columnIndices(0), Song::ColumnIndexList({ 0, 2, 1 }));

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.song()->columnIndices(0), Song::ColumnIndexList({ 0, 2, 1 }));
}

void XmlSerializationTest::test_toXmlFromXml_trackMoved_shouldLoadTrackOrder()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    const auto songOut = editorServiceOut.song();
    const auto trackIndices = songOut->trackIndices();
    QVERIFY(songOut->moveTrackLeft(trackIndices.at(2)));
    QVERIFY(songOut->moveTrackRight(trackIndices.back()));

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.song()->trackIndices(), songOut->trackIndices());
}

void XmlSerializationTest::test_toXml_noColumnsDeleted_shouldNotWriteColumnIndices()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.song()->addColumn(0);

    // A project with the plain column order has to serialize exactly as it did before the attribute existed
    QVERIFY(!editorServiceOut.toXml().contains(Constants::NahdXml::xmlKeyColumnIndices()));
}

void XmlSerializationTest::test_fromXml_legacyNoColumnIndices_shouldLoadConsecutiveIndices()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.song()->addColumn(0);
    editorServiceOut.song()->addColumn(0);

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);

    QCOMPARE(editorServiceIn.song()->columnIndices(0), Song::ColumnIndexList({ 0, 1, 2 }));
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
                // Only an enabled automation counts as present, which is what keeps a disabled one
                // from colouring its column. The round trip itself is asserted just below.
                const bool enabled = track % 2 == 0;
                QCOMPARE(automationServiceIn.hasAutomations(pattern, track, column, line0), enabled);
                QCOMPARE(automationServiceIn.hasAutomations(pattern, track, column, line1), enabled);
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

void XmlSerializationTest::test_toXmlFromXml_automationService_midiCc_curve_shouldLoadAutomationService()
{
    AutomationService automationServiceOut { std::make_shared<PropertyService>() };
    automationServiceOut.addMidiCcAutomation(0, 0, 0, 64, 0, 8, 0, 100, {}, true, 8, 0);
    auto automationOut = automationServiceOut.midiCcAutomations().at(0);
    auto interpolationOut = automationOut.interpolation();
    interpolationOut.curve = Interpolator::CurveType::EaseInOut;
    automationOut.setInterpolation(interpolationOut);
    automationServiceOut.updateMidiCcAutomation(automationOut);

    AutomationService automationServiceIn { std::make_shared<PropertyService>() };
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorService, &EditorService::automationSerializationRequested, &automationServiceOut, &AutomationService::serializeToXml);
    connect(&editorService, &EditorService::automationDeserializationRequested, &automationServiceIn, &AutomationService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    QCOMPARE(automationServiceIn.midiCcAutomations().at(0).interpolation().curve, Interpolator::CurveType::EaseInOut);
}

void XmlSerializationTest::test_toXmlFromXml_automationService_midiCc_linearCurve_shouldNotWriteCurveAttribute()
{
    // A linear automation must stay on the pre-curve XML shape, so that projects written before
    // curves existed keep loading exactly as they did and re-saving them does not add anything
    AutomationService automationServiceOut { std::make_shared<PropertyService>() };
    automationServiceOut.addMidiCcAutomation(0, 0, 0, 64, 0, 8, 0, 100, {}, true, 8, 0);

    AutomationService automationServiceIn { std::make_shared<PropertyService>() };
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorService, &EditorService::automationSerializationRequested, &automationServiceOut, &AutomationService::serializeToXml);
    connect(&editorService, &EditorService::automationDeserializationRequested, &automationServiceIn, &AutomationService::deserializeFromXml);

    const auto xml = editorService.toXml();
    QVERIFY(!xml.contains("curve="));

    // The same XML an older Noteahead would have written loads back as linear
    editorService.fromXml(xml);

    QCOMPARE(automationServiceIn.midiCcAutomations().at(0).interpolation().curve, Interpolator::CurveType::Linear);
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

void XmlSerializationTest::test_toXmlFromXml_automationService_pitchBend_curve_shouldLoadAutomationService()
{
    AutomationService automationServiceOut { std::make_shared<PropertyService>() };
    automationServiceOut.addPitchBendAutomation(0, 0, 0, 0, 8, -100, 100, {}, true);
    auto automationOut = automationServiceOut.pitchBendAutomations().at(0);
    auto interpolationOut = automationOut.interpolation();
    interpolationOut.curve = Interpolator::CurveType::Exponential;
    automationOut.setInterpolation(interpolationOut);
    automationServiceOut.updatePitchBendAutomation(automationOut);

    AutomationService automationServiceIn { std::make_shared<PropertyService>() };
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorService, &EditorService::automationSerializationRequested, &automationServiceOut, &AutomationService::serializeToXml);
    connect(&editorService, &EditorService::automationDeserializationRequested, &automationServiceIn, &AutomationService::deserializeFromXml);

    editorService.fromXml(editorService.toXml());

    QCOMPARE(automationServiceIn.pitchBendAutomations().at(0).interpolation().curve, Interpolator::CurveType::Exponential);
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
                // Only an enabled automation counts as present, which is what keeps a disabled one
                // from colouring its column. The round trip itself is asserted just below.
                const bool enabled = track % 2 == 0;
                QCOMPARE(automationServiceIn.hasAutomations(pattern, track, column, line0), enabled);
                QCOMPARE(automationServiceIn.hasAutomations(pattern, track, column, line1), enabled);
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
    connect(&mixerServiceOut, &MixerService::columnIndicesOfTrackRequested, this, [&](auto && trackIndex) {
        mixerServiceOut.setColumnIndices(trackIndex, { 0, 1, 2 });
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
    instrumentSettingsOut->timing.autoNoteOffOffset = AutoNoteOffOffset { std::chrono::milliseconds { 666 } };
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

void XmlSerializationTest::test_toXmlFromXml_noteData_pan_shouldSaveAndLoadPan()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(1, 3, 64);
    const auto noteData = editorServiceOut.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 });
    QVERIFY(noteData);
    noteData->setPan(42);

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);
    const auto loaded = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 });
    QVERIFY(loaded);
    QVERIFY(loaded->pan().has_value());
    QCOMPARE(*loaded->pan(), 42);
}

void XmlSerializationTest::test_toXmlFromXml_noteData_pan_absent_shouldDefaultToNullopt()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    editorServiceOut.requestNoteOnAtCurrentPosition(1, 3, 64);

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);
    const auto loaded = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 });
    QVERIFY(loaded);
    QVERIFY(!loaded->pan().has_value());
}

void XmlSerializationTest::test_toXmlFromXml_noteData_pan_panOnly_shouldSaveAndLoad()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);
    const auto noteData = editorServiceOut.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 });
    QVERIFY(noteData);
    QCOMPARE(noteData->type(), NoteData::Type::None);
    noteData->setPan(99);

    const auto xml = editorServiceOut.toXml();
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(xml);
    const auto loaded = editorServiceIn.song()->noteDataAtPosition({ 0, 0, 0, 0, 0 });
    QVERIFY(loaded);
    QCOMPARE(loaded->type(), NoteData::Type::None);
    QVERIFY(loaded->pan().has_value());
    QCOMPARE(*loaded->pan(), 99);
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

void XmlSerializationTest::test_toXmlFromXml_instrumentSettings_syncedAutoNoteOff_shouldRoundTrip()
{
    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceOut.requestPosition(0, 0, 0, 0, 0);

    auto instrumentSettingsOut = std::make_shared<InstrumentSettings>();
    instrumentSettingsOut->timing.autoNoteOffOffset = AutoNoteOffOffset { 32 };
    editorServiceOut.setInstrumentSettingsAtCurrentPosition(instrumentSettingsOut);

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    editorServiceIn.fromXml(editorServiceOut.toXml());
    editorServiceIn.requestPosition(0, 0, 0, 0, 0);

    const auto instrumentSettingsIn = editorServiceIn.instrumentSettingsAtCurrentPosition();
    QVERIFY(instrumentSettingsIn);
    QCOMPARE(instrumentSettingsIn->timing.autoNoteOffOffset, instrumentSettingsOut->timing.autoNoteOffOffset);
    QVERIFY(instrumentSettingsIn->timing.autoNoteOffOffset->syncEnabled());
    QCOMPARE(instrumentSettingsIn->timing.autoNoteOffOffset->syncDenominator(), 32);
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

void XmlSerializationTest::test_toXmlFromXml_samplerDevice_padLoopAndChokeGroup_shouldRoundTrip()
{
    const std::string fileName = "test.wav";
    const auto samplerName = "Noteahead Internal Device 1";

    DeviceService deviceServiceOut { std::make_shared<AudioEngine>(), std::make_shared<DataService>() };
    const auto samplerOut = std::make_shared<SamplerDevice>(samplerName, std::make_unique<MockAudioFileReader>());
    samplerOut->loadSample(60, fileName);
    samplerOut->setSampleLoop(60, true);
    samplerOut->setSampleChokeGroup(60, 5);
    samplerOut->loadSample(62, fileName);
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

    QCOMPARE(samplerIn->sampleLoop(60), true);
    QCOMPARE(samplerIn->sampleChokeGroup(60), 5);

    // An untouched pad neither loops nor belongs to a group, which is how every pad saved before these
    // settings existed has to come back.
    QCOMPARE(samplerIn->sampleLoop(62), false);
    QCOMPARE(samplerIn->sampleChokeGroup(62), 0);
}

void XmlSerializationTest::test_toXmlFromXml_samplerDevice_padTuningTrimAndEnvelope_shouldRoundTrip()
{
    const std::string fileName = "test.wav";
    const auto samplerName = "Noteahead Internal Device 1";

    DeviceService deviceServiceOut { std::make_shared<AudioEngine>(), std::make_shared<DataService>() };
    const auto samplerOut = std::make_shared<SamplerDevice>(samplerName, std::make_unique<MockAudioFileReader>());
    samplerOut->loadSample(60, fileName);
    samplerOut->setSampleTune(60, 0.75f);
    samplerOut->setSampleDetune(60, 0.25f);
    samplerOut->setSampleAttack(60, 0.2f);
    samplerOut->setSampleDecay(60, 0.3f);
    samplerOut->setSampleSustain(60, 0.4f);
    samplerOut->setSampleRelease(60, 0.6f);
    samplerOut->setSampleReverse(60, true);
    samplerOut->setSampleEndOffset(60, 0.01);
    // The pad next door keeps the defaults, so the round trip also covers the case that has to keep
    // playing the way it did before any of these settings existed.
    samplerOut->loadSample(62, fileName);
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

    QCOMPARE(SamplerDevice::tuneSemitones(samplerIn->sampleTune(60)), 12);
    QVERIFY(std::abs(SamplerDevice::detuneCents(samplerIn->sampleDetune(60)) + 50.0) < 0.5);
    QVERIFY(std::abs(samplerIn->sampleAttack(60) - 0.2f) < 0.001f);
    QVERIFY(std::abs(samplerIn->sampleDecay(60) - 0.3f) < 0.001f);
    QVERIFY(std::abs(samplerIn->sampleSustain(60) - 0.4f) < 0.001f);
    QVERIFY(std::abs(samplerIn->sampleRelease(60) - 0.6f) < 0.001f);
    QCOMPARE(samplerIn->sampleReverse(60), true);
    QVERIFY(samplerIn->sampleEndOffset(60).has_value());
    QVERIFY(std::abs(*samplerIn->sampleEndOffset(60) - 0.01) < 0.001);

    // An untouched pad comes back untouched: unity tuning, no trim, no reverse, full sustain.
    QCOMPARE(SamplerDevice::tuneSemitones(samplerIn->sampleTune(62)), 0);
    QCOMPARE(samplerIn->sampleDetune(62), 0.5f);
    QCOMPARE(samplerIn->sampleAttack(62), 0.0f);
    QCOMPARE(samplerIn->sampleSustain(62), 1.0f);
    QCOMPARE(samplerIn->sampleRelease(62), 0.0f);
    QCOMPARE(samplerIn->sampleReverse(62), false);
    QVERIFY(!samplerIn->sampleEndOffset(62).has_value());
}

void XmlSerializationTest::test_toXmlFromXml_samplerDevice_padEffectRack_shouldRoundTrip()
{
    const std::string fileName = "test.wav";
    const auto samplerName = "Noteahead Internal Device 1";

    DeviceService deviceServiceOut { std::make_shared<AudioEngine>(), std::make_shared<DataService>() };
    const auto samplerOut = std::make_shared<SamplerDevice>(samplerName, std::make_unique<MockAudioFileReader>());
    samplerOut->loadSample(60, fileName);
    const auto reverbOut = std::make_shared<Reverb>();
    reverbOut->setEnabled(false);
    samplerOut->sampleEffectRack(60).setEffect(0, reverbOut);
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

    QVERIFY(samplerIn->sample(60));
    const auto effect = samplerIn->sampleEffectRack(60).effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), reverbOut->typeId());
    QVERIFY(!effect->enabled());
    // A pad without an added effect must not gain a phantom rack entry.
    QVERIFY(!samplerIn->sampleEffectRack(61).hasEffects());
}

void XmlSerializationTest::test_toXmlFromXml_drumSynthDevice_voiceEffectRack_shouldRoundTrip()
{
    const auto drumName = "Noteahead Internal Device 1";

    DeviceService deviceServiceOut { std::make_shared<AudioEngine>(), std::make_shared<DataService>() };
    const auto drumOut = std::make_shared<DrumSynthDevice>(drumName);
    const auto reverbOut = std::make_shared<Reverb>();
    reverbOut->setEnabled(false);
    drumOut->voiceEffectRack(1).setEffect(0, reverbOut);
    deviceServiceOut.setDevice(0, drumOut);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto deviceServiceIn = std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>());
    const auto drumIn = std::make_shared<DrumSynthDevice>(drumName);
    deviceServiceIn->setDevice(0, drumIn);

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, deviceServiceIn.get(), &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = drumIn->voiceEffectRack(1).effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), reverbOut->typeId());
    QVERIFY(!effect->enabled());
    // A voice without an added effect must not gain one.
    QVERIFY(!drumIn->voiceEffectRack(0).hasEffects());
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

void XmlSerializationTest::test_toXmlFromXml_samplerDevice_saveAs_shouldPreserveEmbeddedData()
{
    const std::string projectPath { "/tmp/noteahead_test" };
    const std::string relativePath { "samples/kick.wav" };
    const std::string absolutePath { QDir(QString::fromStdString(projectPath)).absoluteFilePath(QString::fromStdString(relativePath)).toStdString() };

    // Create the dummy directory and file on disk so serializeDataToXml can open it
    QDir().mkpath(QFileInfo(QString::fromStdString(absolutePath)).absolutePath());
    QFile dummyFile { QString::fromStdString(absolutePath) };
    QVERIFY(dummyFile.open(QIODevice::WriteOnly));
    dummyFile.write("dummy-wav-data");
    dummyFile.close();

    const auto samplerName = "Noteahead Internal Device 1";

    const auto engine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    DeviceService deviceServiceOut { engine, dataService };
    deviceServiceOut.setProjectPath(projectPath);

    const auto samplerOut = std::make_shared<SamplerDevice>(samplerName, std::make_unique<MockAudioFileReader>());
    samplerOut->loadSample(60, absolutePath);
    samplerOut->setEmbedWaveData(true);
    deviceServiceOut.setDevice(0, samplerOut);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), dataService };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);
    connect(&editorServiceOut, &EditorService::dataSerializationRequested, [&deviceServiceOut, dataService](ProjectWriter & writer) {
        const auto files = deviceServiceOut.getFilesToEmbed();
        dataService->serializeDataToXml(writer, files);
    });

    const auto xml = editorServiceOut.toXml();

    // Now load it in a new setup
    const auto engine2 = std::make_shared<AudioEngine>();
    const auto dataService2 = std::make_shared<DataService>();
    DeviceService deviceServiceIn { engine2, dataService2 };
    deviceServiceIn.setProjectPath(projectPath);

    const auto samplerIn = std::make_shared<SamplerDevice>(samplerName, std::make_unique<MockAudioFileReader>());
    deviceServiceIn.setDevice(0, samplerIn);

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), dataService2 };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);
    connect(&editorServiceIn, &EditorService::devicesSerializationRequested, &deviceServiceIn, &DeviceService::serializeToXml);
    connect(&editorServiceIn, &EditorService::dataSerializationRequested, [&deviceServiceIn, dataService2](ProjectWriter & writer) {
        const auto files = deviceServiceIn.getFilesToEmbed();
        dataService2->serializeDataToXml(writer, files);
    });

    editorServiceIn.fromXml(xml);

    // Verify it is loaded and its path in memory is absolute (our fix!)
    QVERIFY(samplerIn->sample(60));
    const auto expectedMemoryPath = samplerIn->sample(60)->filePath;
    // It should start with nahd:// because it was deserialized as embedded
    QVERIFY(QString::fromStdString(expectedMemoryPath).startsWith(Constants::NahdXml::embeddedDataPathPrefix()));

    // Now simulate "Save As" by changing project path to a new location
    const std::string newProjectPath { "/tmp/noteahead_test_new" };
    deviceServiceIn.setProjectPath(newProjectPath);

    // Serialize again! If the bug exists, this will fail or serialize empty data
    // because absoluteFilePath(60) would be resolved relative to the new path and point to a non-existent file
    const auto xml2 = editorServiceIn.toXml();

    // Verify the second XML has the embedded data block
    QVERIFY(xml2.contains("<Data"));
    QVERIFY(xml2.contains("nahd://kick.wav"));

    // Cleanup
    QFile::remove(QString::fromStdString(absolutePath));
    QDir().rmdir(QFileInfo(QString::fromStdString(absolutePath)).absolutePath());
}

void XmlSerializationTest::test_toXml_whileAutomated_shouldSaveAuthoredValues()
{
    // Saving while a song plays is a normal thing to do -- it sounds good, so it gets saved. What
    // the automation happens to be doing at that moment is not part of the patch and must not end
    // up in the file.
    const auto deviceName = "Noteahead Internal Device 1";

    DeviceService deviceServiceOut { std::make_shared<AudioEngine>(), std::make_shared<DataService>() };
    const auto synthOut = std::make_shared<SynthDevice>(deviceName);
    synthOut->setLpfCutoff(0.3f);
    synthOut->setVco1Roundness(0.8f);
    synthOut->setVco2Roundness(0.2f);
    synthOut->setPan(0.25f);
    deviceServiceOut.setDevice(0, synthOut);

    const auto kickHpfKey = DrumSynth::voiceId(static_cast<int>(DrumSynth::VoiceIndex::Kick)) + "_" + Constants::NahdXml::xmlKeyHpfCutoff().toStdString();
    const auto drumOut = std::make_shared<DrumSynthDevice>("Noteahead Internal Device 2");
    drumOut->updateVoiceParameter(0, Constants::NahdXml::xmlKeyHpfCutoff().toStdString(), 0.25f);
    deviceServiceOut.setDevice(1, drumOut);

    // Automation runs over all of them
    synthOut->processMidiCc(74, 127, 0); // Cutoff
    synthOut->processMidiCc(10, 127, 0); // Pan
    drumOut->processMidiCc(16, 127, 0); // Kick HPF
    QCOMPARE(synthOut->lpfCutoff(), 1.0f);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto deviceServiceIn = std::make_shared<DeviceService>(std::make_shared<AudioEngine>(), std::make_shared<DataService>());
    const auto synthIn = std::make_shared<SynthDevice>(deviceName);
    deviceServiceIn->setDevice(0, synthIn);
    const auto drumIn = std::make_shared<DrumSynthDevice>("Noteahead Internal Device 2");
    deviceServiceIn->setDevice(1, drumIn);

    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, deviceServiceIn.get(), &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    QCOMPARE(synthIn->lpfCutoff(), 0.3f);
    QCOMPARE(synthIn->vco1Roundness(), 0.8f);
    QCOMPARE(synthIn->vco2Roundness(), 0.2f);
    QCOMPARE(synthIn->pan(), 0.25f);
    QCOMPARE(drumIn->parameter(kickHpfKey)->get().value(), 0.25f);
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
    synthOut->setAmpCurve(0.65f);
    synthOut->setModCurve(0.35f);
    synthOut->setMultiType(MultiEngine::Type::Decim);
    synthOut->setMultiShape(0.42f);
    synthOut->setMultiLevel(0.88f);
    synthOut->setMultiKeyTrack(0.5f);
    synthOut->setPan(0.12f);
    synthOut->setDelayType(Delay::Type::PingPong);
    synthOut->setDelaySync(true);
    synthOut->setDelaySyncDivision(0.25f);
    synthOut->setFeedbackLpf(0.6f);
    synthOut->setFeedbackHpf(0.2f);
    synthOut->setLfoWaveform(Lfo::Waveform::Saw);
    synthOut->setLfoMode(Lfo::Mode::OneShot);
    synthOut->setLfoRate(0.4f);
    synthOut->setLfoInt(0.6f);
    // The per-oscillator targets are the highest ordinals, so they are what proves the parameter
    // range is wide enough to store every destination the UI offers.
    synthOut->setLfoTarget(SynthDevice::LfoTarget::Pitch2);
    synthOut->setLfo2Waveform(Lfo::Waveform::Square);
    synthOut->setLfo2Mode(Lfo::Mode::OneShot);
    synthOut->setLfo2Rate(0.3f);
    synthOut->setLfo2Int(0.7f);
    synthOut->setLfo2Target(SynthDevice::LfoTarget::Pitch3);
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
    QCOMPARE(synthIn->ampCurve(), 0.65f);
    QCOMPARE(synthIn->modCurve(), 0.35f);
    QCOMPARE(synthIn->multiType(), MultiEngine::Type::Decim);
    QCOMPARE(synthIn->multiShape(), 0.42f);
    QCOMPARE(synthIn->multiLevel(), 0.88f);
    QCOMPARE(synthIn->multiKeyTrack(), 0.5f);
    QCOMPARE(synthIn->pan(), 0.12f);
    QCOMPARE(synthIn->delayType(), Delay::Type::PingPong);
    QCOMPARE(synthIn->delaySync(), true);
    QCOMPARE(synthIn->delaySyncDivision(), 0.25f);
    QCOMPARE(synthIn->delayFeedbackLpf(), 0.6f);
    QCOMPARE(synthIn->delayFeedbackHpf(), 0.2f);
    QCOMPARE(synthIn->lfoWaveform(), Lfo::Waveform::Saw);
    QCOMPARE(synthIn->lfoMode(), Lfo::Mode::OneShot);
    QCOMPARE(synthIn->lfoRate(), 0.4f);
    QCOMPARE(synthIn->lfoTarget(), SynthDevice::LfoTarget::Pitch2);
    QCOMPARE(synthIn->lfo2Waveform(), Lfo::Waveform::Square);
    QCOMPARE(synthIn->lfo2Mode(), Lfo::Mode::OneShot);
    QCOMPARE(synthIn->lfo2Rate(), 0.3f);
    QCOMPARE(synthIn->lfo2Target(), SynthDevice::LfoTarget::Pitch3);

    // Verify discrete flags
    const auto vco1Wave = synthIn->parameter(Constants::NahdXml::xmlKeyVco1Waveform().toStdString());
    QVERIFY(vco1Wave.has_value());
    QVERIFY(vco1Wave->get().isDiscrete());

    const auto vco1Octave = synthIn->parameter(Constants::NahdXml::xmlKeyVco1Octave().toStdString());
    QVERIFY(vco1Octave.has_value());
    QVERIFY(vco1Octave->get().isDiscrete());

    const auto lpfCutoff = synthIn->parameter(Constants::NahdXml::xmlKeyLpfCutoff().toStdString());
    QVERIFY(lpfCutoff.has_value());
    QVERIFY(!lpfCutoff->get().isDiscrete());

    const auto multiShape = synthIn->parameter(Constants::NahdXml::xmlKeyMultiShape().toStdString());
    QVERIFY(multiShape.has_value());
    QVERIFY(!multiShape->get().isDiscrete());

    const auto lfoWaveform = synthIn->parameter(Constants::NahdXml::xmlKeyLfoWaveform().toStdString());
    QVERIFY(lfoWaveform.has_value());
    QVERIFY(lfoWaveform->get().isDiscrete());

    const auto lfoMode = synthIn->parameter(Constants::NahdXml::xmlKeyLfoMode().toStdString());
    QVERIFY(lfoMode.has_value());
    QVERIFY(lfoMode->get().isDiscrete());

    const auto lfoRate = synthIn->parameter(Constants::NahdXml::xmlKeyLfoRate().toStdString());
    QVERIFY(lfoRate.has_value());
    QVERIFY(!lfoRate->get().isDiscrete());

    const auto lfoIntensity = synthIn->parameter(Constants::NahdXml::xmlKeyLfoIntensity().toStdString());
    QVERIFY(lfoIntensity.has_value());
    QCOMPARE(lfoIntensity->get().value(), 0.6f);
    QVERIFY(!lfoIntensity->get().isDiscrete());

    const auto lfoTarget = synthIn->parameter(Constants::NahdXml::xmlKeyLfoTarget().toStdString());
    QVERIFY(lfoTarget.has_value());
    QVERIFY(lfoTarget->get().isDiscrete());

    const auto lfo2Waveform = synthIn->parameter(Constants::NahdXml::xmlKeyLfo2Waveform().toStdString());
    QVERIFY(lfo2Waveform.has_value());
    QVERIFY(lfo2Waveform->get().isDiscrete());

    const auto lfo2Mode = synthIn->parameter(Constants::NahdXml::xmlKeyLfo2Mode().toStdString());
    QVERIFY(lfo2Mode.has_value());
    QVERIFY(lfo2Mode->get().isDiscrete());

    const auto lfo2Rate = synthIn->parameter(Constants::NahdXml::xmlKeyLfo2Rate().toStdString());
    QVERIFY(lfo2Rate.has_value());
    QVERIFY(!lfo2Rate->get().isDiscrete());

    const auto lfo2Intensity = synthIn->parameter(Constants::NahdXml::xmlKeyLfo2Intensity().toStdString());
    QVERIFY(lfo2Intensity.has_value());
    QCOMPARE(lfo2Intensity->get().value(), 0.7f);
    QVERIFY(!lfo2Intensity->get().isDiscrete());

    const auto lfo2Target = synthIn->parameter(Constants::NahdXml::xmlKeyLfo2Target().toStdString());
    QVERIFY(lfo2Target.has_value());
    QVERIFY(lfo2Target->get().isDiscrete());
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
    preset.parameters[Constants::NahdXml::xmlKeyLpfCutoff().toStdString()] = 0.5f;
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
    QCOMPARE(userPresetsIn.at(10).parameters.at(Constants::NahdXml::xmlKeyLpfCutoff().toStdString()), 0.5f);
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
    preset.parameters[Constants::NahdXml::xmlKeyVco1Waveform().toStdString()] = 1.0f;
    // Pitch: 0 cents
    preset.parameters[Constants::NahdXml::xmlKeyVco1Pitch().toStdString()] = 0.0f;
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
    QCOMPARE(userPresetsIn.at(5).parameters.at(Constants::NahdXml::xmlKeyVco1Waveform().toStdString()), 1.0f);
    QCOMPARE(userPresetsIn.at(5).parameters.at(Constants::NahdXml::xmlKeyVco1Pitch().toStdString()), 0.0f);
}

void XmlSerializationTest::test_toXmlFromXml_masterSendEffects_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    // Add a reverb effect to slot 0 of master send rack
    auto reverb = std::make_shared<Reverb>();
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
    QCOMPARE(effect->typeId(), Reverb::typeIdString());
    auto restoredReverb = std::dynamic_pointer_cast<Reverb>(effect);
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
    auto chorus = std::make_shared<Chorus>();
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
    QCOMPARE(effect->typeId(), Chorus::typeIdString());
    auto restoredChorus = std::dynamic_pointer_cast<Chorus>(effect);
    QVERIFY(restoredChorus);
    QCOMPARE(restoredChorus->rate(), 0.5f);
    QCOMPARE(restoredChorus->depth(), 0.6f);
    QCOMPARE(restoredChorus->delay(), 0.4f);
    QCOMPARE(restoredChorus->mix(), 0.7f);
    QCOMPARE(restoredChorus->width(), 0.8f);
    QCOMPARE(restoredChorus->lpfCutoff(), 0.9f);
    QCOMPARE(restoredChorus->hpfCutoff(), 0.1f);
}

void XmlSerializationTest::test_toXmlFromXml_endlessReverbEffect_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto endless = std::make_shared<EndlessReverb>();
    if (auto p = endless->parameter(Constants::NahdXml::xmlKeySize().toStdString()); p) {
        p->get().setValue(0.8f);
    }
    if (auto p = endless->parameter(Constants::NahdXml::xmlKeyFreeze().toStdString()); p) {
        p->get().setValue(1.0f);
    }
    deviceServiceOut.sendEffectRack().setEffect(0, endless);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), EndlessReverb::typeIdString());
    const auto restored = std::dynamic_pointer_cast<EndlessReverb>(effect);
    QVERIFY(restored);
    if (auto p = restored->parameter(Constants::NahdXml::xmlKeySize().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.8f);
    }
    if (auto p = restored->parameter(Constants::NahdXml::xmlKeyFreeze().toStdString()); p) {
        QCOMPARE(p->get().value(), 1.0f);
    }
}

void XmlSerializationTest::test_toXmlFromXml_vintagePassiveEqEffect_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto eq = std::make_shared<VintagePassiveEq>();
    const auto setParam = [&eq](const QString & key, float value) {
        if (auto p = eq->parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    setParam(Constants::NahdXml::xmlKeyLowFreq(), 3.0f); // 100 Hz
    setParam(Constants::NahdXml::xmlKeyLowBoost(), 0.6f);
    setParam(Constants::NahdXml::xmlKeyLowAtten(), 0.3f);
    setParam(Constants::NahdXml::xmlKeyHighBoostFreq(), 4.0f); // 10 kHz
    setParam(Constants::NahdXml::xmlKeyHighBoost(), 0.7f);
    setParam(Constants::NahdXml::xmlKeyBandwidth(), 0.4f);
    setParam(Constants::NahdXml::xmlKeyHighAttenFreq(), 2.0f); // 20 kHz
    setParam(Constants::NahdXml::xmlKeyHighAtten(), 0.5f);
    deviceServiceOut.sendEffectRack().setEffect(0, eq);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), VintagePassiveEq::typeIdString());
    const auto restored = std::dynamic_pointer_cast<VintagePassiveEq>(effect);
    QVERIFY(restored);
    const auto valueOf = [&restored](const QString & key) {
        const auto p = restored->parameter(key.toStdString());
        return p ? p->get().value() : -1.0f;
    };
    QCOMPARE(valueOf(Constants::NahdXml::xmlKeyLowFreq()), 3.0f);
    QCOMPARE(valueOf(Constants::NahdXml::xmlKeyLowBoost()), 0.6f);
    QCOMPARE(valueOf(Constants::NahdXml::xmlKeyLowAtten()), 0.3f);
    QCOMPARE(valueOf(Constants::NahdXml::xmlKeyHighBoostFreq()), 4.0f);
    QCOMPARE(valueOf(Constants::NahdXml::xmlKeyHighBoost()), 0.7f);
    QCOMPARE(valueOf(Constants::NahdXml::xmlKeyBandwidth()), 0.4f);
    QCOMPARE(valueOf(Constants::NahdXml::xmlKeyHighAttenFreq()), 2.0f);
    QCOMPARE(valueOf(Constants::NahdXml::xmlKeyHighAtten()), 0.5f);
}

void XmlSerializationTest::test_toXmlFromXml_simpleEqEffect_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto eq = std::make_shared<SimpleEq>();
    if (auto p = eq->parameter(Constants::NahdXml::xmlKeyAmount().toStdString()); p) {
        p->get().setValue(0.65f);
    }
    deviceServiceOut.sendEffectRack().setEffect(0, eq);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), SimpleEq::typeIdString());
    const auto restored = std::dynamic_pointer_cast<SimpleEq>(effect);
    QVERIFY(restored);
    if (const auto p = restored->parameter(Constants::NahdXml::xmlKeyAmount().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.65f);
    }
}

void XmlSerializationTest::test_toXmlFromXml_airBandEqEffect_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto eq = std::make_shared<AirBandEq>();
    const auto setParam = [&eq](const QString & key, float value) {
        if (auto p = eq->parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    for (size_t i = 0; i < AirBandEq::BandCount; i++) {
        setParam(Constants::NahdXml::xmlKeyBandGain(i), 0.1f * static_cast<float>(i + 1));
    }
    setParam(Constants::NahdXml::xmlKeyAirFreq(), 4.0f); // 20 kHz
    setParam(Constants::NahdXml::xmlKeyAirGain(), 0.65f);
    setParam(Constants::NahdXml::xmlKeyGain(), 0.4f);
    deviceServiceOut.sendEffectRack().setEffect(0, eq);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), AirBandEq::typeIdString());
    const auto restored = std::dynamic_pointer_cast<AirBandEq>(effect);
    QVERIFY(restored);
    const auto valueOf = [&restored](const QString & key) {
        const auto p = restored->parameter(key.toStdString());
        return p ? p->get().value() : -1.0f;
    };
    for (size_t i = 0; i < AirBandEq::BandCount; i++) {
        QVERIFY(std::abs(valueOf(Constants::NahdXml::xmlKeyBandGain(i)) - 0.1f * static_cast<float>(i + 1)) < 0.001f);
    }
    QCOMPARE(valueOf(Constants::NahdXml::xmlKeyAirFreq()), 4.0f);
    QVERIFY(std::abs(valueOf(Constants::NahdXml::xmlKeyAirGain()) - 0.65f) < 0.001f);
    QVERIFY(std::abs(valueOf(Constants::NahdXml::xmlKeyGain()) - 0.4f) < 0.001f);
}

void XmlSerializationTest::test_toXmlFromXml_subMixerDevice_shouldLoadCorrectly()
{
    // Unlike effects, devices are rebuilt through DeviceFactory, which this suite does not
    // otherwise need and so never initialises.
    DeviceFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto subMixer = std::make_shared<SubMixerDevice>("SubMixer");
    subMixer->setMembers({ 2, 5, 7 });
    subMixer->setVolume(0.8f);
    subMixer->setPan(0.3f);
    deviceServiceOut.setDevice(1, subMixer);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto restored = std::dynamic_pointer_cast<SubMixerDevice>(deviceServiceIn.device(size_t { 1 }));
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), SubMixerDevice::typeIdString());

    const auto members = restored->members();
    QCOMPARE(members.size(), size_t { 3 });
    QCOMPARE(members[0], size_t { 2 });
    QCOMPARE(members[1], size_t { 5 });
    QCOMPARE(members[2], size_t { 7 });

    QVERIFY(std::abs(restored->volume() - 0.8f) < 0.001f);
    QVERIFY(std::abs(restored->pan() - 0.3f) < 0.001f);
}

void XmlSerializationTest::test_toXmlFromXml_stringEnsembleDevice_shouldLoadCorrectly()
{
    // Devices are rebuilt through DeviceFactory, so this also covers the factory registration:
    // without it the device would silently vanish from a reloaded project.
    DeviceFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto stringEnsemble = std::make_shared<StringEnsembleDevice>("StringEnsemble");
    stringEnsemble->setContrabassEnabled(true);
    stringEnsemble->setViolaEnabled(false);
    stringEnsemble->setTrumpetEnabled(true);
    stringEnsemble->setModulationEnabled(false);
    stringEnsemble->setPhaserEnabled(true);
    stringEnsemble->setVolumeBass(0.42f);
    stringEnsemble->setCrescendo(0.66f);
    stringEnsemble->setSustainLength(0.77f);
    stringEnsemble->setPhaserColor(0.25f);
    stringEnsemble->setPhaserRate(0.8f);
    stringEnsemble->setVelocitySensitivity(0.3f);
    stringEnsemble->setFaderPosition(Device::FaderPosition::PostInserts);
    stringEnsemble->setSendTap(Device::SendTap::PreFader);
    deviceServiceOut.setDevice(1, stringEnsemble);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto restored = std::dynamic_pointer_cast<StringEnsembleDevice>(deviceServiceIn.device(size_t { 1 }));
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), StringEnsembleDevice::typeIdString());

    QCOMPARE(restored->contrabassEnabled(), true);
    QCOMPARE(restored->celloEnabled(), false);
    QCOMPARE(restored->violaEnabled(), false);
    QCOMPARE(restored->violinEnabled(), true);
    QCOMPARE(restored->trumpetEnabled(), true);
    QCOMPARE(restored->modulationEnabled(), false);
    QCOMPARE(restored->phaserEnabled(), true);
    QVERIFY(std::abs(restored->volumeBass() - 0.42f) < 0.001f);
    QVERIFY(std::abs(restored->crescendo() - 0.66f) < 0.001f);
    QVERIFY(std::abs(restored->sustainLength() - 0.77f) < 0.001f);
    QVERIFY(std::abs(restored->phaserColor() - 0.25f) < 0.001f);
    QVERIFY(std::abs(restored->phaserRate() - 0.8f) < 0.001f);
    QVERIFY(std::abs(restored->velocitySensitivity() - 0.3f) < 0.001f);

    // The channel strip settings live on the Device base class, so this covers them for every device.
    QCOMPARE(static_cast<int>(restored->faderPosition()), static_cast<int>(Device::FaderPosition::PostInserts));
    QCOMPARE(static_cast<int>(restored->sendTap()), static_cast<int>(Device::SendTap::PreFader));
}

void XmlSerializationTest::test_toXmlFromXml_speechDevice_shouldLoadCorrectly()
{
    // Devices are rebuilt through DeviceFactory, so this also covers the factory registration:
    // without it the device would silently vanish from a reloaded project.
    DeviceFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto speech = std::make_shared<SpeechDevice>("Speech");
    // Punctuation, an escape and an ampersand: the phrase is written as an XML attribute, so it is
    // the one setting on any device that can carry characters the format itself cares about.
    speech->setPhrase("hello, /w er l d/ & goodbye");
    speech->setRate(0.75f);
    speech->setGlide(0.2f);
    speech->setFormantShift(0.65f);
    speech->setBreathiness(0.3f);
    speech->setConsonantLevel(0.8f);
    speech->setIntonation(0.55f);
    speech->setVibratoRate(0.45f);
    speech->setVibratoDepth(0.25f);
    speech->setTriggerMode(1);
    speech->setSyncMode(2);
    speech->setSyncLength(12);
    speech->setSyncDivision(3);
    speech->setSibilance(0.42f);
    speech->setVoiceType(1);
    speech->setVelocitySensitivity(0.65f);
    speech->setFaderPosition(Device::FaderPosition::PostInserts);
    deviceServiceOut.setDevice(1, speech);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto restored = std::dynamic_pointer_cast<SpeechDevice>(deviceServiceIn.device(size_t { 1 }));
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), SpeechDevice::typeIdString());

    QCOMPARE(restored->phrase(), std::string { "hello, /w er l d/ & goodbye" });
    // The phrase has to be recompiled on load, not merely stored, or the device restores silent.
    QVERIFY(!restored->phrasePhonemes().empty());
    QCOMPARE(restored->phrasePhonemes(), speech->phrasePhonemes());

    QVERIFY(std::abs(restored->rate() - 0.75f) < 0.001f);
    QVERIFY(std::abs(restored->glide() - 0.2f) < 0.001f);
    QVERIFY(std::abs(restored->formantShift() - 0.65f) < 0.001f);
    QVERIFY(std::abs(restored->breathiness() - 0.3f) < 0.001f);
    QVERIFY(std::abs(restored->consonantLevel() - 0.8f) < 0.001f);
    QVERIFY(std::abs(restored->intonation() - 0.55f) < 0.001f);
    QVERIFY(std::abs(restored->vibratoRate() - 0.45f) < 0.001f);
    QVERIFY(std::abs(restored->vibratoDepth() - 0.25f) < 0.001f);
    QCOMPARE(restored->triggerMode(), 1);
    QCOMPARE(restored->syncMode(), 2);
    QCOMPARE(restored->syncLength(), 12);
    QCOMPARE(restored->syncDivision(), 3);
    QVERIFY(std::abs(restored->sibilance() - 0.42f) < 0.001f);
    QCOMPARE(restored->voiceType(), 1);
    QVERIFY(std::abs(restored->velocitySensitivity() - 0.65f) < 0.001f);

    QCOMPARE(static_cast<int>(restored->faderPosition()), static_cast<int>(Device::FaderPosition::PostInserts));
}

void XmlSerializationTest::test_toXmlFromXml_pianoSynthV2Device_shouldLoadCorrectly()
{
    // Devices are rebuilt through DeviceFactory, so this also covers the factory registration:
    // without it the device would silently vanish from a reloaded project.
    DeviceFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto piano = std::make_shared<PianoSynthV2Device>("PianoSynthV2");
    piano->setBrightness(0.62f);
    piano->setDecay(0.28f);
    piano->setInharmonicity(0.81f);
    piano->setHammerHardness(0.34f);
    piano->setStringDetune(0.19f);
    piano->setStretch(0.55f);
    piano->setRichness(0.88f);
    piano->setDoubleDecay(0.41f);
    piano->setLpfCutoff(0.73f);
    piano->setHpfCutoff(0.12f);
    piano->setReleaseTime(0.26f);
    piano->setStereoWidth(0.37f);
    deviceServiceOut.setDevice(1, piano);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto restored = std::dynamic_pointer_cast<PianoSynthV2Device>(deviceServiceIn.device(size_t { 1 }));
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), PianoSynthV2Device::typeIdString());

    QVERIFY(std::abs(restored->brightness() - 0.62f) < 0.001f);
    QVERIFY(std::abs(restored->decay() - 0.28f) < 0.001f);
    QVERIFY(std::abs(restored->inharmonicity() - 0.81f) < 0.001f);
    QVERIFY(std::abs(restored->hammerHardness() - 0.34f) < 0.001f);
    QVERIFY(std::abs(restored->stringDetune() - 0.19f) < 0.001f);
    QVERIFY(std::abs(restored->stretch() - 0.55f) < 0.001f);
    QVERIFY(std::abs(restored->richness() - 0.88f) < 0.001f);
    QVERIFY(std::abs(restored->doubleDecay() - 0.41f) < 0.001f);
    QVERIFY(std::abs(restored->lpfCutoff() - 0.73f) < 0.001f);
    QVERIFY(std::abs(restored->hpfCutoff() - 0.12f) < 0.001f);
    QVERIFY(std::abs(restored->releaseTime() - 0.26f) < 0.001f);
    QVERIFY(std::abs(restored->stereoWidth() - 0.37f) < 0.001f);
}

void XmlSerializationTest::test_toXmlFromXml_pianoSynthV3Device_shouldLoadCorrectly()
{
    // Devices are rebuilt through DeviceFactory, so this also covers the factory registration:
    // without it the device would silently vanish from a reloaded project.
    DeviceFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto piano = std::make_shared<PianoSynthV3Device>("PianoSynthV3");
    piano->setBrightness(0.62f);
    piano->setDecay(0.28f);
    piano->setInharmonicity(0.81f);
    piano->setHammerHardness(0.34f);
    piano->setStringDetune(0.19f);
    piano->setStretch(0.55f);
    piano->setRichness(0.88f);
    piano->setDoubleDecay(0.41f);
    piano->setLpfCutoff(0.73f);
    piano->setHpfCutoff(0.12f);
    piano->setReleaseTime(0.26f);
    piano->setStereoWidth(0.37f);
    deviceServiceOut.setDevice(1, piano);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto restored = std::dynamic_pointer_cast<PianoSynthV3Device>(deviceServiceIn.device(size_t { 1 }));
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), PianoSynthV3Device::typeIdString());

    QVERIFY(std::abs(restored->brightness() - 0.62f) < 0.001f);
    QVERIFY(std::abs(restored->decay() - 0.28f) < 0.001f);
    QVERIFY(std::abs(restored->inharmonicity() - 0.81f) < 0.001f);
    QVERIFY(std::abs(restored->hammerHardness() - 0.34f) < 0.001f);
    QVERIFY(std::abs(restored->stringDetune() - 0.19f) < 0.001f);
    QVERIFY(std::abs(restored->stretch() - 0.55f) < 0.001f);
    QVERIFY(std::abs(restored->richness() - 0.88f) < 0.001f);
    QVERIFY(std::abs(restored->doubleDecay() - 0.41f) < 0.001f);
    QVERIFY(std::abs(restored->lpfCutoff() - 0.73f) < 0.001f);
    QVERIFY(std::abs(restored->hpfCutoff() - 0.12f) < 0.001f);
    QVERIFY(std::abs(restored->releaseTime() - 0.26f) < 0.001f);
    QVERIFY(std::abs(restored->stereoWidth() - 0.37f) < 0.001f);
}

void XmlSerializationTest::test_toXmlFromXml_kick808Device_shouldLoadCorrectly()
{
    // Devices are rebuilt through DeviceFactory, so this also covers the factory registration:
    // without it the device would silently vanish from a reloaded project.
    DeviceFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto kick = std::make_shared<Kick808Device>("Kick808");
    kick->setTune(0.62f);
    kick->setTone(0.18f);
    kick->setDecay(0.88f);
    kick->setPitchDepth(0.55f);
    kick->setPitchDecay(0.12f);
    kick->setDrive(0.44f);
    kick->setGlide(0.33f);
    kick->setKeyTrack(false);
    kick->setLpfCutoff(0.65f);
    kick->setHpfCutoff(0.2f);
    kick->setFaderPosition(Device::FaderPosition::PostInserts);
    kick->setSendTap(Device::SendTap::PreFader);
    deviceServiceOut.setDevice(1, kick);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto restored = std::dynamic_pointer_cast<Kick808Device>(deviceServiceIn.device(size_t { 1 }));
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), Kick808Device::typeIdString());

    QVERIFY(std::abs(restored->tune() - 0.62f) < 0.001f);
    QVERIFY(std::abs(restored->tone() - 0.18f) < 0.001f);
    QVERIFY(std::abs(restored->decay() - 0.88f) < 0.001f);
    QVERIFY(std::abs(restored->pitchDepth() - 0.55f) < 0.001f);
    QVERIFY(std::abs(restored->pitchDecay() - 0.12f) < 0.001f);
    QVERIFY(std::abs(restored->drive() - 0.44f) < 0.001f);
    QVERIFY(std::abs(restored->glide() - 0.33f) < 0.001f);
    QCOMPARE(restored->keyTrack(), false);
    QVERIFY(std::abs(restored->lpfCutoff() - 0.65f) < 0.001f);
    QVERIFY(std::abs(restored->hpfCutoff() - 0.2f) < 0.001f);

    QCOMPARE(static_cast<int>(restored->faderPosition()), static_cast<int>(Device::FaderPosition::PostInserts));
    QCOMPARE(static_cast<int>(restored->sendTap()), static_cast<int>(Device::SendTap::PreFader));
}

void XmlSerializationTest::test_toXmlFromXml_tubeStage_shouldLoadCorrectly()
{
    // Effects are rebuilt through EffectFactory, so this also covers the factory registration:
    // without it the effect would silently vanish from a reloaded project.
    EffectFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto tubeStage = std::make_shared<TubeStage>();
    if (auto p = tubeStage->parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        p->get().setValue(1.0f);
    }
    if (auto p = tubeStage->parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString()); p) {
        p->get().setValue(0.72f);
    }
    if (auto p = tubeStage->parameter(Constants::NahdXml::xmlKeyBias().toStdString()); p) {
        p->get().setValue(0.31f);
    }
    if (auto p = tubeStage->parameter(Constants::NahdXml::xmlKeyTone().toStdString()); p) {
        p->get().setValue(0.64f);
    }
    if (auto p = tubeStage->parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        p->get().setValue(0.85f);
    }
    deviceServiceOut.sendEffectRack().setEffect(0, tubeStage);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    const auto restored = std::dynamic_pointer_cast<TubeStage>(effect);
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), TubeStage::typeIdString());

    const auto value = [&](const QString & key) {
        const auto p = restored->parameter(key.toStdString());
        return p ? p->get().value() : -1.0f;
    };
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyMode()) - 1.0f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyDriveDb()) - 0.72f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyBias()) - 0.31f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyTone()) - 0.64f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyMix()) - 0.85f) < 0.001f);
}

void XmlSerializationTest::test_toXmlFromXml_bassGrinder_shouldLoadCorrectly()
{
    // Effects are rebuilt through EffectFactory, so this also covers the factory registration:
    // without it the effect would silently vanish from a reloaded project.
    EffectFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto bassGrinder = std::make_shared<BassGrinder>();
    const auto set = [&](const QString & key, float value) {
        if (auto p = bassGrinder->parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    set(Constants::NahdXml::xmlKeyDrive(), 0.62f);
    set(Constants::NahdXml::xmlKeyBlend(), 0.44f);
    set(Constants::NahdXml::xmlKeySplitFreq(), 0.71f);
    set(Constants::NahdXml::xmlKeyColor(), 1.0f);
    set(Constants::NahdXml::xmlKeyBassGain(), 0.83f);
    set(Constants::NahdXml::xmlKeyMidGain(), 0.29f);
    set(Constants::NahdXml::xmlKeyMidFreq(), 0.37f);
    set(Constants::NahdXml::xmlKeyHighGain(), 0.66f);
    set(Constants::NahdXml::xmlKeyMix(), 0.91f);
    deviceServiceOut.sendEffectRack().setEffect(0, bassGrinder);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    const auto restored = std::dynamic_pointer_cast<BassGrinder>(effect);
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), BassGrinder::typeIdString());

    const auto value = [&](const QString & key) {
        const auto p = restored->parameter(key.toStdString());
        return p ? p->get().value() : -1.0f;
    };
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyDrive()) - 0.62f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyBlend()) - 0.44f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeySplitFreq()) - 0.71f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyColor()) - 1.0f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyBassGain()) - 0.83f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyMidGain()) - 0.29f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyMidFreq()) - 0.37f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyHighGain()) - 0.66f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyMix()) - 0.91f) < 0.001f);
}

void XmlSerializationTest::test_toXmlFromXml_stereoExciter_shouldLoadCorrectly()
{
    EffectFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto exciter = std::make_shared<StereoExciter>();
    const auto set = [&](const QString & key, float value) {
        if (auto p = exciter->parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    set(Constants::NahdXml::xmlKeyTune(), 0.72f);
    set(Constants::NahdXml::xmlKeyPeak(), 0.44f);
    set(Constants::NahdXml::xmlKeyZeroFill(), 0.61f);
    set(Constants::NahdXml::xmlKeyTimbre(), 0.28f);
    set(Constants::NahdXml::xmlKeyHarmonics(), 0.83f);
    set(Constants::NahdXml::xmlKeySolo(), 1.0f);
    deviceServiceOut.sendEffectRack().setEffect(0, exciter);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    const auto restored = std::dynamic_pointer_cast<StereoExciter>(effect);
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), StereoExciter::typeIdString());

    const auto value = [&](const QString & key) {
        const auto p = restored->parameter(key.toStdString());
        return p ? p->get().value() : -1.0f;
    };
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyTune()) - 0.72f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyPeak()) - 0.44f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyZeroFill()) - 0.61f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyTimbre()) - 0.28f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyHarmonics()) - 0.83f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeySolo()) - 1.0f) < 0.01f);
}

void XmlSerializationTest::test_toXmlFromXml_stereoEnhancer_shouldLoadCorrectly()
{
    EffectFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto enhancer = std::make_shared<StereoEnhancer>();
    const auto set = [&](const QString & key, float value) {
        if (auto p = enhancer->parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    set(Constants::NahdXml::xmlKeyBassGain(), 0.65f);
    set(Constants::NahdXml::xmlKeyBassFreq(), 0.35f);
    set(Constants::NahdXml::xmlKeyMidGain(), 0.42f);
    set(Constants::NahdXml::xmlKeyMidQ(), 0.77f);
    set(Constants::NahdXml::xmlKeyHighGain(), 0.58f);
    set(Constants::NahdXml::xmlKeyHighFreq(), 0.81f);
    set(Constants::NahdXml::xmlKeySpread(), 0.24f);
    set(Constants::NahdXml::xmlKeySolo(), 1.0f);
    deviceServiceOut.sendEffectRack().setEffect(0, enhancer);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    const auto restored = std::dynamic_pointer_cast<StereoEnhancer>(effect);
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), StereoEnhancer::typeIdString());

    const auto value = [&](const QString & key) {
        const auto p = restored->parameter(key.toStdString());
        return p ? p->get().value() : -1.0f;
    };
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyBassGain()) - 0.65f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyBassFreq()) - 0.35f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyMidGain()) - 0.42f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyMidQ()) - 0.77f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyHighGain()) - 0.58f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyHighFreq()) - 0.81f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeySpread()) - 0.24f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeySolo()) - 1.0f) < 0.01f);
}

void XmlSerializationTest::test_toXmlFromXml_waveDesigner_shouldLoadCorrectly()
{
    // Effects are rebuilt through EffectFactory, so this also covers the factory registration:
    // without it the effect would silently vanish from a reloaded project.
    EffectFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto waveDesigner = std::make_shared<WaveDesigner>();
    if (auto p = waveDesigner->parameter(Constants::NahdXml::xmlKeyAttack().toStdString()); p) {
        p->get().setValue(0.83f);
    }
    if (auto p = waveDesigner->parameter(Constants::NahdXml::xmlKeySustain().toStdString()); p) {
        p->get().setValue(0.21f);
    }
    if (auto p = waveDesigner->parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        p->get().setValue(0.62f);
    }
    deviceServiceOut.sendEffectRack().setEffect(0, waveDesigner);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    const auto restored = std::dynamic_pointer_cast<WaveDesigner>(effect);
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), WaveDesigner::typeIdString());

    const auto value = [&](const QString & key) {
        const auto p = restored->parameter(key.toStdString());
        return p ? p->get().value() : -1.0f;
    };
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyAttack()) - 0.83f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeySustain()) - 0.21f) < 0.001f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyGain()) - 0.62f) < 0.001f);
}

void XmlSerializationTest::test_toXmlFromXml_reverbGate_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto reverb = std::make_shared<Reverb>();
    if (auto p = reverb->parameter(Constants::NahdXml::xmlKeyGated().toStdString()); p) {
        p->get().setValue(1.0f);
    }
    if (auto p = reverb->parameter(Constants::NahdXml::xmlKeyHold().toStdString()); p) {
        p->get().setValue(0.4f);
    }
    deviceServiceOut.sendEffectRack().setEffect(0, reverb);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    const auto restored = std::dynamic_pointer_cast<Reverb>(effect);
    QVERIFY(restored);
    if (auto p = restored->parameter(Constants::NahdXml::xmlKeyGated().toStdString()); p) {
        QCOMPARE(p->get().value(), 1.0f);
    }
    if (auto p = restored->parameter(Constants::NahdXml::xmlKeyHold().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.4f);
    }
}

void XmlSerializationTest::test_toXmlFromXml_limiterEffect_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto limiter = std::make_shared<Limiter>();
    if (auto p = limiter->parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
        p->get().setValue(0.5f);
    }
    if (auto p = limiter->parameter(Constants::NahdXml::xmlKeyCeiling().toStdString()); p) {
        p->get().setValue(0.5f);
    }
    if (auto p = limiter->parameter(Constants::NahdXml::xmlKeyBoost().toStdString()); p) {
        p->get().setValue(1.0f);
    }
    deviceServiceOut.sendEffectRack().setEffect(0, limiter);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), Limiter::typeIdString());
    const auto restored = std::dynamic_pointer_cast<Limiter>(effect);
    QVERIFY(restored);
    if (auto p = restored->parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.5f);
    }
    if (auto p = restored->parameter(Constants::NahdXml::xmlKeyBoost().toStdString()); p) {
        QCOMPARE(p->get().value(), 1.0f);
    }
}

void XmlSerializationTest::test_toXmlFromXml_autoDuckerEffect_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto autoDucker = std::make_shared<AutoDucker>();
    if (auto p = autoDucker->parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
        p->get().setValue(0.5f);
    }
    if (auto p = autoDucker->parameter(Constants::NahdXml::xmlKeyAmount().toStdString()); p) {
        p->get().setValue(0.75f); // +12 dB, the boosting half of the range
    }
    if (auto p = autoDucker->parameter(Constants::NahdXml::xmlKeyHold().toStdString()); p) {
        p->get().setValue(0.4f);
    }
    if (auto p = autoDucker->parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        p->get().setValue(2.0f);
    }
    deviceServiceOut.sendEffectRack().setEffect(0, autoDucker);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), AutoDucker::typeIdString());
    const auto restored = std::dynamic_pointer_cast<AutoDucker>(effect);
    QVERIFY(restored);
    if (auto p = restored->parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.5f);
    }
    if (auto p = restored->parameter(Constants::NahdXml::xmlKeyAmount().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.75f);
    }
    if (auto p = restored->parameter(Constants::NahdXml::xmlKeyHold().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.4f);
    }
    QCOMPARE(restored->sidechainSourceDeviceIndex(), std::optional<size_t> { 2 });
}

void XmlSerializationTest::test_toXmlFromXml_autoFilterEffect_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto autoFilter = std::make_shared<AutoFilter>();
    // A different value for every parameter, so that two keys pointing at the same entry cannot
    // pass unnoticed. The discrete ones hold their setting itself rather than a position.
    const std::vector<std::pair<QString, float>> values {
        { Constants::NahdXml::xmlKeyFilterType(), 2.0f },
        { Constants::NahdXml::xmlKeyFilterSlope(), 0.0f },
        { Constants::NahdXml::xmlKeyCutoff(), 0.42f },
        { Constants::NahdXml::xmlKeyResonance(), 0.66f },
        { Constants::NahdXml::xmlKeyLfoWaveform(), 3.0f },
        { Constants::NahdXml::xmlKeyLfoMode(), 1.0f },
        { Constants::NahdXml::xmlKeyLfoRate(), 0.25f },
        { Constants::NahdXml::xmlKeyLfoIntensity(), 0.9f },
        { Constants::NahdXml::xmlKeyLfo2Waveform(), 4.0f },
        { Constants::NahdXml::xmlKeyLfo2Mode(), 2.0f },
        { Constants::NahdXml::xmlKeyLfo2Rate(), 0.75f },
        { Constants::NahdXml::xmlKeyLfo2Intensity(), 0.2f },
        { Constants::NahdXml::xmlKeyStereoPhase(), 0.5f },
        { Constants::NahdXml::xmlKeyEnvMod(), 0.8f },
        { Constants::NahdXml::xmlKeyAttack(), 0.35f },
        { Constants::NahdXml::xmlKeyRelease(), 0.45f },
        { Constants::NahdXml::xmlKeyGain(), 0.55f },
        { Constants::NahdXml::xmlKeyMix(), 0.85f },
    };
    for (auto && [key, value] : values) {
        if (auto p = autoFilter->parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    }
    deviceServiceOut.sendEffectRack().setEffect(0, autoFilter);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), AutoFilter::typeIdString());
    const auto restored = std::dynamic_pointer_cast<AutoFilter>(effect);
    QVERIFY(restored);

    for (auto && [key, value] : values) {
        const auto p = restored->parameter(key.toStdString());
        QVERIFY(p.has_value());
        QVERIFY(std::abs(p->get().value() - value) < 1.0e-3f);
    }
}

void XmlSerializationTest::test_toXmlFromXml_phaserEffect_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto phaser = std::make_shared<Phaser>();
    const std::vector<std::pair<QString, float>> values {
        { Constants::NahdXml::xmlKeyStages(), 10.0f },
        { Constants::NahdXml::xmlKeyFrequency(), 0.31f },
        { Constants::NahdXml::xmlKeyDepth(), 0.62f },
        { Constants::NahdXml::xmlKeyFeedback(), 0.8f },
        { Constants::NahdXml::xmlKeyLfoWaveform(), 2.0f },
        { Constants::NahdXml::xmlKeyLfoMode(), 1.0f },
        { Constants::NahdXml::xmlKeyLfoRate(), 0.4f },
        { Constants::NahdXml::xmlKeyRateDivider(), 7.0f },
        { Constants::NahdXml::xmlKeyStereoPhase(), 0.25f },
        { Constants::NahdXml::xmlKeyGain(), 0.6f },
        { Constants::NahdXml::xmlKeyMix(), 0.35f },
    };
    for (auto && [key, value] : values) {
        if (auto p = phaser->parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    }
    deviceServiceOut.sendEffectRack().setEffect(0, phaser);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), Phaser::typeIdString());
    const auto restored = std::dynamic_pointer_cast<Phaser>(effect);
    QVERIFY(restored);

    for (auto && [key, value] : values) {
        const auto p = restored->parameter(key.toStdString());
        QVERIFY(p.has_value());
        QVERIFY(std::abs(p->get().value() - value) < 1.0e-3f);
    }
}

void XmlSerializationTest::test_toXmlFromXml_multibandCompressorEffect_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto multibandCompressor = std::make_shared<MultibandCompressor>();
    if (auto p = multibandCompressor->parameter(Constants::NahdXml::xmlKeyCrossoverFreq(0).toStdString()); p) {
        p->get().setValue(0.25f);
    }
    if (auto p = multibandCompressor->parameter(Constants::NahdXml::xmlKeyCrossoverFreq(1).toStdString()); p) {
        p->get().setValue(0.8f);
    }
    // One of each per-band parameter, on a different band each time, so a key built with the wrong
    // band index cannot pass unnoticed.
    if (auto p = multibandCompressor->parameter(Constants::NahdXml::xmlKeyBandThreshold(0).toStdString()); p) {
        p->get().setValue(0.5f);
    }
    if (auto p = multibandCompressor->parameter(Constants::NahdXml::xmlKeyBandRatio(1).toStdString()); p) {
        p->get().setValue(0.75f);
    }
    if (auto p = multibandCompressor->parameter(Constants::NahdXml::xmlKeyBandKnee(2).toStdString()); p) {
        p->get().setValue(0.4f);
    }
    if (auto p = multibandCompressor->parameter(Constants::NahdXml::xmlKeyBandAttack(0).toStdString()); p) {
        p->get().setValue(0.3f);
    }
    if (auto p = multibandCompressor->parameter(Constants::NahdXml::xmlKeyBandRelease(1).toStdString()); p) {
        p->get().setValue(0.7f);
    }
    if (auto p = multibandCompressor->parameter(Constants::NahdXml::xmlKeyBandMakeup(2).toStdString()); p) {
        p->get().setValue(0.6f);
    }
    if (auto p = multibandCompressor->parameter(Constants::NahdXml::xmlKeyBandBypass(1).toStdString()); p) {
        p->get().setValue(1.0f);
    }
    if (auto p = multibandCompressor->parameter(Constants::NahdXml::xmlKeyBandSolo(2).toStdString()); p) {
        p->get().setValue(1.0f);
    }
    if (auto p = multibandCompressor->parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        p->get().setValue(1.0f);
    }
    if (auto p = multibandCompressor->parameter(Constants::NahdXml::xmlKeySideChainSourceDevice().toStdString()); p) {
        p->get().setValue(2.0f);
    }
    deviceServiceOut.sendEffectRack().setEffect(0, multibandCompressor);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), MultibandCompressor::typeIdString());
    const auto restored = std::dynamic_pointer_cast<MultibandCompressor>(effect);
    QVERIFY(restored);

    const auto compareParameter = [&restored](const QString & key, float expected) {
        const auto p = restored->parameter(key.toStdString());
        QVERIFY(p.has_value());
        QVERIFY(std::abs(p->get().value() - expected) < 1.0e-3f);
    };

    compareParameter(Constants::NahdXml::xmlKeyCrossoverFreq(0), 0.25f);
    compareParameter(Constants::NahdXml::xmlKeyCrossoverFreq(1), 0.8f);
    compareParameter(Constants::NahdXml::xmlKeyBandThreshold(0), 0.5f);
    compareParameter(Constants::NahdXml::xmlKeyBandRatio(1), 0.75f);
    compareParameter(Constants::NahdXml::xmlKeyBandKnee(2), 0.4f);
    compareParameter(Constants::NahdXml::xmlKeyBandAttack(0), 0.3f);
    compareParameter(Constants::NahdXml::xmlKeyBandRelease(1), 0.7f);
    compareParameter(Constants::NahdXml::xmlKeyBandMakeup(2), 0.6f);
    compareParameter(Constants::NahdXml::xmlKeyBandBypass(1), 1.0f);
    compareParameter(Constants::NahdXml::xmlKeyBandSolo(2), 1.0f);

    // The bands that were left alone must come back at their defaults rather than at a neighbour's value.
    compareParameter(Constants::NahdXml::xmlKeyBandBypass(0), 0.0f);
    compareParameter(Constants::NahdXml::xmlKeyBandSolo(0), 0.0f);

    QCOMPARE(restored->sidechainSourceDeviceIndex(), std::optional<size_t> { 2 });
}

void XmlSerializationTest::test_toXmlFromXml_stereoWidenerEffect_shouldLoadCorrectly()
{
    EffectFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto stereoWidth = std::make_shared<StereoWidener>();
    const auto set = [&](const QString & key, float value) {
        if (auto p = stereoWidth->parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    set(Constants::NahdXml::xmlKeyCrossoverFreq(0), 0.3f);
    set(Constants::NahdXml::xmlKeyCrossoverFreq(1), 0.85f);
    set(Constants::NahdXml::xmlKeyBandWidth(0), 0.0f);
    set(Constants::NahdXml::xmlKeyBandWidth(1), 0.55f);
    set(Constants::NahdXml::xmlKeyBandWidth(2), 0.9f);
    set(Constants::NahdXml::xmlKeyBandSolo(1), 1.0f);
    set(Constants::NahdXml::xmlKeyMonoBass(), 1.0f);
    set(Constants::NahdXml::xmlKeyMonoFreq(), 0.42f);
    set(Constants::NahdXml::xmlKeyGain(), 0.7f);
    deviceServiceOut.sendEffectRack().setEffect(0, stereoWidth);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    const auto restored = std::dynamic_pointer_cast<StereoWidener>(effect);
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), StereoWidener::typeIdString());

    const auto compareParameter = [&restored](const QString & key, float expected) {
        const auto p = restored->parameter(key.toStdString());
        QVERIFY(p.has_value());
        QVERIFY(std::abs(p->get().value() - expected) < 0.01f);
    };

    compareParameter(Constants::NahdXml::xmlKeyCrossoverFreq(0), 0.3f);
    compareParameter(Constants::NahdXml::xmlKeyCrossoverFreq(1), 0.85f);
    compareParameter(Constants::NahdXml::xmlKeyBandWidth(0), 0.0f);
    compareParameter(Constants::NahdXml::xmlKeyBandWidth(1), 0.55f);
    compareParameter(Constants::NahdXml::xmlKeyBandWidth(2), 0.9f);
    compareParameter(Constants::NahdXml::xmlKeyBandSolo(1), 1.0f);
    compareParameter(Constants::NahdXml::xmlKeyMonoBass(), 1.0f);
    compareParameter(Constants::NahdXml::xmlKeyMonoFreq(), 0.42f);
    compareParameter(Constants::NahdXml::xmlKeyGain(), 0.7f);

    // The bands that were left alone must come back at their defaults rather than at a neighbour's value.
    compareParameter(Constants::NahdXml::xmlKeyBandSolo(0), 0.0f);
    compareParameter(Constants::NahdXml::xmlKeyBandSolo(2), 0.0f);
}

void XmlSerializationTest::test_toXmlFromXml_earlyReflectionsEffect_shouldLoadCorrectly()
{
    EffectFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto reflections = std::make_shared<EarlyReflections>();
    const auto set = [&](const QString & key, float value) {
        if (auto p = reflections->parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    set(Constants::NahdXml::xmlKeySize(), 0.62f);
    set(Constants::NahdXml::xmlKeyPreDelay(), 0.31f);
    set(Constants::NahdXml::xmlKeyDamping(), 0.77f);
    set(Constants::NahdXml::xmlKeyWidth(), 0.9f);
    set(Constants::NahdXml::xmlKeyHpfCutoff(), 0.45f);
    set(Constants::NahdXml::xmlKeyDiffusion(), 0.58f);
    set(Constants::NahdXml::xmlKeyMix(), 0.35f);
    deviceServiceOut.sendEffectRack().setEffect(0, reflections);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    const auto restored = std::dynamic_pointer_cast<EarlyReflections>(effect);
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), EarlyReflections::typeIdString());

    const auto value = [&](const QString & key) {
        const auto p = restored->parameter(key.toStdString());
        return p ? p->get().value() : -1.0f;
    };
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeySize()) - 0.62f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyPreDelay()) - 0.31f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyDamping()) - 0.77f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyWidth()) - 0.9f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyHpfCutoff()) - 0.45f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyDiffusion()) - 0.58f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyMix()) - 0.35f) < 0.01f);
}

void XmlSerializationTest::test_toXmlFromXml_analogFuzzEffect_shouldLoadCorrectly()
{
    EffectFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto analogFuzz = std::make_shared<AnalogFuzz>();
    const auto set = [&](const QString & key, float value) {
        if (auto p = analogFuzz->parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    set(Constants::NahdXml::xmlKeyDrive(), 0.72f);
    set(Constants::NahdXml::xmlKeyFuzz(), 0.31f);
    set(Constants::NahdXml::xmlKeyBias(), 0.66f);
    set(Constants::NahdXml::xmlKeyCutoff(), 0.44f);
    set(Constants::NahdXml::xmlKeyResonance(), 0.88f);
    set(Constants::NahdXml::xmlKeyMix(), 0.55f);
    deviceServiceOut.sendEffectRack().setEffect(0, analogFuzz);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    const auto restored = std::dynamic_pointer_cast<AnalogFuzz>(effect);
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), AnalogFuzz::typeIdString());

    const auto value = [&](const QString & key) {
        const auto p = restored->parameter(key.toStdString());
        return p ? p->get().value() : -1.0f;
    };
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyDrive()) - 0.72f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyFuzz()) - 0.31f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyBias()) - 0.66f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyCutoff()) - 0.44f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyResonance()) - 0.88f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyMix()) - 0.55f) < 0.01f);
}

void XmlSerializationTest::test_toXmlFromXml_gainEffect_shouldLoadCorrectly()
{
    EffectFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto gain = std::make_shared<Gain>();
    if (auto p = gain->parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        p->get().setValue(-6.0f / 48.0f + 0.5f);
    }
    deviceServiceOut.insertEffectRack().setEffect(0, gain);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.insertEffectRack().effect(0);
    QVERIFY(effect);
    const auto restored = std::dynamic_pointer_cast<Gain>(effect);
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), Gain::typeIdString());
    QVERIFY(std::abs(restored->gainDb() + 6.0f) < 0.05f);
}

void XmlSerializationTest::test_toXmlFromXml_monitorEffect_shouldLoadCorrectly()
{
    EffectFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto monitor = std::make_shared<Monitor>();
    if (auto p = monitor->parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        p->get().setValue(static_cast<float>(Monitor::Mode::Side));
    }
    // The insert rack, because that is the only place a Monitor does anything: a send is parallel,
    // and folding a parallel bus to mono says nothing about the mix.
    deviceServiceOut.insertEffectRack().setEffect(0, monitor);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.insertEffectRack().effect(0);
    QVERIFY(effect);
    const auto restored = std::dynamic_pointer_cast<Monitor>(effect);
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), Monitor::typeIdString());
    // Deserialization syncs the effect, so the mode is in effect and not just stored.
    QCOMPARE(restored->mode(), Monitor::Mode::Side);
}

void XmlSerializationTest::test_toXmlFromXml_dimensionEffect_shouldLoadCorrectly()
{
    EffectFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto dimension = std::make_shared<Dimension>();
    const auto set = [&](const QString & key, float value) {
        if (auto p = dimension->parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    set(Constants::NahdXml::xmlKeyDetune(), 0.44f);
    set(Constants::NahdXml::xmlKeyAmount(), 0.66f);
    set(Constants::NahdXml::xmlKeyHpfCutoff(), 0.81f);
    set(Constants::NahdXml::xmlKeySolo(), 1.0f);
    deviceServiceOut.sendEffectRack().setEffect(0, dimension);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    const auto restored = std::dynamic_pointer_cast<Dimension>(effect);
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), Dimension::typeIdString());

    const auto value = [&](const QString & key) {
        const auto p = restored->parameter(key.toStdString());
        return p ? p->get().value() : -1.0f;
    };
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyDetune()) - 0.44f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyAmount()) - 0.66f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyHpfCutoff()) - 0.81f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeySolo()) - 1.0f) < 0.01f);
}

void XmlSerializationTest::test_toXmlFromXml_stereoFieldMeterEffect_shouldLoadCorrectly()
{
    EffectFactory::init();

    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    auto meter = std::make_shared<StereoFieldMeter>();
    const auto set = [&](const QString & key, float value) {
        if (auto p = meter->parameter(key.toStdString()); p) {
            p->get().setValue(value);
        }
    };
    set(Constants::NahdXml::xmlKeySpeed(), 2.0f);
    set(Constants::NahdXml::xmlKeyZoom(), 0.72f);
    set(Constants::NahdXml::xmlKeyShowGuides(), 0.0f);
    deviceServiceOut.sendEffectRack().setEffect(0, meter);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    const auto restored = std::dynamic_pointer_cast<StereoFieldMeter>(effect);
    QVERIFY(restored);
    QCOMPARE(restored->typeId(), StereoFieldMeter::typeIdString());

    const auto value = [&](const QString & key) {
        const auto p = restored->parameter(key.toStdString());
        return p ? p->get().value() : -1.0f;
    };
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeySpeed()) - 2.0f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyZoom()) - 0.72f) < 0.01f);
    QVERIFY(std::abs(value(Constants::NahdXml::xmlKeyShowGuides())) < 0.01f);
}

void XmlSerializationTest::test_toXmlFromXml_lufsMeterEffect_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    deviceServiceOut.sendEffectRack().setEffect(0, std::make_shared<LufsMeter>());

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), LufsMeter::typeIdString());
    QVERIFY(std::dynamic_pointer_cast<LufsMeter>(effect));
}

void XmlSerializationTest::test_toXmlFromXml_dbtpMeterEffect_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    deviceServiceOut.sendEffectRack().setEffect(0, std::make_shared<DbTpMeter>());

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    const auto effect = deviceServiceIn.sendEffectRack().effect(0);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), DbTpMeter::typeIdString());
    QVERIFY(std::dynamic_pointer_cast<DbTpMeter>(effect));
}

void XmlSerializationTest::test_toXmlFromXml_delayEffectRack_shouldLoadCorrectly()
{
    const auto engineOut = std::make_shared<AudioEngine>();
    DeviceService deviceServiceOut { engineOut, std::make_shared<DataService>() };

    // Add a delay effect to slot 1 of master send rack
    auto delay = std::make_shared<Delay>();
    delay->setType(Delay::Type::PingPong);
    delay->setTime(0.5);
    delay->setFeedback(0.7);
    delay->setDepth(0.8);
    delay->setMix(0.4);
    delay->setSync(true);
    delay->setSyncDivision(0.25);
    delay->setFeedbackLpf(0.6);
    delay->setFeedbackHpf(0.1);
    // Important: sync() must be called to update the Parameter values from internal state before serialization
    // OR, we should set the parameter values directly if that's how it's normally done.
    // Actually, in normal usage, the UI sets parameters, which are then synced to internal state.
    // For the test, we can set parameters and then check if they persist.
    if (const auto p = delay->parameter(Constants::NahdXml::xmlKeyDelayType().toStdString()); p)
        p->get().setFromXml(2);
    if (const auto p = delay->parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString()); p)
        p->get().setValue(0.05f); // 0.5s
    if (const auto p = delay->parameter(Constants::NahdXml::xmlKeyDelayFeedback().toStdString()); p)
        p->get().setValue(0.7f);
    if (const auto p = delay->parameter(Constants::NahdXml::xmlKeyDelayDepth().toStdString()); p)
        p->get().setValue(0.8f);
    if (const auto p = delay->parameter(Constants::NahdXml::xmlKeyDelayMix().toStdString()); p)
        p->get().setValue(0.4f);
    if (const auto p = delay->parameter(Constants::NahdXml::xmlKeyDelaySync().toStdString()); p)
        p->get().setValue(1.0f);
    if (const auto p = delay->parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString()); p)
        p->get().setValue(0.25f);
    if (const auto p = delay->parameter(Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString()); p)
        p->get().setValue(0.6f);
    if (const auto p = delay->parameter(Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString()); p)
        p->get().setValue(0.1f);

    deviceServiceOut.sendEffectRack().setEffect(1, delay);

    EditorService editorServiceOut { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceOut, &EditorService::devicesSerializationRequested, &deviceServiceOut, &DeviceService::serializeToXml);

    const auto xml = editorServiceOut.toXml();

    const auto engineIn = std::make_shared<AudioEngine>();
    DeviceService deviceServiceIn { engineIn, std::make_shared<DataService>() };
    EditorService editorServiceIn { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), std::make_shared<AutomationService>(std::make_shared<PropertyService>()), std::make_shared<DataService>() };
    connect(&editorServiceIn, &EditorService::devicesDeserializationRequested, &deviceServiceIn, &DeviceService::deserializeFromXml);

    editorServiceIn.fromXml(xml);

    auto effect = deviceServiceIn.sendEffectRack().effect(1);
    QVERIFY(effect);
    QCOMPARE(effect->typeId(), Delay::typeIdString());

    // Verify parameter values
    if (const auto p = effect->parameter(Constants::NahdXml::xmlKeyDelayType().toStdString()); p)
        QCOMPARE(p->get().xmlValue(), 2);
    if (const auto p = effect->parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString()); p)
        QCOMPARE(p->get().value(), 0.05f);
    if (const auto p = effect->parameter(Constants::NahdXml::xmlKeyDelayFeedback().toStdString()); p)
        QCOMPARE(p->get().value(), 0.7f);
    if (const auto p = effect->parameter(Constants::NahdXml::xmlKeyDelayDepth().toStdString()); p)
        QCOMPARE(p->get().value(), 0.8f);
    if (const auto p = effect->parameter(Constants::NahdXml::xmlKeyDelayMix().toStdString()); p)
        QCOMPARE(p->get().value(), 0.4f);
    if (const auto p = effect->parameter(Constants::NahdXml::xmlKeyDelaySync().toStdString()); p)
        QCOMPARE(p->get().value(), 1.0f);
    if (const auto p = effect->parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString()); p)
        QCOMPARE(p->get().value(), 0.25f);
    if (const auto p = effect->parameter(Constants::NahdXml::xmlKeyDelayFeedbackLpf().toStdString()); p)
        QCOMPARE(p->get().value(), 0.6f);
    if (const auto p = effect->parameter(Constants::NahdXml::xmlKeyDelayFeedbackHpf().toStdString()); p)
        QCOMPARE(p->get().value(), 0.1f);
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

void XmlSerializationTest::test_stringVoice_legacyFemale8_shouldLoadAsUpperMale8()
{
    // The Human Voice section follows the hardware's two switches now: Lower carries Male 8' and 4',
    // Upper carries Male 8' and Female 4'. There is no Female 8' on a VC340, so that register
    // became the upper register's Male 8' and a project written before the change has to find its
    // value there.
    StringVoiceDevice device { "Test StringVoice" };

    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "Test StringVoice");
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeName(), Constants::NahdXml::xmlValueSynths());
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "voiceFemale8");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "70"); // 0.7 internal
    writer.writeEndElement();

    writer.writeEndElement(); // Parameters
    writer.writeEndElement(); // Device

    NahdXmlReader reader { xml };
    while (!reader.atEnd()) {
        if (reader.readNextStartElement() && reader.name() == Constants::NahdXml::xmlKeyDevice()) {
            device.deserializeFromXml(reader);
        }
    }

    QCOMPARE(device.voiceUpperMale8(), 0.7f);
}

void XmlSerializationTest::test_stringVoiceV2_shouldRoundTripThroughTheFactory()
{
    // Covers the factory registration as well as the parameters: a V2 device has to come back as a
    // V2 device and not as the V1 it was copied from.
    StringVoiceV2Device device { "Test StringVoiceV2" };
    device.setStringsUpper(false);
    device.setStringsLower(true);
    device.setStringsTone(0.25f);
    device.setStringsBalance(0.4f);

    QString xml;
    {
        NahdXmlWriter writer { xml };
        device.serializeToXml(writer);
    }

    NahdXmlReader reader { xml };
    QVERIFY(reader.readNextStartElement());
    QCOMPARE(reader.name(), Constants::NahdXml::xmlKeyDevice());

    StringVoiceV2Device restored { "Restored" };
    restored.deserializeFromXml(reader);

    QCOMPARE(restored.stringsUpper(), false);
    QCOMPARE(restored.stringsLower(), true);
    QCOMPARE(restored.stringsTone(), 0.25f);
    QCOMPARE(restored.stringsBalance(), 0.4f);
    QCOMPARE(restored.typeId(), StringVoiceV2Device::typeIdString());
    QVERIFY(restored.typeId() != StringVoiceDevice::typeIdString());
}

void XmlSerializationTest::test_wavetableSynth_legacyNames_shouldLoadCorrectly()
{
    WavetableSynthDevice synth("TestWavetable");

    // Create legacy XML
    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "TestWavetable");
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeName(), Constants::NahdXml::xmlValueSynths());

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());

    // Legacy key: wavetableSynthAmpAttack
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "wavetableSynthAmpAttack");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "5000"); // 0.5 internal
    writer.writeEndElement();

    // Legacy key: wavetableSynthLpfCutoff
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "wavetableSynthLpfCutoff");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "3000"); // 0.3 internal
    writer.writeEndElement();

    writer.writeEndElement(); // Parameters
    writer.writeEndElement(); // Device

    NahdXmlReader reader { xml };
    while (!reader.atEnd()) {
        if (reader.readNextStartElement() && reader.name() == Constants::NahdXml::xmlKeyDevice()) {
            synth.deserializeFromXml(reader);
        }
    }

    QCOMPARE(synth.ampAttack(), 0.5f);
    QCOMPARE(synth.lpfCutoff(), 0.3f);
}

void XmlSerializationTest::test_wavetableSynth_legacyWavetableRange_shouldPreserveSelection()
{
    // A project saved when only two sets existed stores the narrower range alongside the value. The
    // selection is an ordinal into an append-only list, so it has to survive the list growing, and
    // the stored range must not limit what the device can be switched to afterwards.
    WavetableSynthDevice synth("TestWavetable");

    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "TestWavetable");
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeName(), Constants::NahdXml::xmlValueSynths());
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "wavetableSynthWavetableIndex");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueInt());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "1"); // Spectral Additive
    writer.writeAttribute(Constants::NahdXml::xmlKeyMin(), "0");
    writer.writeAttribute(Constants::NahdXml::xmlKeyMax(), "1"); // The old end of the list
    writer.writeEndElement();

    writer.writeEndElement(); // Parameters
    writer.writeEndElement(); // Device

    NahdXmlReader reader { xml };
    while (!reader.atEnd()) {
        if (reader.readNextStartElement() && reader.name() == Constants::NahdXml::xmlKeyDevice()) {
            synth.deserializeFromXml(reader);
        }
    }

    QCOMPARE(synth.wavetableIndex(), 1);
    QCOMPARE(synth.wavetableNames().at(0), std::string { "Classic Morph" });
    QCOMPARE(synth.wavetableNames().at(1), std::string { "Spectral Additive" });

    // The sets added since must still be reachable on a project saved before they existed.
    synth.setWavetableIndex(static_cast<int>(synth.wavetableNames().size()) - 1);
    QCOMPARE(synth.wavetableIndex(), static_cast<int>(synth.wavetableNames().size()) - 1);
}

void XmlSerializationTest::test_eq8BandParametric_legacyNames_shouldLoadCorrectly()
{
    Eq8BandParametric effect;

    // Create legacy XML
    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());

    // Legacy key: eq8BandParametricBand1Type
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "eq8BandParametricBand1Type");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueInt());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "2"); // HighPass
    writer.writeEndElement();

    // Legacy key: eq8BandParametricBand2Freq
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "eq8BandParametricBand2Freq");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "10010"); // 0.5 internal (range 20-20000)
    writer.writeEndElement();

    writer.writeEndElement(); // Parameters

    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        effect.deserializeParametersFromXml(reader);
    }

    if (auto p = effect.parameter(Constants::NahdXml::xmlKeyBandType(0).toStdString()); p) {
        QCOMPARE(p->get().xmlValue(), 2);
    }
    if (auto p = effect.parameter(Constants::NahdXml::xmlKeyBandFreq(1).toStdString()); p) {
        QCOMPARE(p->get().value(), 0.5f);
    }
}

namespace {

//! Writes one parameter as an older Noteahead wrote it: the stored value, and the range it was
//! stored against. The range matters -- it is what the loader reconstructs the position from before
//! the legacy converter maps that position into the new range.
QString legacyParameterXml(const QString & name, const QString & value, const QString & min, const QString & max)
{
    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), name);
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), value);
    writer.writeAttribute(Constants::NahdXml::xmlKeyMin(), min);
    writer.writeAttribute(Constants::NahdXml::xmlKeyMax(), max);
    writer.writeEndElement();
    writer.writeEndElement();
    return xml;
}

void loadParameters(Effect & effect, const QString & xml)
{
    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        effect.deserializeParametersFromXml(reader);
    }
}

} // namespace

void XmlSerializationTest::test_drive_legacyDrive_shouldKeepItsGain()
{
    // The control was 1x to 10x, linear in gain, and is now 40 dB linear in dB. A project saved
    // against the old one has to come out at the gain it had, not at the same knob position.
    Drive effect;
    loadParameters(effect, legacyParameterXml(Constants::NahdXml::xmlKeyDrive(), "50", "0", "100"));

    const auto p = effect.parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString());
    QVERIFY(p.has_value());

    // Half travel used to mean 1 + 0.5 * 9 = 5.5x, which is 14.8 dB.
    const double restoredDb = static_cast<double>(p->get().value()) * 40.0;
    QVERIFY(std::abs(restoredDb - 20.0 * std::log10(5.5)) < 0.05);
}

void XmlSerializationTest::test_saturator_legacyDrive_shouldKeepItsGain()
{
    Saturator effect;
    loadParameters(effect, legacyParameterXml(Constants::NahdXml::xmlKeyDrive(), "600", "0", "2400"));

    const auto p = effect.parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString());
    QVERIFY(p.has_value());

    // A quarter of the old 24 dB range is 6 dB, and 6 dB it must stay.
    QVERIFY(std::abs(static_cast<double>(p->get().value()) * 40.0 - 6.0) < 0.01);
}

void XmlSerializationTest::test_tubeStage_legacyDrive_shouldKeepItsGain()
{
    TubeStage effect;
    loadParameters(effect, legacyParameterXml(Constants::NahdXml::xmlKeyDrive(), "900", "0", "3600"));

    const auto p = effect.parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString());
    QVERIFY(p.has_value());

    // A quarter of the old 36 dB range is 9 dB.
    QVERIFY(std::abs(static_cast<double>(p->get().value()) * 48.0 - 9.0) < 0.01);
}

void XmlSerializationTest::test_chorus_legacyNames_shouldLoadCorrectly()
{
    Chorus effect;

    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "chorusRate");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "600"); // 0.6 internal (range 0-1000)
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "chorusMix");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "750"); // 0.75 internal (range 0-1000)
    writer.writeEndElement();

    writer.writeEndElement(); // Parameters

    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        effect.deserializeParametersFromXml(reader);
    }

    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyRate().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.6f);
    }
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.75f);
    }
}

void XmlSerializationTest::test_clipper_legacyNames_shouldLoadCorrectly()
{
    Clipper effect;

    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "clipperMode");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueInt());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "0"); // Hard
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "clipperGain");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "0"); // 0.5 internal (mid of range -2400..2400)
    writer.writeEndElement();

    writer.writeEndElement(); // Parameters

    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        effect.deserializeParametersFromXml(reader);
    }

    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMode().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.0f);
    }
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.5f);
    }
}

void XmlSerializationTest::test_reverb_legacyNames_shouldLoadCorrectly()
{
    Reverb effect;

    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "reverbSize");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "7500"); // 0.75 internal (range 0-10000)
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "reverbMix");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "3000"); // 0.3 internal (range 0-10000)
    writer.writeEndElement();

    writer.writeEndElement(); // Parameters

    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        effect.deserializeParametersFromXml(reader);
    }

    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeySize().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.75f);
    }
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyMix().toStdString()); p) {
        QCOMPARE(p->get().value(), 0.3f);
    }
}

void XmlSerializationTest::test_allPassFilter_legacyNames_shouldLoadCorrectly()
{
    AllPassFilter effect;

    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "allPassFilterFrequency");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "1000");
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "allPassFilterQ");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueFloat());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "500");
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameter());
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), "allPassFilterStages");
    writer.writeAttribute(Constants::NahdXml::xmlKeyParameterValueType(), Constants::NahdXml::xmlValueInt());
    writer.writeAttribute(Constants::NahdXml::xmlKeyValue(), "3");
    writer.writeEndElement();

    writer.writeEndElement(); // Parameters

    NahdXmlReader reader { xml };
    if (reader.readNextStartElement()) {
        effect.deserializeParametersFromXml(reader);
    }

    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyFrequency().toStdString()); p) {
        QCOMPARE(p->get().xmlValue(), 1000);
    }
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyQ().toStdString()); p) {
        QCOMPARE(p->get().xmlValue(), 500);
    }
    if (const auto p = effect.parameter(Constants::NahdXml::xmlKeyStages().toStdString()); p) {
        QCOMPARE(p->get().xmlValue(), 3);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::XmlSerializationTest)
