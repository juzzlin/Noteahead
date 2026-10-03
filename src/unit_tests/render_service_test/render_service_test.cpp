#include "render_service_test.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/render_service.hpp"
#include "../../application/service/render_worker.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/dsp/audio_context.hpp"
#include "../../domain/effects/effect.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../domain/tracker/note_data.hpp"
#include "../../domain/tracker/parameter_automation.hpp"
#include "../../domain/tracker/song.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace noteahead {

//! Blocks until the spy has seen `count` renders finish, however long they take.
//!
//! The worker hands its result back through the event loop, so waiting on the loop rather than on
//! the clock follows the render itself: a CI machine still linking while the tests run has stalled a
//! one-bar render for five seconds, and any timeout short enough to be useful can lose that race. A
//! render that never finishes is left to ctest's own timeout.
static void waitForRenders(const QSignalSpy & spy, qsizetype count)
{
    while (spy.count() < count) {
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents);
    }
}

//! A device that records the note length the render hands it, and nothing else.
//!
//! What it is for: a rendered song has to reach a device with the same note length a played one
//! does. Speech's Line mode fits a spoken line inside the note that speaks it, so a render that
//! dropped the length spoke to a different clock than playback did -- silently, because a device
//! with no length simply falls back to its own setting.
class NoteBeatsProbeDevice : public Device
{
public:
    std::string name() const override
    {
        return "NoteBeatsProbe";
    }

    std::string category() const override
    {
        return "Test";
    }

    std::string typeName() const override
    {
        return "NoteBeatsProbe";
    }

    std::string typeId() const override
    {
        return "00000000-0000-4000-8000-00000000beef";
    }

    void processMidiNoteOn(uint8_t, uint8_t) override
    {
        m_received.push_back(noteBeats());
    }

    void processMidiNoteOff(uint8_t) override
    {
    }

    void processMidiAllNotesOff() override
    {
    }

    void processDeviceMidiCc(uint8_t, uint8_t, uint8_t) override
    {
    }

    void processAudio(AudioContext &) override
    {
    }

    const std::vector<std::optional<double>> & received() const
    {
        return m_received;
    }

private:
    std::vector<std::optional<double>> m_received;
};

//! An effect that remembers the value its one parameter had in every block it was asked to render.
class ParameterProbeEffect : public Effect
{
public:
    ParameterProbeEffect()
    {
        addParameter(Parameter { "probe", 0.5f, 0, 10000, 5000, 100 });
    }

    std::string type() const override
    {
        return "ParameterProbe";
    }

    std::string typeId() const override
    {
        return "00000000-0000-4000-8000-0000000fade0";
    }

    const std::vector<float> & seen() const
    {
        return m_seen;
    }

protected:
    void processSample(double &, double &) override
    {
    }

    void processBlock(AudioContext &) override
    {
        m_seen.push_back(parameter("probe")->get().value());
    }

private:
    std::vector<float> m_seen;
};

//! A device that remembers the value its one described parameter had in every block it rendered.
class ParameterProbeDevice : public Device
{
public:
    ParameterProbeDevice()
    {
        addParameter(Parameter { "probe", 0.5f, 0, 10000, 5000, 100 });
        // Described, or a device would not offer it to automation at all
        setParameterDisplay("probe", { .unit = "%", .max = 100.0, .label = "Probe" });
    }

    std::string name() const override
    {
        return "ParameterProbe";
    }

    std::string category() const override
    {
        return "Test";
    }

    std::string typeName() const override
    {
        return "ParameterProbe";
    }

    std::string typeId() const override
    {
        return "00000000-0000-4000-8000-00000000d0e5";
    }

    void processMidiNoteOn(uint8_t, uint8_t) override
    {
    }

    void processMidiNoteOff(uint8_t) override
    {
    }

    void processMidiAllNotesOff() override
    {
    }

    void processDeviceMidiCc(uint8_t, uint8_t, uint8_t) override
    {
    }

    void processAudio(AudioContext &) override
    {
        m_seen.push_back(parameter("probe")->get().value());
    }

    //! Always, so that the engine never skips the silent probe and every block is seen
    bool hasActiveAudio() const override
    {
        return true;
    }

    const std::vector<float> & seen() const
    {
        return m_seen;
    }

private:
    std::vector<float> m_seen;
};

class MockEditorService : public EditorService
{
public:
    MockEditorService()
      : EditorService { nullptr, nullptr, nullptr, nullptr }
    {
    }

    TrackIndexList trackIndices() const override
    {
        return m_trackIndices;
    }

    QString instrumentPortName(quint64 trackIndex) const override
    {
        return m_instrumentPorts.at(trackIndex);
    }

