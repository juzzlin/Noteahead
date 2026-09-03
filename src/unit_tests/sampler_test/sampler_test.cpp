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
#include "../../infra/midi/midi_cc_mapping.hpp"
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
        if (m_step >= 0) {
            for (size_t i = 0; i < data.size(); i++) {
                data[i] = static_cast<int64_t>(i) < m_step ? 0.0f : 1000.0f;
            }
        } else if (m_ramp) {
            // A constant sample reads the same in both directions, so the tests that care which way
            // playback runs need something that does not.
            for (size_t i = 0; i < data.size(); i++) {
                data[i] = static_cast<float>(i);
            }
        } else {
            std::fill(data.begin(), data.end(), 1.0f);
        }
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
        return { m_frames, static_cast<int>(Constants::defaultSampleRate()), m_channels, 0 };
    }

    void setForceChannels(int channels)
    {
        m_channels = channels;
    }

    //! Lets a test hand out a shorter file on the next open, which is the only way to tell a
    //! replacement sample apart from the one it replaces.
    void setFrames(int64_t frames)
    {
        m_frames = frames;
    }

    //! Fills the sample with its own frame index instead of a constant, so a test can tell which frame
    //! the device is reading.
    void setRamp(bool ramp)
    {
        m_ramp = ramp;
    }

    //! Silence up to the given frame and a loud constant from there on. A loop ending at that frame
    //! plays pure silence, so anything the device reads past the loop end is impossible to miss.
    void setStepAt(int64_t frame)
    {
        m_step = frame;
    }

private:
    int m_channels = 2;
    int64_t m_frames = 1024;
    bool m_ramp = false;
    int64_t m_step = -1;
};

namespace {

//! A mono sampler holding one second of the given content on note 60, which is long enough for the
//! trimming and envelope tests to run past the times they set without running out of sample.
std::unique_ptr<SamplerDevice> makeMonoSampler(bool ramp = false)
{
    auto reader = std::make_unique<MockAudioFileReader>();
    reader->setForceChannels(1);
    reader->setFrames(static_cast<int64_t>(Constants::defaultSampleRate()));
    reader->setRamp(ramp);
    auto sampler = std::make_unique<SamplerDevice>(Constants::samplerDeviceName().toStdString(), std::move(reader));
    sampler->loadSample(60, "test.wav");
    return sampler;
}

//! Runs frameCount frames through the device and hands back the interleaved output.
std::vector<double> render(SamplerDevice & sampler, uint32_t frameCount)
{
    std::vector<double> buffer(frameCount * 2, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), frameCount, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    sampler.processAudio(context);
    return buffer;
}

} // namespace

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

void SamplerTest::test_loadSample_ontoLoadedPad_shouldKeepItsSettingsAndEffects()
{
    // Changing the file on a pad is a substitution, not a fresh start: the rack and everything the
    // pad was dialled in with belong to the pad rather than to the file that was on it.
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "first.wav");
    sampler.setSamplePan(60, 0.25f);
    sampler.setSampleVolume(60, 0.75f);
    sampler.setSampleCutoff(60, 0.4f);
    sampler.setSampleHpfCutoff(60, 0.1f);
    const auto reverb = std::make_shared<Reverb>();
    reverb->setSize(0.8f);
    sampler.sampleEffectRack(60).setEffect(0, reverb);

    sampler.loadSample(60, "second.wav");

    QVERIFY(sampler.sample(60));
    QVERIFY(sampler.sample(60)->filePath.find("second.wav") != std::string::npos);
    QCOMPARE(sampler.samplePan(60), 0.25f);
    QCOMPARE(sampler.sampleVolume(60), 0.75f);
    QCOMPARE(sampler.sampleCutoff(60), 0.4f);
    QCOMPARE(sampler.sampleHpfCutoff(60), 0.1f);
    const auto kept = std::dynamic_pointer_cast<Reverb>(sampler.sampleEffectRack(60).effect(0));
    QVERIFY(kept != nullptr);
    QCOMPARE(kept->size(), 0.8f);
}

void SamplerTest::test_loadSample_ontoEmptyPad_shouldStartFromTheDefaults()
{
    // Nothing to carry over, so a fresh pad must not inherit whatever a neighbour was set to.
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "first.wav");
    sampler.setSamplePan(60, 0.25f);

    sampler.loadSample(62, "second.wav");

    QCOMPARE(sampler.samplePan(62), 0.5f);
}

