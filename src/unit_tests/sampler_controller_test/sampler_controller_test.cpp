// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
#include "sampler_controller_test.hpp"

#include "../../application/service/settings_service.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../domain/utility/stereo_level_meter.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"
#include "../../view/controllers/sampler_controller.hpp"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>
#include <cmath>
#include <span>
#include <vector>

namespace noteahead {

//! Serves a constant one-second buffer so that pads can be loaded without touching the file system.
class MockAudioFileReader : public AudioFileReader
{
public:
    bool open(const std::string &, Mode, Info & info) override
    {
        info = this->info();
        return true;
    }

    void close() override
    {
    }

    void setTag(TagType, const std::string &) override
    {
    }

    int64_t readFloat(std::span<float> data) override
    {
        std::fill(data.begin(), data.end(), 1.0f);
        return data.size();
    }

    int64_t readDouble(std::span<double> data) override
    {
        return data.size();
    }

    int64_t readInt(std::span<int32_t> data) override
    {
        return data.size();
    }

    int64_t writeFloat(std::span<const float> data) override
    {
        return data.size();
    }

    int64_t writeInt(std::span<const int32_t> data) override
    {
        return data.size();
    }

    bool seek(int64_t, int) override
    {
        return true;
    }

    bool isOpen() const override
    {
        return true;
    }

    Info info() const override
    {
        return { m_frames, static_cast<int>(Constants::defaultSampleRate()), 2, 0 };
    }

    //! Lets a test hand out a longer file, which the offsets need in order not to clamp away.
    void setFrames(int64_t frames)
    {
        m_frames = frames;
    }

private:
    int64_t m_frames = 1024;
};

namespace {

//! Settings of this test's own, so that the metronome switches it writes go to a directory that goes
//! with the process. Without it the test both read back what its previous run had persisted -- which
//! made the defaults it asserts depend on run order -- and wrote into the settings of whatever
//! application name the test binary happens to carry.
QTemporaryDir & settingsDirectory()
{
    static QTemporaryDir directory;
    return directory;
}

} // namespace

void SamplerControllerTest::initTestCase()
{
    QCoreApplication::setOrganizationName("NoteaheadTest");
    QCoreApplication::setApplicationName("SamplerControllerTest");
    QVERIFY(settingsDirectory().isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory().path());
}

void SamplerControllerTest::test_sampleRateChange_shouldUpdateHzValues()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler");
    SamplerController controller { sampler };

    // Initially sample rate should be default
    QCOMPARE(controller.sampleRate(), static_cast<uint32_t>(Constants::defaultSampleRate()));

    controller.setSelectedPad(0);
    controller.setSelectedPadCutoff(0.5);
    const auto initialHz = controller.cutoffToHz(controller.selectedPadCutoff());
    QVERIFY(initialHz > 0.0f);

    QSignalSpy cutoffSpy { &controller, &SamplerController::selectedPadCutoffChanged };
    QSignalSpy srSpy { &controller, &SamplerController::sampleRateChanged };

    // Change sample rate to something lower so maxFreq is affected (min(20000, sr*0.49))
    sampler->setSampleRate(32000);

    QCOMPARE(srSpy.count(), 1);
    QCOMPARE(cutoffSpy.count(), 1);

    const auto newHz = controller.cutoffToHz(controller.selectedPadCutoff());
    const auto expectedHz = initialHz * ((32000.0f * 0.49f) / 20000.0f);
    QVERIFY2(std::abs(newHz - expectedHz) < 1.0f,
             QString("newHz: %1, initialHz: %2, expectedHz: %3").arg(newHz).arg(initialHz).arg(expectedHz).toUtf8().constData());
}

void SamplerControllerTest::test_properties_shouldUpdateDeviceAndEmitSignals()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler");
    SamplerController controller { sampler };

    // Common properties (now scaled ints)
    {
        QSignalSpy spy { &controller, &SamplerController::volumeChanged };
        controller.setVolume(800);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.volume(), 800);
        QCOMPARE(sampler->volume(), 0.8f);
    }

    // Controller specific
    {
        QSignalSpy spy { &controller, &SamplerController::selectedPadChanged };
        controller.setSelectedPad(2);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(controller.selectedPad(), 2);
    }
}

