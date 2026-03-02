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

#include "audio_file_io_test.hpp"
#include "../../infra/audio/audio_file_recorder.hpp"
#include "../../infra/audio/audio_file_streamer.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"

#include <QtTest>
#include <mutex>
#include <condition_variable>

namespace noteahead {

class MockAudioFileIO : public AudioFileReader
{
public:
    bool open(const std::string &, Mode mode, Info & info) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_mode = mode;
        if (mode == Mode::Read) {
            info = m_info;
        } else {
            m_info = info;
            m_data.clear();
        }
        m_isOpen = true;
        m_pos = 0;
        m_cv.notify_all();
        return true;
    }

    void close() override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isOpen = false;
        m_cv.notify_all();
    }

    int64_t readFloat(std::span<float>) override { return 0; }
    int64_t readDouble(std::span<double>) override { return 0; }

    int64_t readInt(std::span<int32_t> data) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_isOpen || m_mode != Mode::Read) return 0;
        const size_t toRead = std::min(data.size(), m_data.size() - m_pos);
        if (toRead == 0) return 0;
        std::copy(m_data.begin() + m_pos, m_data.begin() + m_pos + toRead, data.begin());
        m_pos += toRead;
        m_readCount++;
        m_cv.notify_all();
        return static_cast<int64_t>(toRead / static_cast<size_t>(m_info.channels));
    }

    int64_t writeFloat(std::span<const float>) override { return 0; }

    int64_t writeInt(std::span<const int32_t> data) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_isOpen || m_mode != Mode::Write) return 0;
        m_data.insert(m_data.end(), data.begin(), data.end());
        m_info.frames += static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
        m_writeCount++;
        m_cv.notify_all();
        return static_cast<int64_t>(data.size() / static_cast<size_t>(m_info.channels));
    }

    bool seek(int64_t frames, int) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pos = static_cast<size_t>(frames * m_info.channels);
        m_seekCount++;
        m_cv.notify_all();
        return true;
    }

    bool isOpen() const override { return m_isOpen; }
    Info info() const override { return m_info; }

    // Helper for testing
    void setData(const std::vector<int32_t> & data, const Info & info)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data = data;
        m_info = info;
    }

    const std::vector<int32_t> & data() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_data;
    }

    bool waitForWriteSize(size_t expectedSize, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(lock, timeout, [this, expectedSize] { return m_data.size() >= expectedSize; });
    }

    bool waitForReadCount(int expectedCount, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(lock, timeout, [this, expectedCount] { return m_readCount >= expectedCount; });
    }

    bool waitForSeekCount(int expectedCount, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(lock, timeout, [this, expectedCount] { return m_seekCount >= expectedCount; });
    }

private:
    std::vector<int32_t> m_data;
    Info m_info;
    bool m_isOpen = false;
    Mode m_mode = Mode::Read;
    size_t m_pos = 0;
    int m_readCount = 0;
    int m_writeCount = 0;
    int m_seekCount = 0;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
};

void AudioFileIoTest::testRecordingAndStreaming()
{
    const uint32_t sampleRate = 44100;
    const uint32_t channels = 2;
    const size_t bufferSize = 4096;

    std::vector<int32_t> recordedData;

    // 1. Record some data using Mock
    auto mockWriter = std::make_unique<MockAudioFileIO>();
    auto mockWriterPtr = mockWriter.get();
    {
        AudioFileRecorder recorder(std::move(mockWriter));
        recorder.start("dummy", sampleRate, channels, bufferSize);

        std::vector<int32_t> testData(1000);
        for (size_t i = 0; i < testData.size(); ++i) {
            testData[i] = static_cast<int32_t>(i);
        }

        QVERIFY(recorder.push(testData.data(), testData.size()));

        // Wait for background thread to process the ring buffer
        QVERIFY(mockWriterPtr->waitForWriteSize(1000, std::chrono::milliseconds(1000)));

        recorder.stop();
        recordedData = mockWriterPtr->data();
    }

    QCOMPARE(recordedData.size(), 1000u);

    // 2. Stream it back and verify using Mock
    auto mockReader = std::make_unique<MockAudioFileIO>();
    auto mockReaderPtr = mockReader.get();
    AudioFileReader::Info info;
    info.samplerate = sampleRate;
    info.channels = channels;
    info.frames = 500;
    mockReader->setData(recordedData, info);

    {
        AudioFileStreamer streamer(std::move(mockReader));
        streamer.start("dummy", bufferSize);

        QCOMPARE(streamer.sampleRate(), static_cast<int>(sampleRate));
        QCOMPARE(streamer.channels(), static_cast<int>(channels));

        // Wait for disk read thread to fill ring buffer (at least one read operation)
        QVERIFY(mockReaderPtr->waitForReadCount(1, std::chrono::milliseconds(1000)));

        std::vector<int32_t> readData(1000);
        size_t totalRead = 0;
        int attempts = 0;
        while (totalRead < 1000 && attempts < 100) {
            size_t read = streamer.pop(readData.data() + totalRead, readData.size() - totalRead);
            if (read == 0) {
                // If the ring buffer was empty, we might need to wait for another read from mock
                mockReaderPtr->waitForReadCount(attempts + 2, std::chrono::milliseconds(100));
                attempts++;
                continue;
            }
            totalRead += read;
        }

        QCOMPARE(totalRead, 1000u);
        for (size_t i = 0; i < totalRead; ++i) {
            QCOMPARE(readData[i], static_cast<int32_t>(i));
        }
        QVERIFY(streamer.isFinished());
    }
}

void AudioFileIoTest::testPosition()
{
    const uint32_t sampleRate = 44100;
    const uint32_t channels = 1;
    const size_t bufferSize = 4096;

    auto mockReader = std::make_unique<MockAudioFileIO>();
    auto mockReaderPtr = mockReader.get();
    AudioFileReader::Info info;
    info.samplerate = sampleRate;
    info.channels = channels;
    info.frames = 1000;
    std::vector<int32_t> data(1000, 0);
    mockReader->setData(data, info);

    {
        AudioFileStreamer streamer(std::move(mockReader));
        streamer.start("dummy", bufferSize);
        
        // Wait for initial fill
        QVERIFY(mockReaderPtr->waitForReadCount(1, std::chrono::milliseconds(1000)));

        streamer.setPosition(0.5);
        QCOMPARE(streamer.position(), 0.5);
        QVERIFY(mockReaderPtr->waitForSeekCount(1, std::chrono::milliseconds(1000)));

        // Wait for background thread to fill ring buffer from new position
        QVERIFY(mockReaderPtr->waitForReadCount(2, std::chrono::milliseconds(1000)));

        std::vector<int32_t> dummy(100);
        size_t read = streamer.pop(dummy.data(), dummy.size());
        QCOMPARE(read, 100u);
        QVERIFY(streamer.position() > 0.5);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AudioFileIoTest)
