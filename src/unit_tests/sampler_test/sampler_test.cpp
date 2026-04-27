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

#include "../../application/position.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"

#include <vector>

#include <QTest>

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

void SamplerTest::test_initialState()
{
    const SamplerDevice sampler;
    QCOMPARE(sampler.name(), Constants::samplerDeviceName().toStdString());
    for (int i = 0; i < 128; ++i) {
        QVERIFY(!sampler.sample(static_cast<uint8_t>(i)));
    }
    QCOMPARE(sampler.channelMode(), false);
}

void SamplerTest::test_loadAndClearSample()
{
    SamplerDevice sampler;
    sampler.loadSample(60, "non_existent.wav");
    QVERIFY(!sampler.sample(60));

    sampler.clearSample(60);
    QVERIFY(!sampler.sample(60));
}

void SamplerTest::test_midiNoteOn()
{
    SamplerDevice sampler;
    sampler.processMidiNoteOn(60, 100);

    std::vector<float> buffer(256, 0.0f);
    sampler.processAudio(buffer.data(), 128, 44100);

    for (float val : buffer) {
        QCOMPARE(val, 0.0f);
    }
}

void SamplerTest::test_midiAllNotesOff()
{
    SamplerDevice sampler;
    sampler.processMidiAllNotesOff();
}

void SamplerTest::test_pan()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
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
    
    std::vector<float> buffer(4, 0.0f); // 2 frames
    sampler.processMidiNoteOn(60, 127);
    sampler.processAudio(buffer.data(), 2, 44100);
    
    QCOMPARE(buffer[0], 1.0f); // Left
    QCOMPARE(buffer[1], 0.0f); // Right
    
    // Test full right (127)
    buffer.assign(4, 0.0f);
    sampler.processMidiCc(10, 127, 0); // MIDI Pan full right, channel 0
    sampler.processMidiNoteOn(60, 127);
    sampler.processAudio(buffer.data(), 2, 44100);
    
    QCOMPARE(buffer[0], 0.0f); // Left
    QCOMPARE(buffer[1], 1.0f); // Right
}

void SamplerTest::test_volume()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
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

void SamplerTest::test_cutoff()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    
    // LPF: Default cutoff is 1.0 (fully open)
    QCOMPARE(sampler.sampleCutoff(60), 1.0f);
    sampler.setSampleCutoff(60, 0.5f);
    QCOMPARE(sampler.sampleCutoff(60), 0.5f);
    
    // Instant manual adjustment (no automation)
    std::vector<float> buffer(4, 0.0f);
    sampler.processMidiNoteOn(60, 127);
    sampler.processAudio(buffer.data(), 2, 44100);
    // Cutoff 0.5 means filtering is active. Mock returns 1.0 (DC). 
    // We just verify it doesn't crash and logic executes.
    QVERIFY(buffer[0] > 0.0f);

    // HPF: Default cutoff is 0.0 (fully open)
    QCOMPARE(sampler.sampleHpfCutoff(60), 0.0f);
    sampler.setSampleHpfCutoff(60, 0.5f);
    QCOMPARE(sampler.sampleHpfCutoff(60), 0.5f);
    
    // Instant manual adjustment (no automation)
    buffer.assign(4, 0.0f);
    sampler.processMidiNoteOn(60, 127);
    sampler.processAudio(buffer.data(), 2, 44100);
    QVERIFY(buffer[0] > 0.0f);
}

void SamplerTest::test_channelMode()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
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

void SamplerTest::test_processAudio()
{
    auto mockReader = std::make_unique<MockAudioFileReader>();
    mockReader->setForceChannels(1); // Mono
    SamplerDevice sampler { std::move(mockReader) };
    sampler.loadSample(60, "test.wav");
    sampler.processMidiNoteOn(60, 127);
    
    std::vector<float> buffer(4, 0.0f);
    sampler.processAudio(buffer.data(), 2, 44100);

    // Default pan is center, mono sample should be on both channels
    QCOMPARE(buffer[0], 1.0f);
    QCOMPARE(buffer[1], 1.0f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SamplerTest)