void SamplerTest::test_loadSample_shorterFile_shouldPullTheStartOffsetInside()
{
    // The start offset is the one setting that is about the file. Carrying it past the end of a
    // shorter replacement would leave the pad silent with no visible reason why.
    auto reader = std::make_unique<MockAudioFileReader>();
    auto * mock = reader.get();
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::move(reader) };
    sampler.loadSample(60, "long.wav");
    const auto duration = sampler.sampleDuration(60);
    QVERIFY(duration > 0.0);
    sampler.setSampleStartOffset(60, duration);
    QVERIFY(std::abs(sampler.sampleStartOffset(60) - duration) < 1e-6);

    mock->setFrames(256);
    sampler.loadSample(60, "short.wav");

    const auto shorterDuration = sampler.sampleDuration(60);
    QVERIFY2(shorterDuration < duration, "the replacement was not actually shorter");
    QVERIFY2(sampler.sampleStartOffset(60) <= shorterDuration + 1e-6,
             qPrintable(QString { "offset %1 is past the end of a %2 s sample" }.arg(sampler.sampleStartOffset(60)).arg(shorterDuration)));
}

void SamplerTest::test_copySample_shouldCopySampleAndSettings()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSamplePan(60, 0.25f);
    sampler.setSampleVolume(60, 0.75f);
    sampler.setSampleCutoff(60, 0.4f);
    sampler.setSampleHpfCutoff(60, 0.1f);
    sampler.setSampleStartOffset(60, 0.01); // Inside the sample: an offset past its end is clamped away

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
    QVERIFY(std::abs(sampler.sampleStartOffset(62) - 0.01) < 1e-6);
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

void SamplerTest::test_availableMidiCcControllers_shouldListGlobalsAndAllPads()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };

    const auto controllers = sampler.availableMidiCcControllers();

    // Fader + Pan + LPF + HPF + (16 pads * 4 CCs per pad)
    QCOMPARE(controllers.size(), size_t { 4 + SamplerDevice::padCount * 4 });

    // An unknown number yields a default-constructed entry, which fails the comparisons below
    const auto byNumber = [&controllers](int number) -> MidiCcController {
        const auto it = std::ranges::find(controllers, static_cast<uint8_t>(number), &MidiCcController::number);
        return it != controllers.end() ? *it : MidiCcController {};
    };

    QCOMPARE(byNumber(SamplerDevice::padPanCcStart).name, std::string { "Pad 1 Pan" });
    QCOMPARE(byNumber(SamplerDevice::padCutoffCcStart + 15).name, std::string { "Pad 16 LPF" });
    QCOMPARE(byNumber(SamplerDevice::padHpfCutoffCcStart + 15).name, std::string { "Pad 16 HPF" });

    // The pad fader reaches past unity, so it must advertise the extended range and not MIDI 1.0's
    const auto padVolume = byNumber(SamplerDevice::padVolumeCcStart);
    QCOMPARE(padVolume.name, std::string { "Pad 1 Volume" });
    QCOMPARE(padVolume.maxValue, Constants::faderMaxMidiCcValue());

    // The note rides along for the presentation layer to name; device-wide CCs drive no single note
    QCOMPARE(byNumber(SamplerDevice::padPanCcStart).note, std::optional<uint8_t> { SamplerDevice::padStartNote });
    QCOMPARE(byNumber(SamplerDevice::padPanCcStart + 15).note, std::optional<uint8_t> { SamplerDevice::padStartNote + 15 });
    QCOMPARE(byNumber(static_cast<int>(MidiCcMapping::Controller::PanMSB)).note, std::optional<uint8_t> {});
}

void SamplerTest::test_availableMidiCcControllers_chromaticMode_shouldFollowTheOctaveRoots()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.setChromaticMode(true);

    const auto controllers = sampler.availableMidiCcControllers();
    const auto byNumber = [&controllers](int number) -> MidiCcController {
        const auto it = std::ranges::find(controllers, static_cast<uint8_t>(number), &MidiCcController::number);
        return it != controllers.end() ? *it : MidiCcController {};
    };

    QCOMPARE(byNumber(SamplerDevice::padPanCcStart).note, std::optional<uint8_t> { 0 });
    QCOMPARE(byNumber(SamplerDevice::padPanCcStart + 2).note, std::optional<uint8_t> { 24 });
    // Pad 12 would be note 132: past the end of the keyboard, so it has no note to name
    QCOMPARE(byNumber(SamplerDevice::padPanCcStart + 11).note, std::optional<uint8_t> {});
}

void SamplerTest::test_processMidiCc_padPan_shouldAffectOnlyThatPad()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(SamplerDevice::padStartNote, "pad1.wav");
    sampler.loadSample(SamplerDevice::padStartNote + 1, "pad2.wav");

    sampler.processMidiCc(SamplerDevice::padPanCcStart, 0, 0);

    QVERIFY(qFuzzyIsNull(sampler.samplePan(SamplerDevice::padStartNote)));
    QVERIFY(qFuzzyCompare(sampler.samplePan(SamplerDevice::padStartNote + 1), 0.5f));

    sampler.processMidiCc(SamplerDevice::padPanCcStart + 1, 127, 0);

    QVERIFY(qFuzzyIsNull(sampler.samplePan(SamplerDevice::padStartNote)));
    QVERIFY(qFuzzyCompare(sampler.samplePan(SamplerDevice::padStartNote + 1), 1.0f));
}

