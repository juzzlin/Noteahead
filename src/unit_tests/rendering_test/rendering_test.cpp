#include "rendering_test.hpp"

#include "application/service/device_service.hpp"
#include "application/service/mixer_service.hpp"
#include "application/service/render_worker.hpp"
#include "common/constants.hpp"
#include "domain/devices/device_factory.hpp"
#include "domain/devices/drum_synth_device.hpp"
#include "domain/effects/effect_factory.hpp"
#include "domain/devices/sampler_device.hpp"
#include "domain/devices/synth_device.hpp"
#include "domain/tracker/event.hpp"
#include "domain/tracker/instrument.hpp"
#include "domain/tracker/note_data.hpp"
#include "domain/midi/pitch_bend_data.hpp"
#include "infra/audio/audio_engine.hpp"
#include "infra/audio/backend/audio_file_reader.hpp"

#include "application/service/automation_service.hpp"
#include "application/service/property_service.hpp"
#include "application/service/side_chain_service.hpp"
#include "domain/tracker/pattern.hpp"
#include "domain/tracker/song.hpp"

#include <QTest>

#include <cmath>
#include <mutex>
#include <span>
#include <vector>

namespace noteahead {

void RenderingTest::initTestCase()
{
    EffectFactory::init();
    DeviceFactory::init();
}

void RenderingTest::cleanupTestCase()
{
    EffectFactory::clear();
    DeviceFactory::clear();
}

class MockRenderIo : public AudioFileReader
{
public:
    bool open(const std::string &, Mode mode, Info & info) override
    {
        m_mode = mode;
        m_info = info;
        m_isOpen = true;
        return true;
    }

    void close() override
    {
        m_isOpen = false;
    }

    int64_t readFloat(std::span<float>) override
    {
        return 0;
    }

    int64_t readDouble(std::span<double>) override
    {
        return 0;
    }

    int64_t readInt(std::span<int32_t>) override
    {
        return 0;
    }

    int64_t writeFloat(std::span<const float> data) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.insert(m_data.end(), data.begin(), data.end());
        return static_cast<int64_t>(data.size() / 2);
    }

    int64_t writeInt(std::span<const int32_t>) override
    {
        return 0;
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

    const std::vector<float> & data() const
    {
        return m_data;
    }

private:
    bool m_isOpen = false;
    Mode m_mode = Mode::Write;
    Info m_info;
    std::vector<float> m_data;
    std::mutex m_mutex;
};

void RenderingTest::test_renderSynth_shouldPreserveParameters()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine);
    auto mixerService = std::make_shared<MixerService>();

    auto synth = std::make_shared<SynthDevice>("Noteahead Synth");
    synth->setLpfCutoff(0.5f);
    synth->setGain(0.75f);
    deviceService->setDevice(0, synth);

    RenderWorker worker(audioEngine, deviceService, mixerService);
    worker.setAudioFileReaderFactory([]() { return std::make_unique<MockRenderIo>(); });

    RenderWorker::EventList events;
    auto instrument = std::make_shared<Instrument>("Noteahead Synth");
    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(60, 100);
    auto event = std::make_shared<Event>(0, noteData);
    event->setInstrument(instrument);
    events.push_back(event);

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    worker.render("dummy.wav", events, timing, 24, 44100);

    QCOMPARE(synth->lpfCutoff(), 0.5f);
    QCOMPARE(synth->gain(), 0.75f);
}

void RenderingTest::test_renderSynth_shouldNotBeSilent()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine);
    auto mixerService = std::make_shared<MixerService>();

    auto synth = std::make_shared<SynthDevice>("Noteahead Synth");
    synth->setLpfCutoff(1.0f);
    synth->setGain(0.5f); // 0 dB
    synth->setVolume(1.0f);
    deviceService->setDevice(0, synth);

    RenderWorker worker(audioEngine, deviceService, mixerService);

    MockRenderIo * mockIoPtr = nullptr;
    worker.setAudioFileReaderFactory([&]() {
        auto io = std::make_unique<MockRenderIo>();
        mockIoPtr = io.get();
        return io;
    });

    RenderWorker::EventList events;
    auto instrument = std::make_shared<Instrument>("Noteahead Internal Device 1");
    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(60, 100);
    auto event = std::make_shared<Event>(0, noteData);
    event->setInstrument(instrument);
    events.push_back(event);

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    worker.render("dummy.wav", events, timing, 48, 44100);

    QVERIFY(mockIoPtr != nullptr);
    const auto & audioData = mockIoPtr->data();

    bool hasSound = false;
    float maxAmp = 0.0f;
    for (float s : audioData) {
        maxAmp = std::max(maxAmp, std::abs(s));
        if (std::abs(s) > 0.001f) {
            hasSound = true;
            break;
        }
    }

    if (!hasSound) {
        qDebug() << "Max amplitude detected:" << maxAmp;
    }
    QVERIFY(hasSound);
}

