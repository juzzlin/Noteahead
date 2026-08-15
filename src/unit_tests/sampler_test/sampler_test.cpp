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
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/reverb.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"

#include <QTest>

#include <algorithm>
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

    void setTag(TagType type, const std::string & value) override
    {
        (void)type;
        (void)value;
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
    EffectFactory::init(); // Cloning a pad's insert rack builds the effects through the factory
}

void SamplerTest::cleanupTestCase()
{
    EffectFactory::clear();
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

void SamplerTest::test_copySample_shouldCopySampleAndSettings()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSamplePan(60, 0.25f);
    sampler.setSampleVolume(60, 0.75f);
    sampler.setSampleCutoff(60, 0.4f);
    sampler.setSampleHpfCutoff(60, 0.1f);
    sampler.setSampleStartOffset(60, 0.5);

    sampler.copySample(60, 62);

    QVERIFY(sampler.sample(62));
    QCOMPARE(sampler.sample(62)->filePath, sampler.sample(60)->filePath);
    // The sample data is immutable, so the copy shares the buffer instead of re-reading the file
    QCOMPARE(sampler.sample(62)->data, sampler.sample(60)->data);
    QCOMPARE(sampler.samplePan(62), 0.25f);
    QCOMPARE(sampler.sampleVolume(62), 0.75f);
    QCOMPARE(sampler.sampleCutoff(62), 0.4f);
    QCOMPARE(sampler.sampleHpfCutoff(62), 0.1f);
    // The offset is stored as a float parameter, hence the tolerance
    QVERIFY(std::abs(sampler.sampleStartOffset(62) - 0.5) < 1e-6);
    // The source is left untouched
    QCOMPARE(sampler.samplePan(60), 0.25f);
}

void SamplerTest::test_copySample_shouldGiveTargetIndependentEffectRack()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    const auto reverb = std::make_shared<Reverb>();
    reverb->setSize(0.8f);
    sampler.sampleEffectRack(60).setEffect(0, reverb);

    sampler.copySample(60, 62);

    auto & targetRack = sampler.sampleEffectRack(62);
    QVERIFY(&targetRack != &sampler.sampleEffectRack(60));
    const auto copy = std::dynamic_pointer_cast<Reverb>(targetRack.effect(0));
    QVERIFY(copy != nullptr);
    QCOMPARE(copy->size(), 0.8f);
    // Editing the copied rack must not reach back into the source pad
    copy->setSize(0.2f);
    QCOMPARE(reverb->size(), 0.8f);
}

void SamplerTest::test_copySample_emptySource_shouldNotTouchTarget()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(62, "test.wav");

    sampler.copySample(60, 62); // Pad 60 is empty

    QVERIFY(sampler.sample(62));
}

void SamplerTest::test_copySample_sameNote_shouldDoNothing()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    const auto data = sampler.sample(60)->data;

    sampler.copySample(60, 60);

    QCOMPARE(sampler.sample(60)->data, data);
}

void SamplerTest::test_restoreState_shouldRestorePerPadEffects()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    const auto reverb = std::make_shared<Reverb>();
    reverb->setSize(0.8f);
    sampler.sampleEffectRack(60).setEffect(0, reverb);

    sampler.saveState();

    // Everything the dialog can do to a pad's insert rack: re-tune an effect, and add another one
    reverb->setSize(0.1f);
    sampler.sampleEffectRack(60).setEffect(1, std::make_shared<Reverb>());

    sampler.restoreState();

    auto & restoredRack = sampler.sampleEffectRack(60);
    const auto restored = std::dynamic_pointer_cast<Reverb>(restoredRack.effect(0));
    QVERIFY(restored != nullptr);
    QCOMPARE(restored->size(), 0.8f);
    QVERIFY(!restoredRack.effect(1));
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

void SamplerTest::test_restoreState_whilePlaying_shouldNotLeaveVoicesOnFreedSamples()
{
    // Cancelling the Sampler dialog restores the samples it saved on opening, which destroys the
    // ones playing. A voice holds a raw pointer to its sample, so any voice still running through
    // one of them is left pointing at freed memory and the next audio callback reads it. Chromatic
    // mode makes it near certain, since every note plays through the same sample.
    auto mockReader = std::make_unique<MockAudioFileReader>();
    mockReader->setForceChannels(1);
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::move(mockReader) };
    sampler.setChromaticMode(true);
    sampler.loadSample(60, "test.wav");

    // What the dialog does when it opens.
    sampler.saveState();

    sampler.processMidiNoteOn(67, 127);
    std::vector<double> buffer(64, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 32, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    sampler.processAudio(context);
    QVERIFY2(sampler.hasActiveAudio(), "The note did not start, so the test would prove nothing");

    // What Cancel does.
    sampler.restoreState();

    QVERIFY2(!sampler.hasActiveAudio(),
             "A voice outlived the sample it was playing, and the next callback would read freed memory");

    // Which the next callback must survive.
    std::fill(buffer.begin(), buffer.end(), 0.0);
    sampler.processAudio(context);
}

void SamplerTest::test_loadSample_whilePlaying_shouldNotLeaveVoicesOnFreedSamples()
{
    auto mockReader = std::make_unique<MockAudioFileReader>();
    mockReader->setForceChannels(1);
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::move(mockReader) };
    sampler.loadSample(60, "test.wav");
    sampler.processMidiNoteOn(60, 127);

    std::vector<double> buffer(64, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 32, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    sampler.processAudio(context);
    QVERIFY(sampler.hasActiveAudio());

    // Loading over a pad that is sounding destroys the sample the voice is reading.
    sampler.loadSample(60, "other.wav");

    QVERIFY2(!sampler.hasActiveAudio(), "A voice outlived the sample it was playing");

    std::fill(buffer.begin(), buffer.end(), 0.0);
    sampler.processAudio(context);
}

void SamplerTest::test_clearSample_whilePlaying_shouldNotLeaveVoicesOnFreedSamples()
{
    auto mockReader = std::make_unique<MockAudioFileReader>();
    mockReader->setForceChannels(1);
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::move(mockReader) };
    sampler.loadSample(60, "test.wav");
    sampler.processMidiNoteOn(60, 127);

    std::vector<double> buffer(64, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 32, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    sampler.processAudio(context);
    QVERIFY(sampler.hasActiveAudio());

    sampler.clearSample(60);

    QVERIFY2(!sampler.hasActiveAudio(), "A voice outlived the sample it was playing");

    std::fill(buffer.begin(), buffer.end(), 0.0);
    sampler.processAudio(context);
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
    QCOMPARE(sampler.volume(), Constants::faderUnityPosition());
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
    sampler.processMidiCc(7, 127, 0); // Volume to unity
    sampler.processMidiCc(10, 127, 0); // Pan to 1.0
    QCOMPARE(sampler.volume(), Constants::faderUnityPosition());
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
        QCOMPARE(sampler.volume(), Constants::faderUnityPosition());
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
