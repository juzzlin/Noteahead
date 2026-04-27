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
#include "../../domain/devices/sampler_device.hpp"
#include "../../common/constants.hpp"
#include "../../application/position.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"

#include <vector>
#include <QTest>

Q_DECLARE_METATYPE(noteahead::Position)

namespace noteahead {

class MockAudioFileReader : public AudioFileReader
{
public:
    bool open(const std::string &, Mode, Info & info) override
    {
        info.frames = 1000;
        info.samplerate = 44100;
        info.channels = m_forceChannels > 0 ? m_forceChannels : 2;
        m_info = info;
        m_isOpen = true;
        return true;
    }
    void close() override { m_isOpen = false; }
    int64_t readFloat(std::span<float> data) override
    {
        std::fill(data.begin(), data.end(), 1.0f); // Return 1.0 for all samples
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }
    int64_t readDouble(std::span<double> data) override
    {
        std::fill(data.begin(), data.end(), 1.0);
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }
    int64_t readInt(std::span<int32_t> data) override
    {
        std::fill(data.begin(), data.end(), 1);
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }
    int64_t writeFloat(std::span<const float> data) override { return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels)); }
    int64_t writeInt(std::span<const int32_t> data) override { return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels)); }
    bool seek(int64_t, int) override { return true; }
    bool isOpen() const override { return m_isOpen; }
    Info info() const override { return m_info; }

    void setForceChannels(int channels) { m_forceChannels = channels; }

private:
    bool m_isOpen = false;
    Info m_info;
    int m_forceChannels = 0;
};

void SamplerTest::initTestCase()
{
    qRegisterMetaType<noteahead::Position>("Position");
}

void SamplerTest::testInitialState()
{
    const SamplerDevice sampler;
    QCOMPARE(sampler.name(), Constants::samplerDeviceName().toStdString());
    for (int i = 0; i < 128; ++i) {
        QVERIFY(sampler.sample(static_cast<uint8_t>(i)) == nullptr);
    }
    QCOMPARE(sampler.channelMode(), false);
}

void SamplerTest::testLoadAndClearSample()
{
    SamplerDevice sampler;
    sampler.loadSample(60, "non_existent.wav");
    QVERIFY(sampler.sample(60) == nullptr);

    sampler.clearSample(60);
    QVERIFY(sampler.sample(60) == nullptr);
}

void SamplerTest::testMidiNoteOn()
{
    SamplerDevice sampler;
    sampler.processMidiNoteOn(60, 100);

    std::vector<float> buffer(256, 0.0f);
    sampler.processAudio(buffer.data(), 128, 44100);

    for (float val : buffer) {
        QCOMPARE(val, 0.0f);
    }
}

void SamplerTest::testMidiAllNotesOff()
{
    SamplerDevice sampler;
    sampler.processMidiAllNotesOff();
}

void SamplerTest::testPan()
{
    SamplerDevice sampler(std::make_unique<MockAudioFileReader>());
    sampler.loadSample(60, "test.wav");
    
    // Default pan is center (0.5)
    QCOMPARE(sampler.samplePan(60), 0.5f);
    
    // Set base pan
    sampler.setSamplePan(60, 0.25f);
    QCOMPARE(sampler.samplePan(60), 0.25f);
    
    // MIDI CC 10 (Pan) interaction - verification via processAudio
    // Test full left (0)
    sampler.setSamplePan(60, 0.5f); // Center
    sampler.processMidiCc(10, 0, 0); // MIDI Pan full left, channel 0
    // combinedPan = (0.5*2-1) + (0*2-1) = 0 - 1 = -1 -> clamped to -1 -> mapped to 0 (full left)
    
    std::vector<float> buffer(4, 0.0f); // 2 frames
    sampler.processMidiNoteOn(60, 127);
    sampler.processAudio(buffer.data(), 2, 44100);
    
    QCOMPARE(buffer[0], 1.0f); // Left
    QCOMPARE(buffer[1], 0.0f); // Right
    QCOMPARE(buffer[2], 1.0f); // Left
    QCOMPARE(buffer[3], 0.0f); // Right
    
    // Test full right (127)
    buffer.assign(4, 0.0f);
    sampler.processMidiCc(10, 127, 0); // MIDI Pan full right, channel 0
    sampler.processMidiNoteOn(60, 127);
    sampler.processAudio(buffer.data(), 2, 44100);
    
    QCOMPARE(buffer[0], 0.0f); // Left
    QCOMPARE(buffer[1], 1.0f); // Right
    
    // Test additive logic: Base pan 25% L (-0.5), MIDI Pan 25% R (+0.5) -> Result Center (approx 0.0)
    buffer.assign(4, 0.0f);
    sampler.setSamplePan(60, 0.25f); // sPan = 0.25*2 - 1 = -0.5
    sampler.processMidiCc(10, 96, 0); // 96/127 = 0.7559. mPan = 0.7559*2 - 1 = 0.5118.
    // combinedPan = -0.5 + 0.5118 = 0.0118.
    // gainL = min(1.0, 2.0 - (0.0118+1)) = min(1.0, 2.0 - 1.0118) = 0.9882.
    // gainR = min(1.0, 1.0118) = 1.0.
    sampler.processMidiNoteOn(60, 127);
    sampler.processAudio(buffer.data(), 2, 44100);
    
    QVERIFY(qAbs(buffer[0] - 0.988189f) < 0.0001f);
    QCOMPARE(buffer[1], 1.0f);
    
    // Test clamping: Base pan 100% R (+1.0), MIDI Pan 100% R (+1.0) -> Result 100% R (+1.0)
    buffer.assign(4, 0.0f);
    sampler.setSamplePan(60, 1.0f);
    sampler.processMidiCc(10, 127, 0);
    sampler.processMidiNoteOn(60, 127);
    sampler.processAudio(buffer.data(), 2, 44100);
    QCOMPARE(buffer[0], 0.0f);
    QCOMPARE(buffer[1], 1.0f);
}