void RenderingTest::test_renderSampler_shouldPreserveParameters()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine);
    auto mixerService = std::make_shared<MixerService>();

    auto sampler = std::make_shared<SamplerDevice>("Noteahead Sampler");
    sampler->setVolume(0.33f);
    sampler->setGain(0.88f);
    deviceService->setDevice(0, sampler);

    RenderWorker worker(audioEngine, deviceService, mixerService);
    worker.setAudioFileReaderFactory([]() { return std::make_unique<MockRenderIo>(); });

    RenderWorker::EventList events;
    auto instrument = std::make_shared<Instrument>("Noteahead Sampler");
    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(60, 100);
    auto event = std::make_shared<Event>(0, noteData);
    event->setInstrument(instrument);
    events.push_back(event);

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    worker.render("dummy.wav", events, timing, 24, 44100);

    QCOMPARE(sampler->volume(), 0.33f);
    QCOMPARE(sampler->gain(), 0.88f);
}

void RenderingTest::test_renderDrumSynth_shouldPreserveParameters()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine);
    auto mixerService = std::make_shared<MixerService>();

    auto drumSynth = std::make_shared<DrumSynthDevice>("Noteahead DrumSynth");
    drumSynth->setPan(0.25f);
    drumSynth->setGain(0.99f);
    deviceService->setDevice(0, drumSynth);

    RenderWorker worker(audioEngine, deviceService, mixerService);
    worker.setAudioFileReaderFactory([]() { return std::make_unique<MockRenderIo>(); });

    RenderWorker::EventList events;
    auto instrument = std::make_shared<Instrument>("Noteahead DrumSynth");
    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(36, 100);
    auto event = std::make_shared<Event>(0, noteData);
    event->setInstrument(instrument);
    events.push_back(event);

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    worker.render("dummy.wav", events, timing, 24, 44100);

    QCOMPARE(drumSynth->pan(), 0.25f);
    QCOMPARE(drumSynth->gain(), 0.99f);
}

void RenderingTest::test_render_shouldNotCrashWithNullInstrumentEvents()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine);
    auto mixerService = std::make_shared<MixerService>();

    RenderWorker worker(audioEngine, deviceService, mixerService);
    worker.setAudioFileReaderFactory([]() { return std::make_unique<MockRenderIo>(); });

    RenderWorker::EventList events;
    // Add an event WITHOUT instrument (e.g. StartOfSong)
    auto event = std::make_shared<Event>(0);
    event->setAsStartOfSong();
    events.push_back(event);

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    // This should not crash
    worker.render("dummy.wav", events, timing, 24, 44100);
}

void RenderingTest::test_render_shouldClampSignal()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine);
    auto mixerService = std::make_shared<MixerService>();

    auto drumSynth = std::make_shared<DrumSynthDevice>("Drums");
    drumSynth->setVolume(1.0f);
    drumSynth->setGain(1.0f); // 0 dB
    deviceService->setDevice(0, drumSynth);

    RenderWorker worker(audioEngine, deviceService, mixerService);
    MockRenderIo * mockIoPtr = nullptr;
    worker.setAudioFileReaderFactory([&]() {
        auto io = std::make_unique<MockRenderIo>();
        mockIoPtr = io.get();
        return io;
    });

    RenderWorker::EventList events;
    auto instrument = std::make_shared<Instrument>("Noteahead Internal Device 1");

    // Kick (36), Snare (38) and Clap (39) at the same time with full velocity
    for (uint8_t note : { 36, 38, 39 }) {
        NoteData noteData { 0, 0 };
        noteData.setAsNoteOn(note, 127);
        auto event = std::make_shared<Event>(0, noteData);
        event->setInstrument(instrument);
        events.push_back(event);
    }

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    worker.render("dummy.wav", events, timing, 48, 44100);

    QVERIFY(mockIoPtr != nullptr);
    const auto & audioData = mockIoPtr->data();
    QVERIFY(!audioData.empty());

    bool foundLargeSignal = false;
    for (float s : audioData) {
        QVERIFY2(s >= -1.0f && s <= 1.0f, qPrintable(QString("Sample out of range: %1").arg(static_cast<double>(s))));
        if (std::abs(s) > 0.999f) {
            foundLargeSignal = true;
        }
    }
    QVERIFY2(foundLargeSignal, "Should have found samples close to 1.0 due to clamping");
}