    QString trackName(quint64 trackIndex) const override
    {
        return QString("Track%1").arg(trackIndex);
    }

    quint64 beatsPerMinute() const override
    {
        return 120;
    }

    quint64 linesPerBeat() const override
    {
        return 4;
    }

    quint64 ticksPerLine() const override
    {
        // What Song itself uses. The render turns ticks into both seconds and beats with the
        // numbers the editor hands it, so a mock that disagrees with the song it is rendering
        // measures itself rather than the render.
        return 24;
    }

    void setTrackIndices(TrackIndexList indices)
    {
        m_trackIndices = indices;
    }

    void setInstrumentPort(quint64 trackIndex, QString port)
    {
        m_instrumentPorts[trackIndex] = port;
    }

private:
    TrackIndexList m_trackIndices;
    std::map<quint64, QString> m_instrumentPorts;
};

void RenderServiceTest::test_renderIndividualTracks_shouldSkipNonInternalInstruments()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    auto mixerService = std::make_shared<MixerService>();
    auto editorService = std::make_shared<MockEditorService>();
    auto propertyService = std::make_shared<PropertyService>();
    auto automationService = std::make_shared<AutomationService>(propertyService);
    auto sideChainService = std::make_shared<SideChainService>();

    auto song = std::make_shared<Song>();
    editorService->setSong(song);

    // Track 0: Internal
    // Track 1: External
    // Track 2: Internal
    editorService->setTrackIndices({ 0, 1, 2 });
    editorService->setInstrumentPort(0, Constants::internalDevicePortPrefix() + " 1");
    editorService->setInstrumentPort(1, "External MIDI Port");
    editorService->setInstrumentPort(2, Constants::internalDevicePortPrefix() + " 2");

    RenderService renderService(audioEngine, deviceService, mixerService, editorService, automationService, sideChainService);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QSignalSpy spy(&renderService, &RenderService::renderingFinished);

    renderService.renderIndividualTracks(tempDir.path());

    // Wait for it to finish.
    waitForRenders(spy, 1);

    // Check that it finished successfully
    QCOMPARE(spy.at(0).at(0).toBool(), true);

    // If it skipped Track 1, it should have rendered only 2 files.
    QDir dir(tempDir.path());
    QStringList files = dir.entryList(QDir::Files);
    files.sort();

    // QDir::entryList might contain "." and ".."
    files.removeAll(".");
    files.removeAll("..");

    // Each rendered file is accompanied by its loudness report
    QStringList reports = files.filter(".loudness.txt");
    QCOMPARE(reports.size(), 2);
    for (const auto & report : reports) {
        files.removeAll(report);
    }

    QCOMPARE(files.size(), 2);

    auto containsGlob = [](const QStringList & list, const QString & pattern) {
        QRegularExpression re(QRegularExpression::wildcardToRegularExpression(pattern));
        for (const auto & str : list) {
            if (re.match(str).hasMatch()) {
                return true;
            }
        }
        return false;
    };

    QVERIFY(containsGlob(files, "Track0_*.flac"));
    QVERIFY(containsGlob(files, "Track2_*.flac"));
    QVERIFY(!containsGlob(files, "Track1_*.flac"));

    // The settings a render was made with are carried in its name: sample rate, word length and
    // the oversampling, which changes the master audibly and would otherwise leave two renders of
    // the same song differing only in their timestamp.
    QVERIFY2(containsGlob(files, "Track0_*k_*bit_*x_*.flac"), qPrintable(files.join(", ")));
}

void RenderServiceTest::test_renderIndividualTracks_shouldRestoreMixerState()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    auto mixerService = std::make_shared<MixerService>();
    auto editorService = std::make_shared<MockEditorService>();
    auto propertyService = std::make_shared<PropertyService>();
    auto automationService = std::make_shared<AutomationService>(propertyService);
    auto sideChainService = std::make_shared<SideChainService>();

    auto song = std::make_shared<Song>();
    editorService->setSong(song);

    editorService->setTrackIndices({ 0, 1 });
    editorService->setInstrumentPort(0, Constants::internalDevicePortPrefix() + " 1");
    editorService->setInstrumentPort(1, Constants::internalDevicePortPrefix() + " 2");

    // Set some initial mixer state
    mixerService->soloTrack(0, true);
    mixerService->muteTrack(1, true);

    RenderService renderService(audioEngine, deviceService, mixerService, editorService, automationService, sideChainService);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QSignalSpy spy(&renderService, &RenderService::renderingFinished);

    renderService.renderIndividualTracks(tempDir.path());

    waitForRenders(spy, 1);

    // Mixer state should be restored
    QCOMPARE(mixerService->isTrackSoloed(0), true);
    QCOMPARE(mixerService->isTrackSoloed(1), false);
    QCOMPARE(mixerService->isTrackMuted(1), true);
    QCOMPARE(mixerService->isTrackMuted(0), false);
}