void SamplerControllerTest::test_selectedPadLoopStart_secondsAndMilliseconds_shouldCombineIntoOneOffset()
{
    // The two spin boxes edit halves of one offset, so writing either has to keep the other half.
    auto reader = std::make_unique<MockAudioFileReader>();
    reader->setFrames(static_cast<int64_t>(Constants::defaultSampleRate()) * 4);
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::move(reader));
    SamplerController controller { sampler };
    controller.setSelectedPad(0);
    controller.loadSample(0, "test.wav");

    QSignalSpy spy { &controller, &SamplerController::selectedPadLoopStartChanged };
    controller.setSelectedPadLoopStartSeconds(2);
    controller.setSelectedPadLoopStartMilliseconds(250);

    QVERIFY(spy.count() >= 2);
    QCOMPARE(controller.selectedPadLoopStartSeconds(), 2);
    QCOMPARE(controller.selectedPadLoopStartMilliseconds(), 250);
    QVERIFY(std::abs(sampler->sampleLoopStart(SamplerDevice::padStartNote) - 2.25) < 0.001);
}

void SamplerControllerTest::test_selectedPadStartOffset_wholeSecond_shouldReadBackWhole()
{
    // An offset is stored as a fraction of a minute in a float, which lands a hair either side of the
    // second it was set to. Seventeen lands under, and split by flooring it read as sixteen seconds
    // and a thousand milliseconds.
    auto reader = std::make_unique<MockAudioFileReader>();
    reader->setFrames(static_cast<int64_t>(Constants::defaultSampleRate()) * 20);
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::move(reader));
    SamplerController controller { sampler };
    controller.setSelectedPad(0);
    controller.loadSample(0, "test.wav");

    controller.setSelectedPadStartOffsetSeconds(17);

    QCOMPARE(controller.selectedPadStartOffsetSeconds(), 17);
    QCOMPARE(controller.selectedPadStartOffsetMilliseconds(), 0);
}

void SamplerControllerTest::test_selectedPadStartOffset_pastTheSampleEnd_shouldClampAndStayClearable()
{
    // Three quarters of a second of audio, asked for a whole second of offset: the device clamps to
    // what there is, which leaves the offset in the milliseconds field rather than in the seconds
    // one. Both parts have to report that, or the field the user is looking at says zero while the
    // pad still starts three quarters of a second in.
    auto reader = std::make_unique<MockAudioFileReader>();
    reader->setFrames(static_cast<int64_t>(Constants::defaultSampleRate() * 3 / 4));
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::move(reader));
    SamplerController controller { sampler };
    controller.setSelectedPad(0);
    controller.loadSample(0, "test.wav");

    QSignalSpy spy { &controller, &SamplerController::selectedPadStartOffsetChanged };
    controller.setSelectedPadStartOffsetSeconds(1);

    QVERIFY(spy.count() > 0);
    QCOMPARE(controller.selectedPadStartOffsetSeconds(), 0);
    QCOMPARE(controller.selectedPadStartOffsetMilliseconds(), 750);

    // And it has to be possible to get back out of that: the seconds field alone cannot do it, since
    // the clamped remainder lives in the other one.
    controller.setSelectedPadStartOffsetMilliseconds(0);

    QCOMPARE(controller.selectedPadStartOffsetSeconds(), 0);
    QCOMPARE(controller.selectedPadStartOffsetMilliseconds(), 0);
}