void RenderingTest::test_render_midiSideChain_shouldProcessEventWhenSourceTrackIsMuted()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine);
    auto mixerService = std::make_shared<MixerService>();
    auto propertyService = std::make_shared<PropertyService>();
    auto automationService = std::make_shared<AutomationService>(propertyService);
    auto sideChainService = std::make_shared<SideChainService>();

    auto song = std::make_shared<Song>();
    song->initialize();
    song->addTrackToRightOf(0); // Track 0 and Track 1

    auto instrument0 = std::make_shared<Instrument>(Constants::internalDevicePortPrefix() + " 1");
    song->setInstrument(0, instrument0);

    auto instrument1 = std::make_shared<Instrument>(Constants::internalDevicePortPrefix() + " 2");
    song->setInstrument(1, instrument1);

    auto synth1 = std::make_shared<SynthDevice>("Target Synth");
    synth1->setLpfCutoff(0.5f);
    deviceService->setDevice(1, synth1);

    SideChainSettings settings;
    settings.enabled = true;
    settings.sourceTrackIndex = 0;
    settings.sourceColumnIndex = 0;
    settings.lookahead = std::chrono::milliseconds { 0 };
    settings.release = std::chrono::milliseconds { 100 };
    settings.targets.push_back({ true, 74, 127, 0 }); // CC 74 (Cutoff) -> 127
    sideChainService->setSettings(1, settings);

    // Add a NoteOn on Track 0 at tick 0 to trigger the side-chain
    NoteData triggerNote { 0, 0 };
    triggerNote.setAsNoteOn(60, 100);
    const auto patternIndex = song->patternAtSongPosition(0);
    song->pattern(patternIndex)->setNoteDataAtPosition(triggerNote, { 0, 0, 0, 0, 0 });

    // Solo Track 1, so Track 0 is muted
    mixerService->soloTrack(1, true);

    QVERIFY(mixerService->shouldTrackPlay(1));
    QVERIFY(!mixerService->shouldTrackPlay(0));

    // Render to events
    const auto events = song->renderToEvents(automationService, sideChainService, 0);

    // Verify CC event exists
    bool foundCcEvent = false;
    for (const auto & event : events) {
        if (event->type() == Event::Type::MidiCcData) {
            if (auto data = event->midiCcData(); data && data->track() == 1 && data->controller() == 74) {
                foundCcEvent = true;
                break;
            }
        }
    }
    QVERIFY(foundCcEvent);

    // Process events through RenderWorker
    RenderWorker worker(audioEngine, deviceService, mixerService);
    worker.setAudioFileReaderFactory([]() { return std::make_unique<MockRenderIo>(); });

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    // Process only first tick where CC is expected
    worker.render("dummy.wav", events, timing, 1, 44100);

    // Verify cutoff was updated to 1.0f (from CC 127)
    QCOMPARE(synth1->lpfCutoff(), 1.0f);
}

void RenderingTest::test_render_pitchBend_shouldProcessEvent()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine);
    const auto mixerService = std::make_shared<MixerService>();

    const auto portName = Constants::internalDevicePortPrefix() + " 1";
    const auto synth = std::make_shared<SynthDevice>("Test Synth");
    synth->setPitchBendRange(12);
    deviceService->setDevice(0, synth);

    const auto instrument = std::make_shared<Instrument>(portName);
    RenderWorker::EventList events;

    // Pitch bend to maximum (16383)
    const PitchBendData pbData { 0, 0, static_cast<uint16_t>(16383) };
    const auto event = std::make_shared<Event>(0, pbData);
    event->setInstrument(instrument);
    events.push_back(event);

    RenderWorker worker { audioEngine, deviceService, mixerService };
    worker.setAudioFileReaderFactory([]() { return std::make_unique<MockRenderIo>(); });

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    worker.render("dummy.wav", events, timing, 1, 44100);

    // With range 12, max pitch bend should be approximately +12.0f
    // We use a small delta to account for MIDI resolution (14-bit) and float precision
    const float offset = synth->currentPitchBendOffset();
    QVERIFY2(std::abs(offset - 12.0f) < 0.01f, qPrintable(QString { "Pitch bend offset %1 is not close to 12.0" }.arg(static_cast<double>(offset))));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RenderingTest)