void RenderServiceTest::test_renderMaster_secondRender_shouldStartFromZeroProgress()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    auto mixerService = std::make_shared<MixerService>();
    auto editorService = std::make_shared<MockEditorService>();
    auto propertyService = std::make_shared<PropertyService>();
    auto automationService = std::make_shared<AutomationService>(propertyService);
    auto sideChainService = std::make_shared<SideChainService>();

    editorService->setSong(std::make_shared<Song>());

    RenderService renderService { audioEngine, deviceService, mixerService, editorService, automationService, sideChainService };

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // The progress bar appears when isRendering turns true, so what it shows first is whatever
    // progress reads at that moment.
    double progressWhenBarAppears = -1.0;
    QObject::connect(&renderService, &RenderService::isRenderingChanged, &renderService, [&] {
        if (renderService.isRendering()) {
            progressWhenBarAppears = renderService.progress();
        }
    });

    QSignalSpy spy { &renderService, &RenderService::renderingFinished };

    renderService.renderMaster(QDir { tempDir.path() }.filePath("first.flac"));
    waitForRenders(spy, 1);
    QCOMPARE(progressWhenBarAppears, 0.0);

    // The first render left progress where it finished. Starting another one has to clear that, or
    // the bar shows the previous export as complete until the first progress callback lands.
    QVERIFY2(renderService.progress() > 0.0, qPrintable(QString::number(renderService.progress())));

    progressWhenBarAppears = -1.0;
    renderService.renderMaster(QDir { tempDir.path() }.filePath("second.flac"));
    QCOMPARE(progressWhenBarAppears, 0.0);

    waitForRenders(spy, 2);
}

void RenderServiceTest::test_renderMaster_shouldGiveTheDeviceTheNoteLength()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    auto mixerService = std::make_shared<MixerService>();
    auto editorService = std::make_shared<MockEditorService>();
    auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    auto sideChainService = std::make_shared<SideChainService>();

    auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);
    song->setInstrument(0, std::make_shared<Instrument>(Constants::internalDevicePortPrefix() + " 1"));

    // Eight lines at four to the beat is two beats, which is what the device must be told.
    NoteData noteOn;
    noteOn.setAsNoteOn(60, 100);
    song->setNoteDataAtPosition(noteOn, { 0, 0, 0, 0, 0 });
    NoteData noteOff;
    noteOff.setAsNoteOff(60);
    song->setNoteDataAtPosition(noteOff, { 0, 0, 0, 8, 0 });

    editorService->setSong(song);
    editorService->setTrackIndices({ 0 });
    editorService->setInstrumentPort(0, Constants::internalDevicePortPrefix() + " 1");

    // The port name numbers the slot from one and the rack numbers it from zero, so the device
    // behind "Internal Device 1" sits in slot 0.
    const auto probe = std::make_shared<NoteBeatsProbeDevice>();
    deviceService->setDevice(0, probe);

    RenderService renderService { audioEngine, deviceService, mixerService, editorService, automationService, sideChainService };

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QSignalSpy spy { &renderService, &RenderService::renderingFinished };
    renderService.renderMaster(tempDir.filePath("render.flac"));
    waitForRenders(spy, 1);
    QVERIFY2(spy.at(0).at(0).toBool(), qPrintable(spy.at(0).at(1).toString()));

    QVERIFY2(!probe->received().empty(), "the render never reached the device");
    const auto beats = probe->received().front();
    QVERIFY2(beats.has_value(), "the render gave the device no note length at all");
    QVERIFY2(std::abs(*beats - 2.0) < 1.0e-6, qPrintable(QString::number(*beats)));
}

