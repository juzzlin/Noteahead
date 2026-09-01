#include "rendering_test.hpp"

#include "../../application/service/device_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../application/service/render_worker.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/bass_synth_device.hpp"
#include "../../domain/devices/device_factory.hpp"
#include "../../domain/devices/drum_synth_constants.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/devices/kick_808_device.hpp"
#include "../../domain/devices/piano_synth_device.hpp"
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
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/midi/pitch_bend_data.hpp"
#include "../../domain/tracker/event.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../domain/tracker/note_data.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"
#include "../../infra/data_service.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../domain/tracker/pattern.hpp"
#include "../../domain/tracker/song.hpp"

#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
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
    struct FileEntry
    {
        std::vector<float> data;
        Info info;
    };

    using Registry = std::map<std::string, FileEntry>;

    MockRenderIo(Registry * registry = nullptr, std::mutex * registryMutex = nullptr)
      : m_registry { registry }
      , m_registryMutex { registryMutex }
    {
    }

    bool open(const std::string & filePath, Mode mode, Info & info) override
    {
        m_filePath = filePath;
        m_mode = mode;
        m_isOpen = true;
        m_readPos = 0;

        if (m_registry) {
            std::lock_guard<std::mutex> lock(*m_registryMutex);
            auto & entry = (*m_registry)[m_filePath];
            if (mode == Mode::Write) {
                entry.info = info;
            } else {
                info = entry.info;
            }
            m_info = entry.info;
        } else {
            if (mode == Mode::Write) {
                m_info = info;
            } else {
                info.channels = 2;
                info.samplerate = 44100;
                m_info = info;
            }
        }
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
        if (m_registry) {
            std::lock_guard<std::mutex> lock(*m_registryMutex);
            auto & vec = (*m_registry)[m_filePath].data;
            if (m_readPos >= vec.size()) {
                return 0;
            }
            const size_t toRead = std::min(data.size(), vec.size() - m_readPos);
            std::copy(vec.begin() + m_readPos, vec.begin() + m_readPos + toRead, data.begin());
            m_readPos += toRead;
            return static_cast<int64_t>(toRead / 2); // returns frames
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_readPos >= m_data.size()) {
            return 0;
        }
        const size_t toRead = std::min(data.size(), m_data.size() - m_readPos);
        std::copy(m_data.begin() + m_readPos, m_data.begin() + m_readPos + toRead, data.begin());
        m_readPos += toRead;
        return static_cast<int64_t>(toRead / 2); // returns frames
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
        if (m_registry) {
            std::lock_guard<std::mutex> lock(*m_registryMutex);
            auto & vec = (*m_registry)[m_filePath].data;
            vec.insert(vec.end(), data.begin(), data.end());
            return static_cast<int64_t>(data.size() / 2);
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.insert(m_data.end(), data.begin(), data.end());
        return static_cast<int64_t>(data.size() / 2);
    }

    int64_t writeInt(std::span<const int32_t>) override
    {
        return 0;
    }

    bool seek(int64_t frames, int whence) override
    {
        if (m_registry) {
            std::lock_guard<std::mutex> lock(*m_registryMutex);
            auto & vec = (*m_registry)[m_filePath].data;
            size_t targetPos = 0;
            if (whence == 0) { // SEEK_SET
                targetPos = static_cast<size_t>(frames * 2);
            } else if (whence == 1) { // SEEK_CUR
                targetPos = m_readPos + static_cast<size_t>(frames * 2);
            } else if (whence == 2) { // SEEK_END
                targetPos = vec.size() + static_cast<size_t>(frames * 2);
            }
            if (targetPos > vec.size()) {
                return false;
            }
            m_readPos = targetPos;
            return true;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        size_t targetPos = 0;
        if (whence == 0) { // SEEK_SET
            targetPos = static_cast<size_t>(frames * 2);
        } else if (whence == 1) { // SEEK_CUR
            targetPos = m_readPos + static_cast<size_t>(frames * 2);
        } else if (whence == 2) { // SEEK_END
            targetPos = m_data.size() + static_cast<size_t>(frames * 2);
        }
        if (targetPos > m_data.size()) {
            return false;
        }
        m_readPos = targetPos;
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
        if (m_registry) {
            std::lock_guard<std::mutex> lock(*m_registryMutex);
            return (*m_registry)[m_filePath].data;
        }
        return m_data;
    }

private:
    bool m_isOpen = false;
    Mode m_mode = Mode::Write;
    Info m_info;
    std::string m_filePath;
    std::vector<float> m_data;
    size_t m_readPos = 0;
    std::mutex m_mutex;
    Registry * m_registry = nullptr;
    std::mutex * m_registryMutex = nullptr;
};

void RenderingTest::test_renderSynth_shouldPreserveParameters()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
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

namespace {

//! Renders one note on one device and returns the peak amplitude of the result.
//!
//! Shared by the coverage test below so that adding an instrument to it costs a row rather than
//! fifty lines, which is what the two hand-written copies this replaced had grown into.
float renderSingleNotePeak(std::shared_ptr<Device> device, uint8_t note)
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    auto mixerService = std::make_shared<MixerService>();
    deviceService->setDevice(0, std::move(device));

    RenderWorker worker(audioEngine, deviceService, mixerService);

    MockRenderIo * mockIo = nullptr;
    worker.setAudioFileReaderFactory([&]() {
        auto io = std::make_unique<MockRenderIo>();
        mockIo = io.get();
        return io;
    });

    RenderWorker::EventList events;
    auto instrument = std::make_shared<Instrument>("Noteahead Internal Device 1");
    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(note, 100);
    auto event = std::make_shared<Event>(0, noteData);
    event->setInstrument(instrument);
    events.push_back(event);

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    worker.render("dummy.wav", events, timing, 48, 44100);

    if (!mockIo) {
        return 0.0f;
    }

    float peak = 0.0f;
    for (float sample : mockIo->data()) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

} // namespace

void RenderingTest::test_render_lateNote_shouldStartOnItsOwnTick()
{
    // Event-free ticks are rendered as one block rather than one block each, so the note has to
    // land on the tick it was written on and not on the start of the block that swallowed it.
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    auto mixerService = std::make_shared<MixerService>();
    deviceService->setDevice(0, std::make_shared<SynthDevice>("Noteahead Internal Device 1"));

    RenderWorker worker(audioEngine, deviceService, mixerService);
    // The registry outlives the reader, which the recorder owns and destroys inside render().
    MockRenderIo::Registry registry;
    std::mutex registryMutex;
    worker.setAudioFileReaderFactory([&]() { return std::make_unique<MockRenderIo>(&registry, &registryMutex); });

    // Far enough in that the run-up is many blocks long however the ticks are grouped.
    const quint64 noteTick = 300;

    RenderWorker::EventList events;
    auto instrument = std::make_shared<Instrument>("Noteahead Internal Device 1");
    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(60, 100);
    auto event = std::make_shared<Event>(noteTick, noteData);
    event->setInstrument(instrument);
    events.push_back(event);

    // Real tracker timing: a tick is ~93 frames, so a block spans a couple of dozen of them and
    // a note allowed to drift to a block boundary would land audibly late.
    RenderWorker::Timing timing;
    timing.beatsPerMinute = 148;
    timing.linesPerBeat = 8;
    timing.ticksPerLine = 24;

    const uint32_t sampleRate = 44100;
    worker.render("dummy.wav", events, timing, noteTick + 200, sampleRate);

    const auto & rendered = registry["dummy.wav"].data;
    QVERIFY(!rendered.empty());
    const double samplesPerTick = 60.0 * sampleRate / (timing.beatsPerMinute * timing.linesPerBeat * timing.ticksPerLine);
    const auto expectedFrame = static_cast<size_t>(static_cast<double>(noteTick) * samplesPerTick);

    const auto firstSoundingFrame = [&] {
        for (size_t frame = 0; frame * 2 + 1 < rendered.size(); frame++) {
            if (std::abs(rendered[frame * 2]) > 1.0e-6f || std::abs(rendered[frame * 2 + 1]) > 1.0e-6f) {
                return frame;
            }
        }
        return rendered.size();
    }();

    QVERIFY2(firstSoundingFrame != rendered.size(), "the render was silent");
    // One tick of slack: which side of a tick boundary the fractional sample counter lands on is
    // not the point, being blocks early or late is.
    const auto slack = static_cast<size_t>(samplesPerTick);
    QVERIFY2(firstSoundingFrame + slack >= expectedFrame && firstSoundingFrame <= expectedFrame + slack,
             qPrintable(QString { "note started at frame %1, expected %2" }.arg(firstSoundingFrame).arg(expectedFrame)));
}

void RenderingTest::test_render_everyInstrument_shouldNotBeSilent_data()
{
    QTest::addColumn<QString>("typeId");
    QTest::addColumn<int>("note");

    // Every internal instrument, so that one which renders silent -- or which never reaches the
    // render path at all, having been left out of the device factory -- is caught here rather than
    // in a finished export.
    //
    // Two devices are deliberately absent. The Sampler has nothing to play until a sample is loaded,
    // and the Sub Mixer is not an instrument at all: it sums other devices' outputs and has no voice
    // of its own.
    const auto addInstrument = [](const char * name, const std::string & typeId, int note = 60) {
        QTest::newRow(name) << QString::fromStdString(typeId) << note;
    };

    addInstrument("Synth", SynthDevice::typeIdString());
    addInstrument("Wavetable Synth", WavetableSynthDevice::typeIdString());
    addInstrument("Bass Synth", BassSynthDevice::typeIdString());
    addInstrument("Piano Synth", PianoSynthDevice::typeIdString());
    addInstrument("Piano Synth V2", PianoSynthV2Device::typeIdString());
    addInstrument("Piano Synth V3", PianoSynthV3Device::typeIdString());
    addInstrument("String & Voice", StringVoiceDevice::typeIdString());
    addInstrument("String & Voice V2", StringVoiceV2Device::typeIdString());
    addInstrument("String Ensemble", StringEnsembleDevice::typeIdString());
    addInstrument("Speech", SpeechDevice::typeIdString());
    addInstrument("Kick 808", Kick808Device::typeIdString());

    // The drum machine is a kit rather than a keyboard: only the notes its pads are mapped to make
    // any sound, and 60 is not one of them.
    addInstrument("Drum Synth", DrumSynthDevice::typeIdString(), static_cast<int>(DrumSynth::MidiNote::Kick));
}

void RenderingTest::test_render_everyInstrument_shouldNotBeSilent()
{
    QFETCH(QString, typeId);
    QFETCH(int, note);

    DeviceFactory::init();
    const auto device = DeviceFactory::createDevice(typeId.toStdString(), "Noteahead Internal Device 1");
    QVERIFY2(device, qPrintable(typeId));

    const float peak = renderSingleNotePeak(device, static_cast<uint8_t>(note));
    QVERIFY2(peak > 0.001f, qPrintable(QString { "peak %1" }.arg(peak)));
}

void RenderingTest::test_renderSampler_shouldPreserveParameters()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
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
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
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
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
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
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
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
        if (std::abs(s) > 0.5f) {
            foundLargeSignal = true;
        }
    }
    QVERIFY2(foundLargeSignal, "Should have found large samples from simultaneous drum hits");
}