void SamplerTest::test_processMidiCc_padVolume_shouldUseFaderRange()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(SamplerDevice::padStartNote, "test.wav");

    // 127 still means unity, exactly as it did before the fader gained its boost range
    sampler.processMidiCc(SamplerDevice::padVolumeCcStart, 127, 0);
    QCOMPARE(sampler.sampleVolume(SamplerDevice::padStartNote), Constants::faderUnityPosition());

    // Only the values above it reach into the boost range
    sampler.processMidiCc(SamplerDevice::padVolumeCcStart, static_cast<uint8_t>(Constants::faderMaxMidiCcValue()), 0);
    QCOMPARE(sampler.sampleVolume(SamplerDevice::padStartNote), 1.0f);
}

void SamplerTest::test_processMidiCc_padCutoffAndHpfCutoff_shouldReachThePad()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(SamplerDevice::padStartNote + 2, "test.wav");

    sampler.processMidiCc(SamplerDevice::padCutoffCcStart + 2, 0, 0);
    sampler.processMidiCc(SamplerDevice::padHpfCutoffCcStart + 2, 127, 0);

    QVERIFY(qFuzzyIsNull(sampler.sampleCutoff(SamplerDevice::padStartNote + 2)));
    QVERIFY(qFuzzyCompare(sampler.sampleHpfCutoff(SamplerDevice::padStartNote + 2), 1.0f));
}

void SamplerTest::test_processMidiCc_padPan_shouldUpdateActiveVoice()
{
    // Pad values ride the voices already playing, which is the whole point of automating them: a pad
    // holding a long sample has to follow the CC instead of waiting for the next note.
    auto mockReader = std::make_unique<MockAudioFileReader>();
    mockReader->setForceChannels(1);
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::move(mockReader) };
    sampler.loadSample(SamplerDevice::padStartNote, "test.wav");
    sampler.processMidiNoteOn(SamplerDevice::padStartNote, 127);

    std::vector<double> buffer(4, 0.0);
    AudioContext context { std::span(buffer.data(), buffer.size()), 2, static_cast<uint32_t>(Constants::defaultSampleRate()) };
    sampler.processAudio(context);
    QVERIFY(std::abs(buffer[0] - buffer[1]) < 1e-10); // Centred to begin with

    sampler.processMidiCc(SamplerDevice::padPanCcStart, 0, 0); // Hard left

    std::ranges::fill(buffer, 0.0);
    sampler.processAudio(context);
    QVERIFY(buffer[0] > buffer[1]);
    QVERIFY(std::abs(buffer[1]) < 1e-10);
}

void SamplerTest::test_processMidiCc_padCc_channelModeEnabled_shouldStillApply()
{
    // The per-pad CCs are disjoint from the controllers channel mode reads, so they address the pad
    // by CC number no matter which channel they arrive on.
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.setChannelMode(true);
    sampler.loadSample(SamplerDevice::padStartNote, "pad1.wav");
    sampler.loadSample(SamplerDevice::padStartNote + 5, "pad6.wav");

    sampler.processMidiCc(SamplerDevice::padPanCcStart, 0, 5);

    QVERIFY(qFuzzyIsNull(sampler.samplePan(SamplerDevice::padStartNote)));
    QVERIFY(qFuzzyCompare(sampler.samplePan(SamplerDevice::padStartNote + 5), 0.5f));
}

void SamplerTest::test_processMidiCc_padCc_chromaticMode_shouldTargetOctaveRoot()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.setChromaticMode(true);
    sampler.loadSample(24, "test.wav"); // C2, the root of pad 3

    sampler.processMidiCc(SamplerDevice::padPanCcStart + 2, 0, 0);

    QVERIFY(qFuzzyIsNull(sampler.samplePan(24)));
}

void SamplerTest::test_processMidiCc_padCc_chromaticModeOutOfRange_shouldBeIgnored()
{
    // The chromatic layout runs off the end of the sample array on the topmost pads: pad 12 would be
    // note 132. Those CCs have nothing to address and must not touch anything.
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.setChromaticMode(true);
    sampler.loadSample(0, "test.wav");

    sampler.processMidiCc(SamplerDevice::padPanCcStart + 11, 0, 0);

    QVERIFY(qFuzzyCompare(sampler.samplePan(0), 0.5f));
}

