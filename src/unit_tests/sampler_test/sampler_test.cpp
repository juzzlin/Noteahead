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

#include "sampler_test.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QTest>
#include <cmath>
#include <numbers>

namespace noteahead {

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
        return { 1024, static_cast<int>(Constants::defaultSampleRate()), m_channels, 0 };
    }

    void setForceChannels(int channels)
    {
        m_channels = channels;
    }

private:
    int m_channels = 2;
};

void SamplerTest::initTestCase()
{
}

void SamplerTest::test_initialState_shouldBeCorrect()
{
    const auto samplerName = Constants::samplerDeviceName().toStdString();
    SamplerDevice sampler { samplerName, std::make_unique<MockAudioFileReader>() };
    QCOMPARE(sampler.name(), samplerName);
    QCOMPARE(sampler.gain(), 0.5f);
}

void SamplerTest::test_loadAndClearSample_shouldUpdateModel()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    QVERIFY(sampler.sample(60));
    sampler.clearSample(60);
    QVERIFY(!sampler.sample(60));
}

void SamplerTest::test_midiNoteOn_shouldPlaySample()
{
    auto mockReader = std::make_unique<MockAudioFileReader>();
    mockReader->setForceChannels(1);
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::move(mockReader) };
    sampler.loadSample(60, "test.wav");
    sampler.processMidiNoteOn(60, 127);
    std::vector<double> buffer(4, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 2, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    sampler.processAudio(context);
    QVERIFY(buffer[0] > 0);
}

void SamplerTest::test_processAudio_reusesBuffersWithoutLeaking()
{
    auto mockReader = std::make_unique<MockAudioFileReader>();
    mockReader->setForceChannels(1);
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::move(mockReader) };
    sampler.loadSample(60, "test.wav");
    sampler.processMidiNoteOn(60, 127);

    const auto sampleRate = static_cast<uint32_t>(Constants::defaultSampleRate());

    // First callback: the note is playing, so the output is non-silent.
    std::vector<double> buffer1(8, 0.0);
    AudioContext context1 { std::span(buffer1.data(), buffer1.size()), 4, sampleRate };
    sampler.processAudio(context1);
    QVERIFY(buffer1[0] > 0.0);

    // Stop all voices, then process again. processAudio() reuses a member mix buffer across callbacks;
    // it must be cleared each time, so with no active voice the sampler must add pure silence — any
    // stale data from the previous callback would leak through here.
    sampler.processMidiAllNotesOff();
    std::vector<double> buffer2(8, 0.0);
    AudioContext context2 { std::span(buffer2.data(), buffer2.size()), 4, sampleRate };
    sampler.processAudio(context2);
    for (const double sample : buffer2) {
        QCOMPARE(sample, 0.0);
    }
}

void SamplerTest::test_midiAllNotesOff_shouldStopAllVoices()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.processMidiNoteOn(60, 127);
    sampler.processMidiAllNotesOff();
    std::vector<double> buffer(4, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 2, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    sampler.processAudio(context);
    QCOMPARE(buffer[0], 0.0f);
}

void SamplerTest::test_pan_shouldAdjustPanning()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSamplePan(60, 0.75f);
    QVERIFY(qFuzzyCompare(sampler.samplePan(60), 0.75f));
}

void SamplerTest::test_volume_shouldAdjustVolume()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSampleVolume(60, 0.5f);
    QCOMPARE(sampler.sampleVolume(60), 0.5f);

    sampler.setVolume(0.7f);
    QCOMPARE(sampler.volume(), 0.7f);

    sampler.setGain(0.8f);
    QCOMPARE(sampler.gain(), 0.8f);
}

void SamplerTest::test_cutoff_shouldAdjustCutoff()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSampleCutoff(60, 0.5f);
    QCOMPARE(sampler.sampleCutoff(60), 0.5f);
    sampler.setSampleHpfCutoff(60, 0.5f);
    QCOMPARE(sampler.sampleHpfCutoff(60), 0.5f);
}