void SamplerControllerTest::test_selectedPadLoop_enabled_shouldDropTheLoopPointInTheMiddleOfTheRange()
{
    // Four seconds trimmed by a second at each end leaves a two second range, so the point lands one
    // second in from where the range begins.
    auto reader = std::make_unique<MockAudioFileReader>();
    reader->setFrames(static_cast<int64_t>(Constants::defaultSampleRate()) * 4);
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::move(reader));
    SamplerController controller { sampler };
    controller.setSelectedPad(0);
    controller.loadSample(0, "test.wav");
    controller.setSelectedPadStartOffsetSeconds(1);
    controller.setSelectedPadEndOffsetSeconds(1);

    controller.setSelectedPadLoop(true);

    QCOMPARE(controller.selectedPadLoopStartSeconds(), 1);
    QCOMPARE(controller.selectedPadLoopStartMilliseconds(), 0);
}

void SamplerControllerTest::test_selectedPadLoop_enabled_shouldKeepALoopPointThePadAlreadyHas()
{
    auto reader = std::make_unique<MockAudioFileReader>();
    reader->setFrames(static_cast<int64_t>(Constants::defaultSampleRate()) * 4);
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::move(reader));
    SamplerController controller { sampler };
    controller.setSelectedPad(0);
    controller.loadSample(0, "test.wav");
    controller.setSelectedPadLoopStartSeconds(3);

    controller.setSelectedPadLoop(true);

    QCOMPARE(controller.selectedPadLoopStartSeconds(), 3);
}

void SamplerControllerTest::test_reset_shouldRestoreDefaultValues()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler");
    SamplerController controller { sampler };

    controller.setVolume(100);
    QSignalSpy spy { &controller, &SamplerController::volumeChanged };
    controller.reset();
    QVERIFY(spy.count() >= 1);
    QCOMPARE(controller.volume(), static_cast<int>(std::round(Constants::faderUnityPosition() * Constants::uiInternalScaling())));
}

void SamplerControllerTest::test_setSampler_shouldRefreshGlobalSwitchesToReflectNewInstance()
{
    // First instance with chromatic mode enabled.
    const auto samplerA = std::make_shared<SamplerDevice>("Sampler A");
    SamplerController controller { samplerA };
    controller.setChromaticMode(true);
    QVERIFY(controller.chromaticMode());

    // Switching to a second instance (chromatic mode off) must notify the UI so the switch
    // reflects the new instance instead of retaining the previous one's state.
    const auto samplerB = std::make_shared<SamplerDevice>("Sampler B");
    QVERIFY(!samplerB->chromaticMode());

    QSignalSpy chromaticSpy { &controller, &SamplerController::chromaticModeChanged };
    QSignalSpy channelSpy { &controller, &SamplerController::channelModeChanged };
    QSignalSpy embedSpy { &controller, &SamplerController::embedWaveDataChanged };

    controller.setSampler(samplerB);

    QCOMPARE(chromaticSpy.count(), 1);
    QCOMPARE(channelSpy.count(), 1);
    QCOMPARE(embedSpy.count(), 1);
    QVERIFY(!controller.chromaticMode());
}

void SamplerControllerTest::test_loadedPads_shouldListOnlyLoadedPads()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };

    QVERIFY(controller.loadedPads().isEmpty());

    controller.loadSample(0, "/samples/kick.wav"); // Pad 0 is note 36 in drum mode
    controller.loadSample(2, "/samples/hat.wav");

    const auto pads = controller.loadedPads();
    QCOMPARE(pads.size(), 2);
    QCOMPARE(pads.at(0).toMap()["padIndex"].toInt(), 0);
    QCOMPARE(pads.at(0).toMap()["note"].toInt(), 36);
    QCOMPARE(pads.at(0).toMap()["fileName"].toString(), QString { "kick.wav" });
    QCOMPARE(pads.at(1).toMap()["padIndex"].toInt(), 2);
    QCOMPARE(pads.at(1).toMap()["fileName"].toString(), QString { "hat.wav" });
}

void SamplerControllerTest::test_copyPad_shouldCopyPadToTarget()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };

    controller.loadSample(0, "/samples/kick.wav");
    controller.setSelectedPad(0);
    controller.setSelectedPadCutoff(0.4);
    controller.setSelectedPad(5);
    QSignalSpy cutoffSpy { &controller, &SamplerController::selectedPadCutoffChanged };

    controller.copyPad(0, 5);

    QVERIFY(sampler->sample(41)); // Pad 5 is note 41 in drum mode
    // The cutoff is stored as a float, hence the tolerance
    QVERIFY(std::abs(controller.selectedPadCutoff() - 0.4) < 1e-6);
    // The copy landed on the selected pad, so the pad settings are re-read
    QCOMPARE(cutoffSpy.count(), 1);
}

