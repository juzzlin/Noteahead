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

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace noteahead {

class MockAudioFileReader : public AudioFileReader
{
public:
    bool open(const std::string &, Mode, Info & info) override { info = this->info(); return true; }
    void close() override { }
    int64_t readFloat(std::span<float> data) override { std::fill(data.begin(), data.end(), 1.0f); return data.size(); }
    int64_t readDouble(std::span<double> data) override { return data.size(); }
    int64_t readInt(std::span<int32_t> data) override { return data.size(); }
    int64_t writeFloat(std::span<const float> data) override { return data.size(); }
    int64_t writeInt(std::span<const int32_t> data) override { return data.size(); }
    bool seek(int64_t, int) override { return true; }
    bool isOpen() const override { return true; }
    Info info() const override { return { 1024, 44100, m_channels, 0 }; }
    void setForceChannels(int channels) { m_channels = channels; }
private:
    int m_channels = 2;
};

void SamplerTest::initTestCase() { }

void SamplerTest::test_initialState_shouldBeCorrect()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
    QCOMPARE(sampler.name(), Constants::samplerDeviceName().toStdString());
}

void SamplerTest::test_loadAndClearSample_shouldUpdateModel()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    QVERIFY(sampler.sample(60) != nullptr);
    sampler.clearSample(60);
    QVERIFY(sampler.sample(60) == nullptr);
}

void SamplerTest::test_midiNoteOn_shouldPlaySample()
{
    auto mockReader = std::make_unique<MockAudioFileReader>();
    mockReader->setForceChannels(1);
    SamplerDevice sampler { std::move(mockReader) };
    sampler.loadSample(60, "test.wav");
    sampler.processMidiNoteOn(60, 127);
    std::vector<float> buffer(4, 0.0f);
    sampler.processAudio(buffer.data(), 2, 44100);
    QVERIFY(buffer[0] > 0.0f);
}

void SamplerTest::test_midiAllNotesOff_shouldStopAllVoices()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.processMidiNoteOn(60, 127);
    sampler.processMidiAllNotesOff();
    std::vector<float> buffer(4, 0.0f);
    sampler.processAudio(buffer.data(), 2, 44100);
    QCOMPARE(buffer[0], 0.0f);
}

void SamplerTest::test_pan_shouldAdjustPanning()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSamplePan(60, 0.75f);
    QVERIFY(qFuzzyCompare(sampler.samplePan(60), 0.75f));
}

void SamplerTest::test_volume_shouldAdjustVolume()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSampleVolume(60, 0.5f);
    QCOMPARE(sampler.sampleVolume(60), 0.5f);
}

void SamplerTest::test_cutoff_shouldAdjustCutoff()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSampleCutoff(60, 0.5f);
    QCOMPARE(sampler.sampleCutoff(60), 0.5f);
    sampler.setSampleHpfCutoff(60, 0.5f);
    QCOMPARE(sampler.sampleHpfCutoff(60), 0.5f);
}

void SamplerTest::test_channelMode_shouldToggleCorrectMode()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
    sampler.setChannelMode(true);
    QCOMPARE(sampler.channelMode(), true);
}

void SamplerTest::test_midiCcReset_shouldResetInternalValues()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSamplePan(60, 0.75f);
    sampler.setChannelMode(true);
    sampler.processMidiCc(10, 0, 24);
    sampler.processMidiCc(121, 127, 24);
    QVERIFY(qFuzzyCompare(sampler.samplePan(60), 0.75f));
}

void SamplerTest::test_startOffset_shouldShiftPlaybackStart()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setSampleStartOffset(60, 0.01);
    QVERIFY(std::abs(sampler.sampleStartOffset(60) - 0.01) < 0.0001);
}

void SamplerTest::test_reset_shouldResetParametersAndPads()
{
    SamplerDevice sampler { std::make_unique<MockAudioFileReader>() };
    sampler.loadSample(60, "test.wav");
    sampler.setChannelMode(true);
    sampler.reset();
    QCOMPARE(sampler.channelMode(), false);
    QVERIFY(sampler.sample(60) == nullptr);
}

void SamplerTest::test_processAudio_shouldProduceOutput()
{
    auto mockReader = std::make_unique<MockAudioFileReader>();
    mockReader->setForceChannels(1);
    SamplerDevice sampler { std::move(mockReader) };
    sampler.loadSample(60, "test.wav");
    sampler.processMidiNoteOn(60, 127);
    std::vector<float> buffer(4, 0.0f);
    sampler.processAudio(buffer.data(), 2, 44100);
    QCOMPARE(buffer[0], 1.0f);
    QCOMPARE(buffer[1], 1.0f);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::SamplerTest)
