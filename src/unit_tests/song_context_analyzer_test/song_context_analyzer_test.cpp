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

#include "song_context_analyzer_test.hpp"

#include "../../application/position.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../application/service/song_context_analyzer.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device_factory.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../domain/tracker/note_data.hpp"
#include "../../domain/tracker/side_chain_settings.hpp"
#include "../../domain/tracker/song.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"
#include "../../plugins/devices/bass_synth/bass_synth_device.hpp"
#include "../../plugins/devices/fm_synth/fm_synth_device.hpp"
#include "../../plugins/devices/kick_808/kick_808_device.hpp"
#include "../../plugins/devices/kick_909/kick_909_device.hpp"

#include <QTest>

#include <cmath>
#include <memory>

namespace noteahead {

namespace {

void putNoteOn(Song & song, size_t pattern, size_t track, size_t line, uint8_t note, uint8_t velocity = 100)
{
    NoteData data { track, 0 };
    data.setAsNoteOn(note, velocity);
    song.setNoteDataAtPosition(data, { .pattern = pattern, .track = track, .column = 0, .line = line });
}

} // namespace

void SongContextAnalyzerTest::test_edmSongWith909AndBass_shouldDetectFourOnFloorAndBassHz()
{
    DeviceFactory::init();
    const auto engine { std::make_shared<AudioEngine>() };
    DeviceService deviceService { engine, std::make_shared<DataService>() };

    const auto kickPort { Constants::internalDevicePortPrefix() + " 1" };
    const auto bassPort { Constants::internalDevicePortPrefix() + " 2" };
    const auto leadPort { Constants::internalDevicePortPrefix() + " 3" };
    deviceService.setDevice(0, DeviceFactory::createDevice(Kick909Device::typeIdString(), kickPort.toStdString()));
    deviceService.setDevice(1, DeviceFactory::createDevice(BassSynthDevice::typeIdString(), bassPort.toStdString()));
    deviceService.setDevice(2, DeviceFactory::createDevice(FmSynthDevice::typeIdString(), leadPort.toStdString()));

    Song song;
    song.setBeatsPerMinute(138);
    song.setLinesPerBeat(4);
    song.setInstrument(0, std::make_shared<Instrument>(kickPort));
    song.setInstrument(1, std::make_shared<Instrument>(bassPort));
    song.setInstrument(2, std::make_shared<Instrument>(leadPort));

    // 4-on-the-floor kick on lines 0, 4, 8, 12 (pattern 0 has 16 lines = 4 beats)
    song.setLineCount(0, 16);
    for (size_t line = 0; line < 16; line += 4) {
        putNoteOn(song, 0, 0, line, 36);
    }
    // Bass playing E1 (MIDI 28 ~ 41.20 Hz)
    putNoteOn(song, 0, 1, 0, 28);
    // FM lead on C4 (MIDI 60)
    putNoteOn(song, 0, 2, 2, 60);

    const auto ctx { SongContextAnalyzer::analyze(song, deviceService) };
    QVERIFY(ctx.hasSongData);
    QCOMPARE(ctx.bpm, 138.0);
    QVERIFY(ctx.isFourOnTheFloor);
    QVERIFY(ctx.hasPunchyKick);
    QVERIFY(ctx.hasSubOr808);
    QVERIFY(ctx.hasBrightOrFmSynths);
    QVERIFY(std::abs(ctx.lowestBassHz - 41.20) < 0.5);
    QCOMPARE(ctx.activeInstrumentCount, size_t { 3 });
    QVERIFY(!ctx.summaryText.empty());
}

void SongContextAnalyzerTest::test_trapSongWith808AndSidechain_shouldDetectSubAndDucking()
{
    DeviceFactory::init();
    const auto engine { std::make_shared<AudioEngine>() };
    DeviceService deviceService { engine, std::make_shared<DataService>() };

    const auto port808 { Constants::internalDevicePortPrefix() + " 1" };
    deviceService.setDevice(0, DeviceFactory::createDevice(Kick808Device::typeIdString(), port808.toStdString()));
    Song song;
    song.setBeatsPerMinute(140);
    song.setLinesPerBeat(4);
    song.setInstrument(0, std::make_shared<Instrument>(port808));
    song.setLineCount(0, 16);
    putNoteOn(song, 0, 0, 0, 29); // F1 ~ 43.65 Hz

    SideChainService scService;
    SideChainSettings sc;
    sc.enabled = true;
    scService.setSettings(1, sc);

    const auto ctx { SongContextAnalyzer::analyze(song, deviceService, &scService) };
    QVERIFY(ctx.hasSubOr808);
    QVERIFY(ctx.hasSidechainDucking);
    QVERIFY(std::abs(ctx.lowestBassHz - 43.65) < 0.6);
}

void SongContextAnalyzerTest::test_externalMidiFallback_shouldStillExtractRhythmAndPitch()
{
    const auto engine { std::make_shared<AudioEngine>() };
    DeviceService deviceService { engine, std::make_shared<DataService>() };

    Song song;
    song.setBeatsPerMinute(125);
    song.setLinesPerBeat(4);
    song.setLineCount(0, 16);
    auto drumInst { std::make_shared<Instrument>("External MIDI Port 1") };
    InstrumentSettings drumSettings;
    drumSettings.drumTrack = true;
    drumInst->setSettings(drumSettings);
    song.setInstrument(0, drumInst);

    auto bassInst { std::make_shared<Instrument>("External MIDI Port 2") };
    song.setInstrument(1, bassInst);

    for (size_t line = 0; line < 16; line += 4) {
        putNoteOn(song, 0, 0, line, 36);
    }
    putNoteOn(song, 0, 1, 0, 33); // A1 = 55 Hz

    const auto ctx { SongContextAnalyzer::analyze(song, deviceService) };
    QVERIFY(ctx.hasSongData);
    QCOMPARE(ctx.bpm, 125.0);
    QVERIFY(ctx.isFourOnTheFloor);
    QVERIFY(std::abs(ctx.lowestBassHz - 55.0) < 0.5);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SongContextAnalyzerTest)