void RenderingTest::test_render_shouldNotDependOnSessionState()
{
    // A render is a function of the project and nothing else. Whatever the session did first --
    // an automation left mid-sweep, a controller knob moved, a take stopped half way -- the same
    // song has to come out the same. It did not: the devices kept whatever MIDI CC had last put
    // them at, and the export drifted with the session.
    const auto renderWith = [](bool polluteFirst) {
        const auto audioEngine = std::make_shared<AudioEngine>();
        const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
        const auto mixerService = std::make_shared<MixerService>();

        const auto synth = std::make_shared<SynthDevice>("Lead");
        synth->setVco1Sync(true); // Phase randomisation off, so two renders are comparable at all
        synth->setLpfCutoff(0.6f);
        synth->setMixVco1(1.0f);
        synth->setVolume(1.0f);
        synth->setGain(0.5f);
        deviceService->setDevice(0, synth);

        // The lead is summed through a SubMixer. A SubMixer plays no notes, so no instrument points
        // at it and it never receives the reset that goes out with a track's settings: whatever
        // automation last left on its fader is still there when the render starts. That is the hole
        // this test is about, and a device the song plays through directly would not show it.
        const auto subMixer = std::make_shared<SubMixerDevice>("Bus");
        deviceService->setDevice(1, subMixer);
        const bool routed = deviceService->addSubMixerMember(1, 0);
        Q_ASSERT(routed);
        Q_UNUSED(routed);

        if (polluteFirst) {
            // What an earlier playback would have left behind: the bus faded out and never restored.
            subMixer->processMidiCc(7, 0, 0);
        }

        RenderWorker worker { audioEngine, deviceService, mixerService };
        MockRenderIo::Registry registry;
        std::mutex registryMutex;
        worker.setAudioFileReaderFactory([&]() { return std::make_unique<MockRenderIo>(&registry, &registryMutex); });

        RenderWorker::EventList events;
        const auto instrument = std::make_shared<Instrument>(Constants::internalDevicePortPrefix() + " 1");
        NoteData noteData { 0, 0 };
        noteData.setAsNoteOn(60, 100);
        const auto event = std::make_shared<Event>(0, noteData);
        event->setInstrument(instrument);
        events.push_back(event);

        RenderWorker::Timing timing;
        timing.beatsPerMinute = 120;
        timing.linesPerBeat = 4;
        timing.ticksPerLine = 6;

        worker.render("dummy.wav", events, timing, 24, 48000);
        return registry["dummy.wav"].data;
    };

    const auto clean = renderWith(false);
    const auto afterAutomation = renderWith(true);

    QVERIFY(!clean.empty());
    QCOMPARE(afterAutomation.size(), clean.size());

    double difference = 0.0;
    for (size_t i = 0; i < clean.size(); i++) {
        difference = std::max(difference, std::abs(static_cast<double>(clean.at(i) - afterAutomation.at(i))));
    }
    QVERIFY2(difference < 1.0e-6, QString("Renders differed by %1").arg(difference).toUtf8().constData());
}