void SamplerControllerTest::test_recordingSeconds_notRecording_shouldBeZero()
{
    // The clock is read every tenth of a second while the dialog is open, including before anything
    // has ever been recorded. An unstarted QElapsedTimer has nothing meaningful to report, so the
    // reading has to come from the guard rather than from the timer.
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };

    QVERIFY(!controller.recording());
    QCOMPARE(controller.recordingSeconds(), 0.0);
}

void SamplerControllerTest::test_metronomeSettings_shouldClampAndSayWhenTheyChange()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };

    // A metronome nobody asked for stays out of the way, and a bar is the pre-count most people want.
    QVERIFY(!controller.metronomeEnabled());
    QVERIFY(controller.clickDuringTake());
    QCOMPARE(controller.preCountBars(), 1);
    QCOMPARE(controller.metronomeBeatsPerBar(), 4);
    QCOMPARE(controller.countInBeatsRemaining(), 0);

    QSignalSpy enabledSpy { &controller, &SamplerController::metronomeEnabledChanged };
    controller.setMetronomeEnabled(true);
    QCOMPARE(enabledSpy.count(), 1);
    controller.setMetronomeEnabled(true); // no change, no signal
    QCOMPARE(enabledSpy.count(), 1);

    // Nothing absurd gets through to the click: zero bars is "no count", and there is a ceiling.
    QSignalSpy barsSpy { &controller, &SamplerController::preCountBarsChanged };
    controller.setPreCountBars(-1);
    QCOMPARE(controller.preCountBars(), 0);
    controller.setPreCountBars(999);
    QCOMPARE(controller.preCountBars(), 8);
    QCOMPARE(barsSpy.count(), 2);

    controller.setMetronomeBeatsPerBar(0);
    QCOMPARE(controller.metronomeBeatsPerBar(), 1);
    controller.setMetronomeBeatsPerBar(99);
    QCOMPARE(controller.metronomeBeatsPerBar(), 16);
}

void SamplerControllerTest::test_selectedPadMono_shouldBePerPadAndSayWhenItChanges()
{
    // The checkbox binds to this property, so selecting another pad has to say the value changed. It
    // did not, and the switch went on showing whichever pad was last touched -- which reads in the UI
    // as Mono being stuck on for every pad, even though each pad held its own value all along.
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };
    controller.loadSample(0, "/samples/one.wav");
    controller.loadSample(1, "/samples/two.wav");

    controller.setSelectedPad(0);
    controller.setSelectedPadMono(true);
    QVERIFY(controller.selectedPadMono());

    QSignalSpy spy { &controller, &SamplerController::selectedPadMonoChanged };
    controller.setSelectedPad(1);

    QCOMPARE(spy.count(), 1);
    QVERIFY(!controller.selectedPadMono()); // pad 1 was never switched to mono
    QVERIFY(!sampler->sampleMono(37));

    controller.setSelectedPad(0);
    QVERIFY(controller.selectedPadMono()); // and pad 0 kept it
    QVERIFY(sampler->sampleMono(36));
}

void SamplerControllerTest::test_padNote_chromaticMode_shouldDefaultToOneOctavePerPad()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };

    QCOMPARE(controller.padNote(0), 36); // drum mode: pad 0 is C-3
    sampler->setChromaticMode(true);
    QCOMPARE(controller.padNote(0), 0);
    QCOMPARE(controller.padNote(3), 36);
}

void SamplerControllerTest::test_setPadNote_shouldMoveThePadAndItsAudio()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };
    sampler->setChromaticMode(true);
    controller.loadSample(2, "/samples/bass_e.wav"); // pad 2 is C-2

    QVERIFY(controller.setPadNote(2, 28)); // E-2

    QCOMPARE(controller.padNote(2), 28);
    QVERIFY(sampler->sample(28));
    QVERIFY(!sampler->sample(24)); // the audio moved rather than being copied
}