void SamplerTest::test_channelMode_shouldToggleCorrectMode()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.setChannelMode(true);
    QCOMPARE(sampler.channelMode(), true);
}

void SamplerTest::test_chromaticMode_shouldToggleCorrectMode()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    QCOMPARE(sampler.chromaticMode(), false);
    sampler.setChromaticMode(true);
    QCOMPARE(sampler.chromaticMode(), true);
}

void SamplerTest::test_chromaticMode_singleSample_shouldCoverWholeRange()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.setChromaticMode(true);
    sampler.loadSample(0, "test.wav"); // Octave 0 root (C0)

    // A single sample covers the entire range: every note resolves to it, pitched from the C0 root.
    uint8_t root = 255;
    QVERIFY(sampler.coveringSample(0, root));
    QCOMPARE(root, static_cast<uint8_t>(0));
    QVERIFY(sampler.coveringSample(60, root));
    QCOMPARE(root, static_cast<uint8_t>(0));
    QVERIFY(sampler.coveringSample(127, root));
    QCOMPARE(root, static_cast<uint8_t>(0));

    QVERIFY(qFuzzyCompare(sampler.chromaticPitchRatio(0), 1.0));
    QVERIFY(qFuzzyCompare(sampler.chromaticPitchRatio(12), 2.0)); // One octave up
}

void SamplerTest::test_chromaticMode_multipleSamples_shouldSelectCoveringSample()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.setChromaticMode(true);
    sampler.loadSample(0, "low.wav"); // C0
    sampler.loadSample(36, "high.wav"); // C3

    // Notes below C3 resolve to the C0 sample, notes at/above C3 to the C3 sample.
    uint8_t root = 255;
    sampler.coveringSample(12, root);
    QCOMPARE(root, static_cast<uint8_t>(0));
    sampler.coveringSample(35, root);
    QCOMPARE(root, static_cast<uint8_t>(0));
    sampler.coveringSample(36, root);
    QCOMPARE(root, static_cast<uint8_t>(36));
    sampler.coveringSample(60, root);
    QCOMPARE(root, static_cast<uint8_t>(36));
}

void SamplerTest::test_chromaticMode_pitch_shouldMatchSemitoneRatio()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.setChromaticMode(true);
    sampler.loadSample(12, "test.wav"); // C1 root

    QVERIFY(qFuzzyCompare(sampler.chromaticPitchRatio(12), 1.0)); // At the root
    QVERIFY(qFuzzyCompare(sampler.chromaticPitchRatio(24), 2.0)); // One octave above
    QVERIFY(qFuzzyCompare(sampler.chromaticPitchRatio(0), 0.5)); // One octave below (lowest root extends down)
}

void SamplerTest::test_chromaticMode_shouldRoundTripThroughXml()
{
    QByteArray data;
    {
        SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
        sampler.setChromaticMode(true);
        NahdXmlWriter writer { data };
        sampler.serializeToXml(writer);
    }

    {
        SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        sampler.deserializeFromXml(reader);
        QCOMPARE(sampler.chromaticMode(), true);
    }
}

void SamplerTest::test_midiCcReset_shouldResetInternalValues()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSamplePan(60, 0.75f);
    sampler.setChannelMode(true);
    sampler.processMidiCc(10, 0, 24);
    sampler.processMidiCc(121, 127, 24);
    QVERIFY(qFuzzyCompare(sampler.samplePan(60), 0.75f));
}

void SamplerTest::test_startOffset_shouldShiftPlaybackStart()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSampleStartOffset(60, 0.01);
    QVERIFY(std::abs(sampler.sampleStartOffset(60) - 0.01) < 0.0001);
}

void SamplerTest::test_reset_shouldResetParametersAndPads()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(36, "test1.wav");
    sampler.loadSample(48, "test2.wav");
    sampler.setVolume(0.1f);
    sampler.setPan(0.1f);
    sampler.setChannelMode(true);

    sampler.reset();

    QCOMPARE(sampler.channelMode(), false);
    QCOMPARE(sampler.volume(), 1.0f);
    QCOMPARE(sampler.pan(), 0.5f);
    for (uint8_t note = 36; note < 36 + 16; note++) {
        QVERIFY(!sampler.sample(note));
    }
}