void RenderingTest::test_render_shouldApplyTrackInstrumentSettings()
{
    // A render used to start from whatever the session had left the devices holding: the track's
    // own MIDI CC settings, which playback applies on every rewind, never reached it. An export
    // could then differ from what had just been heard.
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto mixerService = std::make_shared<MixerService>();
    const auto propertyService = std::make_shared<PropertyService>();
    const auto automationService = std::make_shared<AutomationService>(propertyService);
    const auto sideChainService = std::make_shared<SideChainService>();

    const auto synth = std::make_shared<SynthDevice>("Rendered Synth");
    synth->setLpfCutoff(0.25f);
    deviceService->setDevice(0, synth);

    auto song = std::make_shared<Song>();
    song->initialize();

    const auto instrument = std::make_shared<Instrument>(Constants::internalDevicePortPrefix() + " 1");
    InstrumentSettings settings;
    settings.midiCcSettings.push_back({ true, 74, 127 }); // Cutoff wide open
    instrument->setSettings(settings);
    song->setInstrument(0, instrument);

    NoteData note { 0, 0 };
    note.setAsNoteOn(60, 100);
    song->pattern(song->patternAtSongPosition(0))->setNoteDataAtPosition(note, { 0, 0, 0, 0, 0 });

    const auto events = song->renderToEvents(automationService, sideChainService, 0);

    RenderWorker worker(audioEngine, deviceService, mixerService);
    worker.setAudioFileReaderFactory([]() { return std::make_unique<MockRenderIo>(); });

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    QSignalSpy parameterSpy { synth.get(), &Device::parametersChanged };

    worker.render("dummy.wav", events, timing, 1, 44100);

    // The settings reached the device...
    QVERIFY(parameterSpy.count() > 0);

    // ...and the render handed the patch back on the way out, as it does for automation.
    QCOMPARE(synth->lpfCutoff(), 0.25f);
}

