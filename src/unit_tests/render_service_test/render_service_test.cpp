#include "render_service_test.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/render_service.hpp"
#include "../../application/service/render_worker.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/tracker/song.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/data_service.hpp"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace noteahead {

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
        return 6;
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

    QVERIFY(containsGlob(files, "Track0_*.wav"));
    QVERIFY(containsGlob(files, "Track2_*.wav"));
    QVERIFY(!containsGlob(files, "Track1_*.wav"));
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

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::RenderServiceTest)