void RenderServiceTest::test_renderMaster_parameterAutomation_shouldSweepTheEffect()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    auto mixerService = std::make_shared<MixerService>();
    auto editorService = std::make_shared<MockEditorService>();
    auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    auto sideChainService = std::make_shared<SideChainService>();
    automationService->setTargetResolver([deviceService](const ParameterAutomation::Target & target) -> std::shared_ptr<ParameterAutomationTarget> {
        if (target.owner == ParameterAutomation::Target::Owner::Device) {
            return deviceService->findDevice(target.ownerId);
        }
        return deviceService->findEffect(target.ownerId);
    });

    auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);
    song->setInstrument(0, std::make_shared<Instrument>(Constants::internalDevicePortPrefix() + " 1"));
    editorService->setSong(song);
    editorService->setTrackIndices({ 0 });
    editorService->setInstrumentPort(0, Constants::internalDevicePortPrefix() + " 1");

    // On the master inserts, so that the automation is seen to need no device of its own.
    const auto probe = std::make_shared<ParameterProbeEffect>();
    deviceService->insertEffectRack().setEffect(0, probe);

    // Lines 0 to 16, all the way up.
    automationService->addParameterAutomation({ 0, AutomationLocation { 0, 0, 0 }, { ParameterAutomation::Target::Owner::Effect, probe->id(), "probe" }, ParameterAutomation::Mode::Absolute, { 0, 16, 0.0, 1.0 }, {}, {}, true });

    RenderService renderService { audioEngine, deviceService, mixerService, editorService, automationService, sideChainService };

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QSignalSpy spy { &renderService, &RenderService::renderingFinished };
    renderService.renderMaster(tempDir.filePath("render.flac"));
    waitForRenders(spy, 1);
    QVERIFY2(spy.at(0).at(0).toBool(), qPrintable(spy.at(0).at(1).toString()));

    const auto & seen = probe->seen();
    QVERIFY(!seen.empty());
    QCOMPARE(seen.front(), 0.0f);
    QVERIFY(std::ranges::find(seen, 1.0f) != seen.end());
    // Rising the whole way, and through far more values than the sixteen lines it spans: the sweep
    // is rendered tick by tick rather than stepped once a line.
    QVERIFY(std::ranges::is_sorted(seen.begin(), std::ranges::find(seen, 1.0f)));
    std::vector<float> distinct = seen;
    std::ranges::sort(distinct);
    distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());
    QVERIFY2(distinct.size() > 100, qPrintable(QString::number(distinct.size())));

    // And the render leaves the knob as the user set it
    QCOMPARE(probe->parameter("probe")->get().value(), 0.5f);
}

void RenderServiceTest::test_renderMaster_parameterAutomation_shouldSweepTheDevice()
{
    auto audioEngine = std::make_shared<AudioEngine>();
    auto deviceService = std::make_shared<DeviceService>(audioEngine, std::make_shared<DataService>());
    auto mixerService = std::make_shared<MixerService>();
    auto editorService = std::make_shared<MockEditorService>();
    auto automationService = std::make_shared<AutomationService>(std::make_shared<PropertyService>());
    auto sideChainService = std::make_shared<SideChainService>();
    automationService->setTargetResolver([deviceService](const ParameterAutomation::Target & target) -> std::shared_ptr<ParameterAutomationTarget> {
        if (target.owner == ParameterAutomation::Target::Owner::Device) {
            return deviceService->findDevice(target.ownerId);
        }
        return deviceService->findEffect(target.ownerId);
    });

    auto song = std::make_shared<Song>();
    song->setBeatsPerMinute(120);
    song->setLinesPerBeat(4);
    song->setInstrument(0, std::make_shared<Instrument>(Constants::internalDevicePortPrefix() + " 1"));
    editorService->setSong(song);
    editorService->setTrackIndices({ 0 });
    editorService->setInstrumentPort(0, Constants::internalDevicePortPrefix() + " 1");

    const auto probe = std::make_shared<ParameterProbeDevice>();
    deviceService->setDevice(0, probe);
    probe->parameter("probe")->get().setValue(0.8f);

    // Relative, from nothing up to the knob over lines 0 to 16
    automationService->addParameterAutomation({ 0, AutomationLocation { 0, 0, 0 }, { ParameterAutomation::Target::Owner::Device, probe->uuid(), "probe" }, ParameterAutomation::Mode::Relative, { 0, 16, 0.0, 1.0 }, {}, {}, true });

    RenderService renderService { audioEngine, deviceService, mixerService, editorService, automationService, sideChainService };

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QSignalSpy spy { &renderService, &RenderService::renderingFinished };
    renderService.renderMaster(tempDir.filePath("render.flac"));
    waitForRenders(spy, 1);
    QVERIFY2(spy.at(0).at(0).toBool(), qPrintable(spy.at(0).at(1).toString()));

    const auto & seen = probe->seen();
    QVERIFY(!seen.empty());
    QCOMPARE(seen.front(), 0.0f);
    // Up to the knob and no further: relative never passes the setting it scales
    const auto top = std::ranges::max(seen);
    QVERIFY(std::abs(top - 0.8f) < 1.0e-6f);
    QVERIFY(std::ranges::is_sorted(seen.begin(), std::ranges::find_if(seen, [](float value) { return std::abs(value - 0.8f) < 1.0e-6f; })));
    std::vector<float> distinct = seen;
    std::ranges::sort(distinct);
    distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());
    QVERIFY2(distinct.size() > 100, qPrintable(QString::number(distinct.size())));

    // And the render leaves the knob as the user set it
    QCOMPARE(probe->parameter("probe")->get().value(), 0.8f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RenderServiceTest)
