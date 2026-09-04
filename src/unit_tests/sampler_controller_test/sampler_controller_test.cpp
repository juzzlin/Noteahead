// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
#include "sampler_controller_test.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"
#include "../../view/controllers/sampler_controller.hpp"

#include <QSignalSpy>
#include <QTest>

#include <algorithm>
#include <cmath>

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

void SamplerControllerTest::test_copyPad_samePad_shouldDoNothing()
{
    const auto sampler = std::make_shared<SamplerDevice>("Test Sampler", std::make_unique<MockAudioFileReader>());
    SamplerController controller { sampler };

    controller.loadSample(0, "/samples/kick.wav");
    const auto data = sampler->sample(36)->data;

    controller.copyPad(0, 0);

    QCOMPARE(sampler->sample(36)->data, data);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SamplerControllerTest)