void SamplerTest::testVolume()
{
    SamplerDevice sampler(std::make_unique<MockAudioFileReader>());
    sampler.loadSample(60, "test.wav");
    
    // Default volume is 1.0
    QCOMPARE(sampler.sampleVolume(60), 1.0f);
    
    // Set base volume
    sampler.setSampleVolume(60, 0.5f);
    QCOMPARE(sampler.sampleVolume(60), 0.5f);
    
    // MIDI CC 7 (Volume) interaction
    sampler.setSampleVolume(60, 1.0f);
    sampler.processMidiCc(7, 64, 0); // MIDI Volume approx 0.5 (64/127 = 0.5039)
    
    std::vector<float> buffer(4, 0.0f);
    sampler.processMidiNoteOn(60, 127);
    sampler.processAudio(buffer.data(), 2, 44100);
    
    const float expected = 1.0f * (64.0f / 127.0f);
    QVERIFY(qAbs(buffer[0] - expected) < 0.0001f);
    QVERIFY(qAbs(buffer[1] - expected) < 0.0001f);
}

void SamplerTest::testCutoff()
{
    SamplerDevice sampler(std::make_unique<MockAudioFileReader>());
    sampler.loadSample(60, "test.wav");
    
    // Default cutoff is 1.0 (fully open)
    QCOMPARE(sampler.sampleCutoff(60), 1.0f);
    
    // Set base cutoff
    sampler.setSampleCutoff(60, 0.5f);
    QCOMPARE(sampler.sampleCutoff(60), 0.5f);
    
    // MIDI CC 74 (Cutoff) interaction
    sampler.setSampleCutoff(60, 1.0f);
    sampler.processMidiCc(74, 64, 0); // MIDI Cutoff approx 0.5
    
    std::vector<float> buffer(4, 0.0f);
    sampler.processMidiNoteOn(60, 127);
    sampler.processAudio(buffer.data(), 2, 44100);
    
    // With cutoff at ~0.5, signal should be filtered. 
    // Since MockAudioFileReader returns 1.0f (DC), the filter will eventually 
    // settle to 1.0f if it's purely low-pass.
    // We just verify it's not zero and the logic executes.
    QVERIFY(buffer[0] > 0.0f);
    QVERIFY(buffer[1] > 0.0f);
}

void SamplerTest::testChannelMode()
{
    SamplerDevice sampler(std::make_unique<MockAudioFileReader>());
    sampler.loadSample(36, "pad1.wav"); // Pad 1
    sampler.loadSample(37, "pad2.wav"); // Pad 2
    
    sampler.setChannelMode(true);
    
    // CC on channel 0 (Pad 1 / Note 36)
    sampler.processMidiCc(7, 64, 0); // Vol ~0.5
    QCOMPARE(sampler.sampleVolume(36), 64.0f / 127.0f);
    QCOMPARE(sampler.sampleVolume(37), 1.0f); // Unaffected
    
    // CC on channel 1 (Pad 2 / Note 37)
    sampler.processMidiCc(10, 0, 1); // Pan full left
    QCOMPARE(sampler.samplePan(37), 0.0f);
    QCOMPARE(sampler.samplePan(36), 0.5f); // Unaffected
}

void SamplerTest::testProcessAudio()
{
    auto mockReader = std::make_unique<MockAudioFileReader>();
    mockReader->setForceChannels(1); // Mono
    SamplerDevice sampler(std::move(mockReader));
    sampler.loadSample(60, "test.wav");
    sampler.processMidiNoteOn(60, 127);
    
    std::vector<float> buffer(4, 0.0f);
    sampler.processAudio(buffer.data(), 2, 44100);

    // Default pan is center, mono sample should be on both channels
    QCOMPARE(buffer[0], 1.0f);
    QCOMPARE(buffer[1], 1.0f);
    QCOMPARE(buffer[2], 1.0f);
    QCOMPARE(buffer[3], 1.0f);
}

} // namespace noteahead

QTEST_MAIN(noteahead::SamplerTest)