void SamplerControllerTest::test_setPadNote_occupiedNote_shouldBeRefused()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };
    sampler->setChromaticMode(true);
    controller.loadSample(2, "/samples/one.wav");
    controller.loadSample(3, "/samples/two.wav");

    // Pad 3 sits on C-3 already. Moving pad 2 onto it would leave one of them unreachable.
    QVERIFY(!controller.setPadNote(2, 36));
    QCOMPARE(controller.padNote(2), 24);
    QVERIFY(sampler->sample(24));
    QVERIFY(sampler->sample(36));
}

void SamplerControllerTest::test_setPadNote_outOfRange_shouldBeRefused()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };
    sampler->setChromaticMode(true);
    controller.loadSample(2, "/samples/kick.wav");

    QVERIFY(!controller.setPadNote(2, 128));
    QVERIFY(!controller.setPadNote(2, -1));
    QCOMPARE(controller.padNote(2), 24);
}

void SamplerControllerTest::test_setPadNote_drumMode_shouldBeRefused()
{
    // A drum pad is its note by definition: the layout is what a kit is.
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };
    controller.loadSample(0, "/samples/kick.wav");

    QVERIFY(!controller.setPadNote(0, 40));
    QCOMPARE(controller.padNote(0), 36);
}

void SamplerControllerTest::test_noteName_shouldNameTheNote()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };

    // The names the menu is built from, so they have to be the tracker's own.
    QCOMPARE(controller.noteName(36), QString { "C-3" });
    QCOMPARE(controller.noteName(28), QString { "E-2" });
    QCOMPARE(controller.noteName(127), QString { "G-A" });
    QCOMPARE(controller.noteName(128), QString {});
    QCOMPARE(controller.noteName(-1), QString {});
}

void SamplerControllerTest::test_meterLevels_notRecording_shouldReadTheSamplerOutput()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };

    // Nothing has played and the tap is off, so the pair reads its floor -- but it reads, rather than
    // handing the meters nothing to show.
    const auto levels = controller.meterLevels();
    QVERIFY(levels.contains("leftPeakDb"));
    QVERIFY(levels.contains("rightRmsDb"));
    QCOMPARE(levels["leftPeakDb"].toFloat(), StereoLevelMeter::MinimumDb);

    controller.setMetersActive(true);
    QVERIFY(sampler->outputStereoMeter().active());
    controller.setMetersActive(false);
    QVERIFY(!sampler->outputStereoMeter().active());
}

void SamplerControllerTest::test_copyPad_samePad_shouldDoNothing()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };

    controller.loadSample(0, "/samples/kick.wav");
    const auto data = sampler->sample(36)->data;

    controller.copyPad(0, 0);

    QCOMPARE(sampler->sample(36)->data, data);
}

void SamplerControllerTest::test_playbackPosition_chromaticMode_shouldFollowAPitchedNote()
{
    // The dialog asks for the selected pad, which in chromatic mode is the root of an octave, while
    // the note actually playing is somewhere inside that octave. The playhead stood still because
    // the two were compared to each other.
    auto reader = std::make_unique<MockAudioFileReader>();
    reader->setFrames(static_cast<int64_t>(Constants::defaultSampleRate()));
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::move(reader));
    sampler->setChromaticMode(true);
    SamplerController controller { sampler };

    controller.loadSample(3, "/samples/bass.wav"); // Pad 3 is the C3 root in chromatic mode
    controller.setSelectedPad(3);
    QVERIFY(controller.isFinished());

    sampler->processMidiNoteOn(40, 100); // E3, pitched up from that root
    std::vector<double> buffer(256 * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 256, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    sampler->processAudio(context);

    QVERIFY(!controller.isFinished());
    QVERIFY(controller.playbackPosition() > 0.0);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SamplerControllerTest)