void SamplerTest::test_processMidiCc_padPan_shouldNotDisturbDeviceWidePan()
{
    // A pad's pan is combined with the device-wide pan, so automating the pad must leave the device
    // one alone: setting a pad's pan over MIDI has to sound exactly like setting it from the dialog.
    const auto render = [](bool overMidi) {
        auto mockReader = std::make_unique<MockAudioFileReader>();
        mockReader->setForceChannels(1);
        SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::move(mockReader) };
        sampler.loadSample(SamplerDevice::padStartNote, "test.wav");
        sampler.setPan(0.75f);
        sampler.processMidiNoteOn(SamplerDevice::padStartNote, 127);

        if (overMidi) {
            sampler.processMidiCc(SamplerDevice::padPanCcStart, 32, 0);
        } else {
            sampler.setSamplePan(SamplerDevice::padStartNote, 32.0f / 127.0f);
        }

        std::vector<double> buffer(4, 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), 2, static_cast<uint32_t>(Constants::defaultSampleRate()) };
        sampler.processAudio(context);
        return buffer;
    };

    const auto viaMidi = render(true);
    const auto viaSetter = render(false);

    QCOMPARE(viaMidi.size(), viaSetter.size());
    for (size_t i = 0; i < viaMidi.size(); i++) {
        QVERIFY(std::abs(viaMidi.at(i) - viaSetter.at(i)) < 1e-10);
    }
}

void SamplerTest::test_midiCcReset_padCc_shouldRestoreManualValues()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(SamplerDevice::padStartNote, "test.wav");
    sampler.setSamplePan(SamplerDevice::padStartNote, 0.25f);
    sampler.setSampleCutoff(SamplerDevice::padStartNote, 0.4f);

    sampler.processMidiCc(SamplerDevice::padPanCcStart, 127, 0);
    sampler.processMidiCc(SamplerDevice::padCutoffCcStart, 0, 0);
    QVERIFY(qFuzzyCompare(sampler.samplePan(SamplerDevice::padStartNote), 1.0f));

    sampler.processMidiCc(121, 127, 0); // Reset All Controllers

    QVERIFY(qFuzzyCompare(sampler.samplePan(SamplerDevice::padStartNote), 0.25f));
    QVERIFY(qFuzzyCompare(sampler.sampleCutoff(SamplerDevice::padStartNote), 0.4f));
}

void SamplerTest::test_startOffset_shouldShiftPlaybackStart()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSampleStartOffset(60, 0.01);
    QVERIFY(std::abs(sampler.sampleStartOffset(60) - 0.01) < 0.0001);
}

void SamplerTest::test_endOffset_zero_shouldPlayToTheEndOfTheSample()
{
    // Zero is no trim at all, which is both the default and the only way to clear the setting.
    const auto sampler = makeMonoSampler();
    QCOMPARE(sampler->sampleEndOffset(60), 0.0);

    sampler->processMidiNoteOn(60, 127);
    // Half a second in, a sample a second long is still going.
    render(*sampler, static_cast<uint32_t>(Constants::defaultSampleRate()) / 2);
    QVERIFY(!sampler->isFinished(60));
}

void SamplerTest::test_offsets_pastTheEndOfTheSample_shouldBeClamped()
{
    // Neither trim can take off more than the sample holds. Left unchecked they park at a value the pad
    // can never reach, which reads as a pad that has silently stopped working.
    const auto sampler = makeMonoSampler();
    const auto duration = sampler->sampleDuration(60);

    sampler->setSampleStartOffset(60, duration * 10.0);
    QVERIFY(sampler->sampleStartOffset(60) <= duration + 1e-6);

    sampler->setSampleEndOffset(60, duration * 10.0);
    QVERIFY(sampler->sampleEndOffset(60) <= duration + 1e-6);

    // And neither goes negative.
    sampler->setSampleEndOffset(60, -1.0);
    QCOMPARE(sampler->sampleEndOffset(60), 0.0);
}

void SamplerTest::test_endOffset_shouldTrimThatMuchOffTheEnd()
{
    // The offset counts back from the end, so 0.9 s off a one second pad leaves a tenth of a second.
    const auto sampler = makeMonoSampler();
    sampler->setSampleEndOffset(60, 0.9);
    QVERIFY(std::abs(sampler->sampleEndOffset(60) - 0.9) < 0.001);

    sampler->processMidiNoteOn(60, 127);
    // Just short of what is left, the voice is still sounding...
    render(*sampler, static_cast<uint32_t>(Constants::defaultSampleRate() * 0.09));
    QVERIFY(!sampler->isFinished(60));
    // ...and past it, it is gone, well before the end of the second the sample actually holds.
    render(*sampler, static_cast<uint32_t>(Constants::defaultSampleRate() * 0.02));
    QVERIFY(sampler->isFinished(60));
}