void SamplerTest::test_processAudio_shouldProduceOutput()
{
    auto mockReader = std::make_unique<MockAudioFileReader>();
    mockReader->setForceChannels(1);
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::move(mockReader) };
    sampler.loadSample(60, "test.wav");
    sampler.processMidiNoteOn(60, 127);
    std::vector<double> buffer(4, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 2, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    sampler.processAudio(context);
    // Constant-power center pan: mono sample 1.0 → cos(π/4) on both channels
    const double expected = std::cos(std::numbers::pi * 0.25);
    QVERIFY(std::abs(buffer[0] - expected) < 1e-10);
    QVERIFY(std::abs(buffer[1] - expected) < 1e-10);
}

void SamplerTest::test_serialization_shouldSaveAndLoadGain()
{
    QByteArray data;
    {
        SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
        sampler.setGain(0.8f);
        sampler.setVolume(0.4f);
        NahdXmlWriter writer { data };
        sampler.serializeToXml(writer);
    }

    {
        SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        sampler.deserializeFromXml(reader);
        QCOMPARE(sampler.gain(), 0.8f);
        QCOMPARE(sampler.volume(), 0.4f);
    }
}

void SamplerTest::test_midiCcResetGlobalPanAndVolume_shouldRestoreManualValues()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };

    // 1. Initial manual state
    sampler.setVolume(0.8f);
    sampler.setPan(0.2f);
    sampler.setGain(0.6f);

    // 2. Change via MIDI CC
    sampler.processMidiCc(7, 127, 0); // Volume to 1.0
    sampler.processMidiCc(10, 127, 0); // Pan to 1.0
    QCOMPARE(sampler.volume(), 1.0f);
    QCOMPARE(sampler.pan(), 1.0f);

    // 3. Reset All Controllers (CC 121)
    sampler.processMidiCc(121, 0, 0);

    // 4. Should restore to manual values
    QCOMPARE(sampler.volume(), 0.8f);
    QCOMPARE(sampler.pan(), 0.2f);
    QCOMPARE(sampler.gain(), 0.6f);
}

void SamplerTest::test_projectLoadMidiCcResetGlobal_shouldRestoreLoadedValues()
{
    QByteArray data;
    {
        SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
        sampler.setVolume(0.4f);
        sampler.setPan(0.6f);
        sampler.setGain(0.7f);
        NahdXmlWriter writer { data };
        sampler.serializeToXml(writer);
    }

    {
        SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
        NahdXmlReader reader { data };
        while (!reader.atEnd() && !reader.isStartElement()) {
            reader.readNext();
        }
        sampler.deserializeFromXml(reader);

        QCOMPARE(sampler.volume(), 0.4f);
        QCOMPARE(sampler.pan(), 0.6f);
        QCOMPARE(sampler.gain(), 0.7f);

        // Change via MIDI CC
        sampler.processMidiCc(7, 127, 0);
        sampler.processMidiCc(10, 127, 0);
        QCOMPARE(sampler.volume(), 1.0f);
        QCOMPARE(sampler.pan(), 1.0f);

        // Reset All Controllers
        sampler.processMidiCc(121, 0, 0);

        // Should return to LOADED values
        QCOMPARE(sampler.volume(), 0.4f);
        QCOMPARE(sampler.pan(), 0.6f);
        QCOMPARE(sampler.gain(), 0.7f);
    }
}

void SamplerTest::test_loadSample_relativePath_shouldWorkWithProjectPath()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };

    const std::string projectPath { "/tmp/noteahead_test" };
    sampler.setProjectPath(projectPath);

    const std::string relativePath { "samples/kick.wav" };
    sampler.loadSample(60, relativePath);

    const auto expectedPath { QDir { QString::fromStdString(projectPath) }.absoluteFilePath(QString::fromStdString(relativePath)).toStdString() };
    QCOMPARE(sampler.absoluteFilePath(60), expectedPath);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SamplerTest)