void RenderingTest::test_render_midiSideChain_shouldProcessEventWhenSourceTrackIsMuted()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
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

    // Nothing else in this render moves a parameter, so an emission proves the CC arrived
    QSignalSpy parameterSpy { synth1.get(), &Device::parametersChanged };

    // Process only first tick where CC is expected
    worker.render("dummy.wav", events, timing, 1, 44100);

    // The CC reached the device and drove its cutoff
    QVERIFY(parameterSpy.count() > 0);

    // ...and the render handed the patch back on the way out. Automation writes only the live layer,
    // so exporting a song can never leave the automated value behind as the one that gets saved.
    QCOMPARE(synth1->lpfCutoff(), 0.5f);
}

void RenderingTest::test_render_pitchBend_shouldProcessEvent()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
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

void RenderingTest::test_render_shouldTrimAudio()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto mixerService = std::make_shared<MixerService>();

    const auto synth = std::make_shared<SynthDevice>("Noteahead Synth");
    synth->setLpfCutoff(1.0f);
    synth->setGain(0.5f);
    synth->setVolume(1.0f);
    deviceService->setDevice(0, synth);

    RenderWorker worker { audioEngine, deviceService, mixerService };

    MockRenderIo * mockIoPtr = nullptr;
    worker.setAudioFileReaderFactory([&]() {
        auto io = std::make_unique<MockRenderIo>();
        mockIoPtr = io.get();
        return io;
    });

    RenderWorker::EventList events;
    const auto instrument = std::make_shared<Instrument>("Noteahead Internal Device 1");
    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(60, 100);
    const auto event = std::make_shared<Event>(0, noteData);
    event->setInstrument(instrument);
    events.push_back(event);

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    // Render with trim: trim to 2 seconds
    RenderOptions options;
    options.trim = true;
    options.trimSeconds = 2;
    worker.render("dummy.wav", events, timing, 1000, 44100, options);

    QVERIFY(mockIoPtr != nullptr);
    const auto & audioData = mockIoPtr->data();
    QVERIFY(!audioData.empty());

    // 2 seconds of stereo audio at 44100Hz should have exactly 2 * 44100 * 2 = 176400 samples
    const size_t expectedSamples = 2 * 44100 * 2;
    QCOMPARE(audioData.size(), expectedSamples);
}