void SamplerTest::test_endOffset_beforeStartOffset_shouldPlayNothing()
{
    const auto sampler = makeMonoSampler();
    sampler->setSampleStartOffset(60, 0.5);
    sampler->setSampleEndOffset(60, 0.6); // The two trims overlap, leaving nothing between them

    sampler->processMidiNoteOn(60, 127);
    QVERIFY(sampler->isFinished(60));

    const auto buffer = render(*sampler, 16);
    QVERIFY(std::all_of(buffer.begin(), buffer.end(), [](double v) { return v == 0.0; }));
}

void SamplerTest::test_loadSample_shorterFile_shouldPullTheEndOffsetInside()
{
    // Same reasoning as the start offset: an end left past the end of a shorter replacement would trim
    // nothing, silently changing what the pad does when the file behind it is swapped.
    auto reader = std::make_unique<MockAudioFileReader>();
    auto * mock = reader.get();
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::move(reader) };
    mock->setFrames(static_cast<int64_t>(Constants::defaultSampleRate()));
    sampler.loadSample(60, "long.wav");
    const auto duration = sampler.sampleDuration(60);
    sampler.setSampleEndOffset(60, duration);

    mock->setFrames(256);
    sampler.loadSample(60, "short.wav");

    const auto shorterDuration = sampler.sampleDuration(60);
    QVERIFY2(shorterDuration < duration, "the replacement was not actually shorter");
    QVERIFY2(sampler.sampleEndOffset(60) <= shorterDuration + 1e-6,
             qPrintable(QString { "a %1 s trim is more than the %2 s sample holds" }.arg(sampler.sampleEndOffset(60)).arg(shorterDuration)));
}

void SamplerTest::test_tune_coarse_shouldSnapToWholeSemitones()
{
    const auto sampler = makeMonoSampler();
    QCOMPARE(SamplerDevice::tuneSemitones(sampler->sampleTune(60)), 0);

    // A position three and a bit semitones up is three semitones up, and reads back as exactly that
    // rather than as the place the knob was let go of.
    sampler->setSampleTune(60, 0.5f + 3.4f / 48.0f);
    QCOMPARE(SamplerDevice::tuneSemitones(sampler->sampleTune(60)), 3);
    QVERIFY(std::abs(sampler->sampleTune(60) - (0.5f + 3.0f / 48.0f)) < 1e-4f);

    // The ends of the travel are the full two octaves either way.
    sampler->setSampleTune(60, 0.0f);
    QCOMPARE(SamplerDevice::tuneSemitones(sampler->sampleTune(60)), -24);
    sampler->setSampleTune(60, 1.0f);
    QCOMPARE(SamplerDevice::tuneSemitones(sampler->sampleTune(60)), 24);
}

void SamplerTest::test_tune_octaveUp_shouldDoublePlaybackRate()
{
    const auto untuned = makeMonoSampler();
    untuned->processMidiNoteOn(60, 127);
    render(*untuned, 512);
    const auto plainPosition = untuned->playbackPosition(60);
    QVERIFY(plainPosition > 0.0);

    const auto tuned = makeMonoSampler();
    tuned->setSampleTune(60, 0.75f); // +12 semitones, so twice the rate
    QCOMPARE(SamplerDevice::tuneSemitones(tuned->sampleTune(60)), 12);
    tuned->processMidiNoteOn(60, 127);
    render(*tuned, 512);

    QVERIFY2(std::abs(tuned->playbackPosition(60) - plainPosition * 2.0) < plainPosition * 0.01,
             qPrintable(QString { "expected %1, got %2" }.arg(plainPosition * 2.0).arg(tuned->playbackPosition(60))));
}

void SamplerTest::test_tune_fine_shouldDetuneWithinASemitone()
{
    const auto sampler = makeMonoSampler();
    QCOMPARE(SamplerDevice::detuneCents(sampler->sampleDetune(60)), 0.0);

    sampler->setSampleDetune(60, 1.0f);
    QVERIFY(std::abs(SamplerDevice::detuneCents(sampler->sampleDetune(60)) - 100.0) < 0.5);

    // A hundred cents up is a semitone up, whichever control it was dialled in on.
    const auto semitone = std::pow(2.0, 1.0 / 12.0);
    QVERIFY(std::abs(SamplerDevice::tuneRatio(*sampler->sample(60)) - semitone) < 1e-3);
}

