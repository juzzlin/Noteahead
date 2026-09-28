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
#include "../../domain/tracker/instrument.hpp"
#include "../../domain/tracker/note_data.hpp"
#include "../../domain/tracker/song.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <cmath>
#include <optional>
#include <vector>

namespace noteahead {

//! A device that records the note length the render hands it, and nothing else.
//!
//! What it is for: a rendered song has to reach a device with the same note length a played one
//! does. Speech's Line mode fits a spoken line inside the note that speaks it, so a render that
//! dropped the length spoke to a different clock than playback did -- silently, because a device
//! with no length simply falls back to its own setting.
class NoteBeatsProbeDevice : public Device
{
public:
    std::string name() const override { return "NoteBeatsProbe"; }
    std::string category() const override { return "Test"; }
    std::string typeName() const override { return "NoteBeatsProbe"; }
    std::string typeId() const override { return "00000000-0000-4000-8000-00000000beef"; }

    void processMidiNoteOn(uint8_t, uint8_t) override
    {
        m_received.push_back(noteBeats());
    }

    void processMidiNoteOff(uint8_t) override { }
    void processMidiAllNotesOff() override { }
    void processDeviceMidiCc(uint8_t, uint8_t, uint8_t) override { }
    void processAudio(AudioContext &) override { }

    const std::vector<std::optional<double>> & received() const { return m_received; }

private:
    std::vector<std::optional<double>> m_received;
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
    QVERIFY(spy.wait(5000));

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

    QVERIFY(spy.wait(5000));

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
    QVERIFY(spy.wait(5000));
    QCOMPARE(progressWhenBarAppears, 0.0);

    // The first render left progress where it finished. Starting another one has to clear that, or
    // the bar shows the previous export as complete until the first progress callback lands.
    QVERIFY2(renderService.progress() > 0.0, qPrintable(QString::number(renderService.progress())));

    progressWhenBarAppears = -1.0;
    renderService.renderMaster(QDir { tempDir.path() }.filePath("second.flac"));
    QCOMPARE(progressWhenBarAppears, 0.0);

    QVERIFY(spy.wait(5000));
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
    QVERIFY(spy.wait(30000));
    QVERIFY2(spy.at(0).at(0).toBool(), qPrintable(spy.at(0).at(1).toString()));

    QVERIFY2(!probe->received().empty(), "the render never reached the device");
    const auto beats = probe->received().front();
    QVERIFY2(beats.has_value(), "the render gave the device no note length at all");
    QVERIFY2(std::abs(*beats - 2.0) < 1.0e-6, qPrintable(QString::number(*beats)));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RenderServiceTest)