namespace {

//! What the fade and silence tests all need: one held note through a synth, differing only in the
//! options. Returns the stereo samples that reached the file.
std::vector<float> renderHeldNote(const RenderOptions & options, quint64 maxTick, quint32 sampleRate)
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto mixerService = std::make_shared<MixerService>();

    const auto synth = std::make_shared<SynthDevice>("Noteahead Synth");
    synth->setLpfCutoff(1.0f);
    synth->setGain(0.5f);
    synth->setVolume(1.0f);
    deviceService->setDevice(0, synth);

    RenderWorker worker { audioEngine, deviceService, mixerService };

    // The registry outlives the reader the recorder owns, unlike a bare pointer to it.
    MockRenderIo::Registry registry;
    std::mutex registryMutex;
    worker.setAudioFileReaderFactory([&]() {
        return std::make_unique<MockRenderIo>(&registry, &registryMutex);
    });

    RenderWorker::EventList events;
    const auto instrument = std::make_shared<Instrument>("Noteahead Internal Device 1");
    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(60, 100);
    const auto event = std::make_shared<Event>(0, noteData);
    event->setInstrument(instrument);
    events.push_back(event);

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    worker.render("dummy.wav", events, timing, maxTick, sampleRate, options);

    return registry["dummy.wav"].data;
}

//! Largest absolute sample over a frame range, both channels.
float peakOverFrames(const std::vector<float> & data, size_t firstFrame, size_t lastFrame)
{
    float peak = 0.0f;
    for (size_t i = firstFrame * 2; i < lastFrame * 2 && i < data.size(); i++) {
        peak = std::max(peak, std::abs(data[i]));
    }
    return peak;
}

} // namespace