void SamplerTest::test_reverse_shouldPlayFromTheEndOfTheRange()
{
    const auto sampler = makeMonoSampler(true);
    sampler->setSampleReverse(60, true);
    QVERIFY(sampler->sampleReverse(60));

    sampler->processMidiNoteOn(60, 127);
    const auto buffer = render(*sampler, 16);

    // The ramp counts up with the frame index, so playing backwards starts near the top of it and the
    // output falls away from there.
    QVERIFY2(buffer[0] > buffer[2], "the output is not running backwards through the sample");
    QVERIFY(buffer[0] > static_cast<double>(Constants::defaultSampleRate()) * 0.5);
    // And the position walks back towards the start rather than away from it.
    QVERIFY(sampler->playbackPosition(60) < 1.0);
    QVERIFY(sampler->playbackPosition(60) > 0.9);
}

void SamplerTest::test_reverse_startOffset_shouldTrimWhatIsHeardFirst()
{
    // The offsets are read against the waveform as it sounds, which for a reversed pad is the file back
    // to front. Trimming the start therefore cuts into the tail of the file.
    const auto sampler = makeMonoSampler(true);
    sampler->setSampleReverse(60, true);
    sampler->setSampleStartOffset(60, 0.25);

    sampler->processMidiNoteOn(60, 127);
    render(*sampler, 16);

    // A quarter of a second in from the end of a one second sample, not from its beginning.
    QVERIFY2(std::abs(sampler->playbackPosition(60) - 0.75) < 0.01,
             qPrintable(QString { "started at %1" }.arg(sampler->playbackPosition(60))));
}

void SamplerTest::test_reverse_endOffset_shouldTrimWhatIsHeardLast()
{
    const auto sampler = makeMonoSampler(true);
    sampler->setSampleReverse(60, true);
    sampler->setSampleEndOffset(60, 0.25);

    sampler->processMidiNoteOn(60, 127);
    render(*sampler, 16);

    // Playing back to front still begins at the end of the file: the end offset trims the far end,
    // which here is the head of the file.
    QVERIFY2(sampler->playbackPosition(60) > 0.99,
             qPrintable(QString { "started at %1" }.arg(sampler->playbackPosition(60))));

    // Three quarters of a second of material rather than the whole second.
    render(*sampler, static_cast<uint32_t>(Constants::defaultSampleRate() * 0.7));
    QVERIFY(!sampler->isFinished(60));
    render(*sampler, static_cast<uint32_t>(Constants::defaultSampleRate() * 0.1));
    QVERIFY(sampler->isFinished(60));
}

void SamplerTest::test_ampEnvelope_defaults_shouldNotAttenuateTheSample()
{
    // The envelope defaults have to leave the pads exactly as they were, so a project made before it
    // existed plays back unchanged rather than merely close.
    const auto sampler = makeMonoSampler();
    sampler->processMidiNoteOn(60, 127);
    const auto buffer = render(*sampler, 64);

    const auto expected = std::cos(std::numbers::pi * 0.25);
    for (size_t i = 0; i < buffer.size(); i++) {
        QVERIFY2(std::abs(buffer[i] - expected) < 1e-10,
                 qPrintable(QString { "frame %1 is %2, not %3" }.arg(i).arg(buffer[i]).arg(expected)));
    }
}

void SamplerTest::test_ampEnvelope_zeroSustain_shouldDropTheVoiceWithoutANoteOff()
{
    // A percussive envelope parks at its zero sustain rather than going idle. A voice that waited for a
    // note-off would render silence at full cost for the rest of the pattern.
    const auto sampler = makeMonoSampler();
    sampler->setSampleSustain(60, 0.0f);
    sampler->processMidiNoteOn(60, 127);
    QVERIFY(!sampler->isFinished(60));

    render(*sampler, 4096);
    QVERIFY(sampler->isFinished(60));
}

void SamplerTest::test_ampEnvelope_noteOff_shouldReleaseRatherThanCutOff()
{
    const auto sampler = makeMonoSampler();
    sampler->setSampleRelease(60, 0.5f); // Well over a second, so the release is still running at the end
    sampler->processMidiNoteOn(60, 127);
    render(*sampler, 64);
    sampler->processMidiNoteOff(60);

    const auto buffer = render(*sampler, 64);
    QVERIFY2(!sampler->isFinished(60), "the voice was cut off instead of released");
    // Still sounding, and on its way down rather than held.
    QVERIFY(std::abs(buffer[0]) > 0.0);
    QVERIFY2(std::abs(buffer[buffer.size() - 2]) < std::abs(buffer[0]), "the release is not falling");
}

void SamplerTest::test_ampEnvelope_attack_shouldRampTheSampleIn()
{
    const auto sampler = makeMonoSampler();
    sampler->setSampleAttack(60, 1.0f); // The top of the travel, ten seconds
    sampler->processMidiNoteOn(60, 127);

    const auto buffer = render(*sampler, 64);
    const auto expected = std::cos(std::numbers::pi * 0.25);
    // A ten second attack has barely started after a millisecond and a half.
    QVERIFY2(buffer[0] < expected * 0.01, "the attack did not ramp the sample in");
    QVERIFY2(buffer[buffer.size() - 2] > buffer[0], "the attack is not rising");
}

