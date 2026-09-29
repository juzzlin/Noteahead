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

#include "audio_service_test.hpp"

#include "../../application/service/audio_service.hpp"
#include "../../application/service/jack_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../domain/dsp/metronome.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QSignalSpy>
#include <QTest>

namespace noteahead {

namespace {

//! An AudioService that has not opened anything: autoInitialize off means no worker and no stream, so
//! what is left is the bookkeeping these tests are about. startRecording() logs that it could not
//! reach the worker and carries on, which is exactly the part under test.
std::unique_ptr<AudioService> makeAudioService()
{
    const auto settingsService = std::make_shared<SettingsService>();
    const auto audioEngine = std::make_shared<AudioEngine>();
    return std::make_unique<AudioService>(settingsService,
                                          std::make_shared<JackService>(settingsService, audioEngine),
                                          audioEngine,
                                          nullptr,
                                          false);
}

QString serialized(const AudioService & service)
{
    QString xml;
    NahdXmlWriter writer { xml };
    writer.writeStartElement("Song");
    service.serializeToXml(writer);
    writer.writeEndElement();
    return xml;
}

} // namespace

void AudioServiceTest::test_stopRecording_songTake_shouldBecomeTheLatestRecording()
{
    const auto service = makeAudioService();
    QSignalSpy spy { service.get(), &AudioService::latestRecordingFileNameChanged };

    service->startRecording("/tmp/song_take.wav", 256, 128);
    service->stopRecording(512);

    QCOMPARE(service->latestRecordingFileName(), QString { "/tmp/song_take.wav" });
    QCOMPARE(service->latestRecordingStartTick(), static_cast<quint64>(128));
    QCOMPARE(service->latestRecordingEndTick(), static_cast<quint64>(512));
    QCOMPARE(spy.count(), 1);
}

void AudioServiceTest::test_stopRecording_sampleTake_shouldNotTouchTheLatestRecording()
{
    // A Sampler pad take is a sample, not a take of the song. It used to land in the song's latest
    // recording all the same, which put it in the wave view under the editor, made the play button
    // there play it, and mapped it onto the song's ticks from a start tick it never had.
    const auto service = makeAudioService();
    service->startRecording("/tmp/song_take.wav", 256, 128);
    service->stopRecording(512);

    QSignalSpy spy { service.get(), &AudioService::latestRecordingFileNameChanged };
    service->startSampleRecording("/tmp/pad_take.wav", 0);
    service->stopRecording(0);

    // The song's take is still the song's take.
    QCOMPARE(service->latestRecordingFileName(), QString { "/tmp/song_take.wav" });
    QCOMPARE(service->latestRecordingStartTick(), static_cast<quint64>(128));
    QCOMPARE(service->latestRecordingEndTick(), static_cast<quint64>(512));
    QCOMPARE(spy.count(), 0);
}

void AudioServiceTest::test_stopRecording_sampleTake_shouldNotBeSerialized()
{
    // The reason this is more than cosmetic: the latest recording is written into the project, so a
    // pad take recorded before the project was saved left a path behind that stops existing with the
    // temporary directory it was written into.
    const auto service = makeAudioService();
    service->startSampleRecording("/tmp/pad_take.wav", 0);
    service->stopRecording(0);

    const auto xml = serialized(*service);
    QVERIFY2(!xml.contains("pad_take.wav"), qPrintable(xml));
}

void AudioServiceTest::test_beatsToSeconds_shouldFollowTheEngineTempo()
{
    // What a take is trimmed by, so that it starts on the downbeat rather than on the count-in.
    const auto engine = std::make_shared<AudioEngine>();
    const auto settingsService = std::make_shared<SettingsService>();
    AudioService service { settingsService, std::make_shared<JackService>(settingsService, engine), engine, nullptr, false };

    engine->setBpm(120.0f);
    QVERIFY(qFuzzyCompare(service.beatsToSeconds(4), 2.0)); // four beats at two a second

    engine->setBpm(60.0f);
    QVERIFY(qFuzzyCompare(service.beatsToSeconds(4), 4.0));
    QCOMPARE(service.beatsToSeconds(0), 0.0);
}

void AudioServiceTest::test_startMetronome_shouldRunTheEngineClickAndCountIn()
{
    const auto engine = std::make_shared<AudioEngine>();
    const auto settingsService = std::make_shared<SettingsService>();
    AudioService service { settingsService, std::make_shared<JackService>(settingsService, engine), engine, nullptr, false };

    QVERIFY(!engine->metronome().running());

    service.startMetronome(4, 4, 0.5);
    QVERIFY(engine->metronome().running());
    QCOMPARE(service.metronomeCountInBeatsRemaining(), 4);
    QVERIFY(!service.metronomeCountInFinished());

    service.stopMetronome();
    QVERIFY(!engine->metronome().running());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AudioServiceTest)