void RenderingTest::test_render_silence_shouldFitInsideTrim()
{
    // Trimming fixes the length of the file, so the tail silence is carved out of its end rather
    // than added to it.
    const quint32 sampleRate = 44100;
    RenderOptions options;
    options.trim = true;
    options.trimSeconds = 2;
    options.silence = true;
    options.silenceTenths = 5; // 0.5 s

    const auto audioData = renderHeldNote(options, 1000, sampleRate);

    QCOMPARE(audioData.size(), size_t { 2 * 44100 * 2 });

    const size_t silenceFrames = sampleRate / 2;
    const size_t totalFrames = audioData.size() / 2;
    QCOMPARE(peakOverFrames(audioData, totalFrames - silenceFrames, totalFrames), 0.0f);
    QVERIFY(peakOverFrames(audioData, 0, totalFrames - silenceFrames) > 0.0f);
}

void RenderingTest::test_render_silence_withoutTrim_shouldExtendFile()
{
    // Without a trim there is nothing to fit inside, so the silence follows the song end.
    const quint32 sampleRate = 44100;
    const quint64 maxTick = 48;
    RenderOptions options;
    options.silence = true;
    options.silenceTenths = 5; // 0.5 s

    const auto audioData = renderHeldNote(options, maxTick, sampleRate);

    const double samplesPerTick = 60.0 * sampleRate / (120.0 * 4.0 * 6.0);
    const auto songFrames = static_cast<size_t>(std::llround(static_cast<double>(maxTick + 1) * samplesPerTick));
    const size_t silenceFrames = sampleRate / 2;
    QCOMPARE(audioData.size(), (songFrames + silenceFrames) * 2);

    const size_t totalFrames = audioData.size() / 2;
    QCOMPARE(peakOverFrames(audioData, songFrames, totalFrames), 0.0f);
    QVERIFY(peakOverFrames(audioData, 0, songFrames) > 0.0f);
}

void RenderingTest::test_render_fadeOut_shouldRampDownToZero()
{
    // Compared against the very same render without the fade, so that a synth decaying on its own
    // cannot make this pass.
    const quint32 sampleRate = 44100;
    RenderOptions options;
    options.trim = true;
    options.trimSeconds = 2;

    const auto withoutFade = renderHeldNote(options, 1000, sampleRate);

    options.fadeOut = true;
    options.fadeOutSeconds = 1;
    const auto withFade = renderHeldNote(options, 1000, sampleRate);

    QCOMPARE(withFade.size(), withoutFade.size());

    const size_t totalFrames = withFade.size() / 2;
    const size_t fadeStart = totalFrames - sampleRate; // The fade covers the last second
    const size_t tailStart = totalFrames - sampleRate / 10;

    // Untouched before the fade begins
    QCOMPARE(peakOverFrames(withFade, 0, fadeStart), peakOverFrames(withoutFade, 0, fadeStart));

    // The last tenth of the fade is where the cosine has all but landed on zero
    const auto fadedTailPeak = peakOverFrames(withFade, tailStart, totalFrames);
    const auto originalTailPeak = peakOverFrames(withoutFade, tailStart, totalFrames);
    QVERIFY(originalTailPeak > 0.0f);
    QVERIFY(fadedTailPeak < originalTailPeak * 0.1f);

    // And the very last frame is silent
    QVERIFY(std::abs(withFade[withFade.size() - 1]) < 1e-6f);
}