void SamplerTest::test_loop_shouldWrapWithinTheRange()
{
    const auto sampler = makeMonoSampler(true);
    sampler->setSampleEndOffset(60, 0.9); // leaves 4410 frames of the 44100 the pad holds
    sampler->setSampleLoop(60, true);
    sampler->processMidiNoteOn(60, 127);

    // Nearly twice around the loop. Without wrapping the voice would have run out long ago.
    render(*sampler, 8000);

    QVERIFY2(!sampler->isFinished(60), "the looping voice ended at the end of its range");
    const auto frame = sampler->playbackPosition(60) * Constants::defaultSampleRate();
    QVERIFY2(frame >= 0.0 && frame <= 4410.0,
             qPrintable(QString { "position %1 is outside the loop" }.arg(frame)));
}

void SamplerTest::test_loop_shouldNotInterpolateAcrossTheSeam()
{
    // The frame after the loop end is not the frame the loop wraps to, and interpolating towards it
    // drags material from outside the loop into every wrap -- a buzz at the loop's own rate.
    //
    // The sample is silent up to frame 441 and very loud from there on, and the loop ends exactly at
    // 441. Everything the loop plays is therefore silence, and reading one frame too far is not a
    // subtle error but a full-scale spike.
    constexpr int64_t stepFrame = 441;
    auto reader = std::make_unique<MockAudioFileReader>();
    reader->setForceChannels(1);
    reader->setFrames(static_cast<int64_t>(Constants::defaultSampleRate()));
    reader->setStepAt(stepFrame);
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::move(reader) };
    sampler.loadSample(60, "step.wav");

    // Trimmed back to the step, so everything the loop plays is the silence in front of it.
    sampler.setSampleEndOffset(60, (Constants::defaultSampleRate() - static_cast<double>(stepFrame)) / Constants::defaultSampleRate());
    sampler.setSampleLoop(60, true);
    // Detuned so the read position lands between frames; on whole frames the seam never shows.
    sampler.setSampleDetune(60, 0.75f);

    sampler.processMidiNoteOn(60, 127);
    const auto buffer = render(sampler, 4410); // Ten times round the loop

    double peak = 0.0;
    for (const double v : buffer) {
        peak = std::max(peak, std::abs(v));
    }
    QVERIFY2(peak < 1.0, qPrintable(QString { "peak %1 -- the loop read past its own end" }.arg(peak)));
}

void SamplerTest::test_loop_disabled_shouldStopAtTheEndOfTheRange()
{
    const auto sampler = makeMonoSampler(true);
    sampler->setSampleEndOffset(60, 0.9);
    sampler->processMidiNoteOn(60, 127);

    render(*sampler, 8000);

    QVERIFY(sampler->isFinished(60));
}

void SamplerTest::test_loop_reverse_shouldWrapBackToTheEnd()
{
    const auto sampler = makeMonoSampler(true);
    sampler->setSampleEndOffset(60, 0.9);
    sampler->setSampleLoop(60, true);
    sampler->setSampleReverse(60, true);
    sampler->processMidiNoteOn(60, 127);

    // Reversed, the loop is the last 4410 frames of the file. It starts at the very end and walks back
    // past the loop start, so it has to have come round to the top again.
    render(*sampler, 8000);

    QVERIFY(!sampler->isFinished(60));
    const auto frame = sampler->playbackPosition(60) * Constants::defaultSampleRate();
    const auto loopStart = Constants::defaultSampleRate() - 1.0 - 4410.0;
    QVERIFY2(frame >= loopStart && frame <= Constants::defaultSampleRate(),
             qPrintable(QString { "position %1 is outside the loop" }.arg(frame)));
}

void SamplerTest::test_loop_noteOff_shouldEndTheVoiceThroughTheRelease()
{
    // A looping voice never runs off its range, so the amp envelope is the only thing left that can
    // end it. If that ever stops holding, a looping pad pins a voice for the rest of the song.
    const auto sampler = makeMonoSampler();
    sampler->setSampleEndOffset(60, 0.9);
    sampler->setSampleLoop(60, true);
    sampler->processMidiNoteOn(60, 127);

    render(*sampler, 8000);
    QVERIFY(!sampler->isFinished(60));

    sampler->processMidiNoteOff(60);
    render(*sampler, 1024); // The default release is a few milliseconds

    QVERIFY(sampler->isFinished(60));
}

