#include "rendering_test.hpp"

#include "../../application/service/render_worker.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/event.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/instrument.hpp"
#include "../../common/constants.hpp"

#include <QTest>
#include <cmath>
#include <span>
#include <vector>
#include <mutex>

namespace noteahead {

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
    void close() override { m_isOpen = false; }
    int64_t readFloat(std::span<float>) override { return 0; }
    int64_t readDouble(std::span<double>) override { return 0; }
    int64_t readInt(std::span<int32_t>) override { return 0; }
    int64_t writeFloat(std::span<const float> data) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.insert(m_data.end(), data.begin(), data.end());
        return static_cast<int64_t>(data.size() / 2);
    }
    int64_t writeInt(std::span<const int32_t>) override { return 0; }
    bool seek(int64_t, int) override { return true; }
    bool isOpen() const override { return m_isOpen; }
    Info info() const override { return m_info; }

    const std::vector<float> & data() const { return m_data; }

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

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RenderingTest)