void RenderingTest::test_render_shouldNormalizeAudio()
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto mixerService = std::make_shared<MixerService>();

    const auto synth = std::make_shared<SynthDevice>("Noteahead Synth");
    synth->setLpfCutoff(1.0f);
    synth->setGain(0.2f); // Make it quiet (peak well below 1.0)
    synth->setVolume(1.0f);
    deviceService->setDevice(0, synth);

    RenderWorker worker { audioEngine, deviceService, mixerService };

    MockRenderIo::Registry registry;
    std::mutex registryMutex;
    worker.setAudioFileReaderFactory([&]() {
        return std::make_unique<MockRenderIo>(&registry, &registryMutex);
    });

    RenderWorker::EventList events;
    const auto instrument = std::make_shared<Instrument>("Noteahead Internal Device 1");
    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(60, 100);
    const auto event = std::make_shared<Event>(0, noteData);
    event->setInstrument(instrument);
    events.push_back(event);

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    // Render with normalization: normalize to -6.0 dB (approx 0.5 amplitude)
    const double targetDb = -6.0;
    RenderOptions options;
    options.normalize = true;
    options.normalizeTargetDb = targetDb;
    worker.render("dummy.wav", events, timing, 48, 44100, options);

    // The normalized audio will be written to "dummy.wav" (the final path)
    QVERIFY(registry.find("dummy.wav") != registry.end());
    const auto & audioData = registry["dummy.wav"].data;
    QVERIFY(!audioData.empty());

    float maxAmp = 0.0f;
    for (float s : audioData) {
        maxAmp = std::max(maxAmp, std::abs(s));
    }

    // Peak amplitude should be close to 0.5 (linear for -6 dB is 10^(-6/20) ≈ 0.501)
    const float expectedPeak = std::pow(10.0f, targetDb / 20.0f);
    QVERIFY(maxAmp > expectedPeak - 0.05f);
    QVERIFY(maxAmp < expectedPeak + 0.05f);
}

namespace {

//! Renders one note to the given path, with the loudness analysis on or off.
void renderWithAnalysis(const QString & path, bool analyze)
{
    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    const auto mixerService = std::make_shared<MixerService>();

    const auto synth = std::make_shared<SynthDevice>("Noteahead Synth");
    synth->setLpfCutoff(1.0f);
    synth->setGain(0.5f);
    synth->setVolume(1.0f);
    deviceService->setDevice(0, synth);

    RenderWorker worker { audioEngine, deviceService, mixerService };

    MockRenderIo::Registry registry;
    std::mutex registryMutex;
    worker.setAudioFileReaderFactory([&]() {
        return std::make_unique<MockRenderIo>(&registry, &registryMutex);
    });

    RenderWorker::EventList events;
    const auto instrument = std::make_shared<Instrument>("Noteahead Internal Device 1");
    NoteData noteData { 0, 0 };
    noteData.setAsNoteOn(60, 100);
    const auto event = std::make_shared<Event>(0, noteData);
    event->setInstrument(instrument);
    events.push_back(event);

    RenderWorker::Timing timing;
    timing.beatsPerMinute = 120;
    timing.linesPerBeat = 4;
    timing.ticksPerLine = 6;

    RenderOptions options;
    options.analyze = analyze;
    worker.render(path, events, timing, 48, 44100, options);
}

} // namespace

void RenderingTest::test_render_analysis_shouldWriteReportBesideTheRenderedFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("song.flac");

    renderWithAnalysis(path, true);

    // The whole rendered name plus the suffix, so a WAV and a FLAC of the same song keep their own
    const QFile report { path + ".loudness.txt" };
    QVERIFY(QFileInfo::exists(report.fileName()));

    QFile openedReport { report.fileName() };
    QVERIFY(openedReport.open(QIODevice::ReadOnly | QIODevice::Text));
    const auto text = QString::fromUtf8(openedReport.readAll());

    QVERIFY(text.contains("song.flac"));
    QVERIFY(text.contains("Integrated loudness:"));
    QVERIFY(text.contains("True peak:"));
    QVERIFY(text.contains("Loudness range (LRA):"));
    QVERIFY(text.contains("Threshold:"));
    QVERIFY(text.contains("LUFS"));
    QVERIFY(text.contains("dBTP"));
}

void RenderingTest::test_render_analysisDisabled_shouldWriteNoReport()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("song.flac");

    // Without an analysis there is nothing to report, so nothing may be left beside the audio
    renderWithAnalysis(path, false);

    QVERIFY(!QFileInfo::exists(path + ".loudness.txt"));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RenderingTest)