void SamplerTest::test_chokeGroup_shouldSilenceOtherPadsInTheSameGroup()
{
    const auto sampler = makeMonoSampler();
    sampler->loadSample(62, "other.wav");
    sampler->setSampleChokeGroup(60, 1);
    sampler->setSampleChokeGroup(62, 1);
    QCOMPARE(sampler->sampleChokeGroup(60), 1);

    sampler->processMidiNoteOn(60, 127);
    render(*sampler, 256);
    QVERIFY(!sampler->isFinished(60));

    sampler->processMidiNoteOn(62, 127);
    render(*sampler, 1024); // Long enough for the choke fade to run out

    QVERIFY2(sampler->isFinished(60), "the open pad was not choked");
    QVERIFY2(!sampler->isFinished(62), "the pad doing the choking stopped too");
}

void SamplerTest::test_chokeGroup_zero_shouldChokeNothing()
{
    const auto sampler = makeMonoSampler();
    sampler->loadSample(62, "other.wav");
    QCOMPARE(sampler->sampleChokeGroup(60), 0);

    sampler->processMidiNoteOn(60, 127);
    sampler->processMidiNoteOn(62, 127);
    render(*sampler, 1024);

    QVERIFY(!sampler->isFinished(60));
    QVERIFY(!sampler->isFinished(62));
}

void SamplerTest::test_chokeGroup_differentGroups_shouldNotInterfere()
{
    const auto sampler = makeMonoSampler();
    sampler->loadSample(62, "other.wav");
    sampler->setSampleChokeGroup(60, 1);
    sampler->setSampleChokeGroup(62, 2);

    sampler->processMidiNoteOn(60, 127);
    sampler->processMidiNoteOn(62, 127);
    render(*sampler, 1024);

    QVERIFY(!sampler->isFinished(60));
    QVERIFY(!sampler->isFinished(62));
}

void SamplerTest::test_chokeGroup_shouldNotChokeTheTriggeringPad()
{
    // One sample covers a whole octave in chromatic mode. Letting a pad choke itself would quietly
    // make any grouped pad monophonic across its own range.
    const auto sampler = makeMonoSampler();
    sampler->setChromaticMode(true);
    sampler->clearSample(60);
    sampler->loadSample(0, "root.wav");
    sampler->setSampleChokeGroup(0, 1);

    sampler->processMidiNoteOn(3, 127);
    render(*sampler, 256);
    sampler->processMidiNoteOn(7, 127);
    render(*sampler, 1024);

    QVERIFY2(!sampler->isFinished(3), "the pad choked its own earlier note");
    QVERIFY(!sampler->isFinished(7));
}

void SamplerTest::test_copySample_shouldCopyLoopAndChokeGroup()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSampleLoop(60, true);
    sampler.setSampleChokeGroup(60, 3);

    sampler.copySample(60, 62);

    QCOMPARE(sampler.sampleLoop(62), true);
    QCOMPARE(sampler.sampleChokeGroup(62), 3);
}

void SamplerTest::test_copySample_shouldCopyTuningTrimAndEnvelope()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSampleTune(60, 0.75f);
    sampler.setSampleDetune(60, 0.25f);
    sampler.setSampleAttack(60, 0.2f);
    sampler.setSampleDecay(60, 0.3f);
    sampler.setSampleSustain(60, 0.4f);
    sampler.setSampleRelease(60, 0.6f);
    sampler.setSampleReverse(60, true);
    sampler.setSampleEndOffset(60, 0.01);

    sampler.copySample(60, 62);

    QCOMPARE(SamplerDevice::tuneSemitones(sampler.sampleTune(62)), SamplerDevice::tuneSemitones(sampler.sampleTune(60)));
    QCOMPARE(sampler.sampleDetune(62), sampler.sampleDetune(60));
    QCOMPARE(sampler.sampleAttack(62), sampler.sampleAttack(60));
    QCOMPARE(sampler.sampleDecay(62), sampler.sampleDecay(60));
    QCOMPARE(sampler.sampleSustain(62), sampler.sampleSustain(60));
    QCOMPARE(sampler.sampleRelease(62), sampler.sampleRelease(60));
    QCOMPARE(sampler.sampleReverse(62), true);
    QVERIFY(std::abs(sampler.sampleEndOffset(62) - sampler.sampleEndOffset(60)) < 1e-6);
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

void SamplerTest::test_padMidiCc_shouldNotChangeAuthoredValue()
{
    SamplerDevice sampler { Constants::samplerDeviceName().toStdString(), std::make_unique<MockAudioFileReader>() };
    const uint8_t note = SamplerDevice::padStartNote;
    sampler.loadSample(note, "test.wav");
    sampler.setSamplePan(note, 0.25f);

    // Pad 0 pan
    sampler.processMidiCc(SamplerDevice::padPanCcStart, 127, 0);
    QCOMPARE(sampler.samplePan(note), 1.0f);

    sampler.clearAutomation();

    QCOMPARE(sampler.samplePan(note), 0.25f);
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
