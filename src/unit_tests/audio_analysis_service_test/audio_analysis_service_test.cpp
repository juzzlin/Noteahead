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

#include "audio_analysis_service_test.hpp"

#include "../../application/service/analysis_report_formatter.hpp"
#include "../../application/service/audio_analysis_service.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <cmath>
#include <map>
#include <numbers>
#include <random>
#include <vector>

namespace noteahead {

namespace {

constexpr int SampleRate = 44100;
constexpr int Channels = 2;

using Library = std::map<std::string, std::vector<float>>;

//! Hands the analyzer audio held in memory, so a test can measure a mix that was never a file. A
//! path the library does not know fails to open, which is the missing-file case.
class MockAudioFileReader : public AudioFileReader
{
public:
    explicit MockAudioFileReader(const Library & library)
      : m_library { library }
    {
    }

    bool open(const std::string & filePath, Mode, Info & info) override
    {
        const auto it = m_library.find(filePath);
        if (it == m_library.end()) {
            return false;
        }
        m_data = &it->second;
        m_position = 0;
        info.samplerate = SampleRate;
        info.channels = Channels;
        info.frames = static_cast<int64_t>(m_data->size() / Channels);
        m_info = info;
        m_isOpen = true;
        return true;
    }

    void close() override
    {
        m_isOpen = false;
    }

    void setTag(TagType, const std::string &) override
    {
    }

    int64_t readFloat(std::span<float> data) override
    {
        if (!m_data || m_position >= m_data->size()) {
            return 0;
        }
        const auto toRead = std::min(data.size(), m_data->size() - m_position);
        std::copy(m_data->begin() + static_cast<ptrdiff_t>(m_position),
                  m_data->begin() + static_cast<ptrdiff_t>(m_position + toRead), data.begin());
        m_position += toRead;
        return static_cast<int64_t>(toRead / Channels);
    }

    int64_t readDouble(std::span<double>) override
    {
        return 0;
    }

    int64_t readInt(std::span<int32_t>) override
    {
        return 0;
    }

    int64_t writeFloat(std::span<const float>) override
    {
        return 0;
    }

    int64_t writeInt(std::span<const int32_t>) override
    {
        return 0;
    }

    bool seek(int64_t, int) override
    {
        return false;
    }

    bool isOpen() const override
    {
        return m_isOpen;
    }

    Info info() const override
    {
        return m_info;
    }

private:
    const Library & m_library;
    const std::vector<float> * m_data { nullptr };
    size_t m_position { 0 };
    Info m_info {};
    bool m_isOpen { false };
};

//! Noise split into a low and a high half at about 1 kHz, mixed in the proportion @p highWeight
//! asks for: above 1 it is a brighter mix, below it a darker one. Noise rather than a tone because
//! the balance is measured across third-octave bands, and a tone leaves all but one of them empty.
std::vector<float> tiltedNoise(double seconds, double highWeight, float level = 0.2f)
{
    const auto frames = static_cast<size_t>(seconds * SampleRate);
    std::mt19937 generator { 1234 };
    std::uniform_real_distribution<float> distribution { -1.0f, 1.0f };

    std::vector<float> out(frames * Channels, 0.0f);
    float state = 0.0f;
    // A one-pole split: what the filter keeps is the low half, and what it rejects is the high one.
    const float coefficient = 0.9f;
    for (size_t i = 0; i < frames; i++) {
        const float white = distribution(generator);
        state = coefficient * state + (1.0f - coefficient) * white;
        const float low = state;
        const float high = white - state;
        const float sample = level * (low + high * static_cast<float>(highWeight));
        out[i * Channels] = sample;
        out[i * Channels + 1] = sample;
    }
    return out;
}

std::vector<float> tone(double seconds, double frequency, float amplitude)
{
    const auto frames = static_cast<size_t>(seconds * SampleRate);
    std::vector<float> out(frames * Channels, 0.0f);
    for (size_t i = 0; i < frames; i++) {
        const auto sample = amplitude * static_cast<float>(std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(i) / SampleRate));
        out[i * Channels] = sample;
        out[i * Channels + 1] = sample;
    }
    return out;
}

//! Runs one analysis and waits for the service to answer.
void analyzeAndWait(AudioAnalysisService & service, AudioAnalysisService::Side side, const QString & path)
{
    QSignalSpy changed { &service, &AudioAnalysisService::analysisChanged };
    service.analyze(side, path);
    QVERIFY(changed.wait(30000));
}

//! The difference the comparison is about: the top of the band range minus the bottom of it, as
//! seen from the right side against the left.
double highMinusLow(const AudioAnalysisService & service)
{
    const auto & left = service.analysis(AudioAnalysisService::Side::Left).spectrum;
    const auto & right = service.analysis(AudioAnalysisService::Side::Right).spectrum;
    return static_cast<double>(right.highDb - left.highDb);
}

} // namespace

void AudioAnalysisServiceTest::test_analyze_toneFile_shouldReportItsLoudness()
{
    Library library { { "tone.wav", tone(3.0, 1000.0, 0.5f) } };
    AudioAnalysisService service;
    service.setAudioFileReaderFactory([&library] { return std::make_unique<MockAudioFileReader>(library); });

    analyzeAndWait(service, AudioAnalysisService::Side::Left, "tone.wav");

    const auto & analysis = service.analysis(AudioAnalysisService::Side::Left);
    QVERIFY(analysis.isValid);
    QCOMPARE(analysis.sampleRate, static_cast<quint32>(SampleRate));
    QCOMPARE(analysis.channels, Channels);
    // A 1 kHz sine at half scale in both channels: K-weighting lifts it a little over the -9.0 LUFS
    // its power alone would give, and the true peak is the amplitude itself.
    QVERIFY2(analysis.loudness.integratedLoudness > -12.0f && analysis.loudness.integratedLoudness < -4.0f,
             qPrintable(QString { "%1 LUFS" }.arg(analysis.loudness.integratedLoudness)));
    QVERIFY2(std::abs(analysis.loudness.truePeak - -6.0f) < 1.0f, qPrintable(QString { "%1 dBTP" }.arg(analysis.loudness.truePeak)));
    QVERIFY(!service.isAnalyzing());
}

void AudioAnalysisServiceTest::test_analyze_unreadableFile_shouldFail()
{
    Library library;
    AudioAnalysisService service;
    service.setAudioFileReaderFactory([&library] { return std::make_unique<MockAudioFileReader>(library); });

    QSignalSpy failed { &service, &AudioAnalysisService::errorOccurred };
    service.analyze(AudioAnalysisService::Side::Left, "missing.wav");
    QVERIFY(failed.wait(30000));

    QCOMPARE(failed.at(0).at(0).toInt(), static_cast<int>(AudioAnalysisService::Side::Left));
    QVERIFY(!failed.at(0).at(1).toString().isEmpty());
    QVERIFY(!service.analysis(AudioAnalysisService::Side::Left).isValid);
    // The side that failed must not leave the dialog waiting for it forever.
    QVERIFY(!service.isAnalyzing());
}

void AudioAnalysisServiceTest::test_analyze_brighterFile_shouldShowPositiveDifference()
{
    // Left is the reference and right is the brighter mix, so the difference up top is the cut the
    // right one needs to match the left: the whole point of comparing the two.
    Library library {
        { "dark.wav", tiltedNoise(4.0, 0.25) },
        { "bright.wav", tiltedNoise(4.0, 4.0) }
    };
    AudioAnalysisService service;
    service.setAudioFileReaderFactory([&library] { return std::make_unique<MockAudioFileReader>(library); });

    analyzeAndWait(service, AudioAnalysisService::Side::Left, "dark.wav");
    analyzeAndWait(service, AudioAnalysisService::Side::Right, "bright.wav");

    QVERIFY(service.analysis(AudioAnalysisService::Side::Left).spectrum.isValid);
    QVERIFY(service.analysis(AudioAnalysisService::Side::Right).spectrum.isValid);
    QVERIFY2(highMinusLow(service) > 3.0, qPrintable(QString { "highs differ by only %1 dB" }.arg(highMinusLow(service))));
}

void AudioAnalysisServiceTest::test_swap_shouldInvertTheDifference()
{
    Library library {
        { "dark.wav", tiltedNoise(4.0, 0.25) },
        { "bright.wav", tiltedNoise(4.0, 4.0) }
    };
    AudioAnalysisService service;
    service.setAudioFileReaderFactory([&library] { return std::make_unique<MockAudioFileReader>(library); });

    analyzeAndWait(service, AudioAnalysisService::Side::Left, "dark.wav");
    analyzeAndWait(service, AudioAnalysisService::Side::Right, "bright.wav");
    const auto before = highMinusLow(service);

    service.swap();

    QCOMPARE(service.analysis(AudioAnalysisService::Side::Left).filePath, QString { "bright.wav" });
    QVERIFY2(std::abs(highMinusLow(service) + before) < 1.0e-4,
             qPrintable(QString { "%1 against %2 before the swap" }.arg(highMinusLow(service)).arg(before)));
}

void AudioAnalysisServiceTest::test_reportText_noFiles_shouldBeEmpty()
{
    AudioAnalysisService service;
    QVERIFY(service.reportText().isEmpty());
    QVERIFY(!service.saveReport("nowhere.txt"));
}

void AudioAnalysisServiceTest::test_saveReport_shouldWriteTheComparison()
{
    Library library {
        { "dark.wav", tiltedNoise(4.0, 0.25) },
        { "bright.wav", tiltedNoise(4.0, 4.0) }
    };
    AudioAnalysisService service;
    service.setAudioFileReaderFactory([&library] { return std::make_unique<MockAudioFileReader>(library); });

    analyzeAndWait(service, AudioAnalysisService::Side::Left, "dark.wav");
    analyzeAndWait(service, AudioAnalysisService::Side::Right, "bright.wav");

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("comparison.txt");
    QVERIFY(service.saveReport(path));

    QFile file { path };
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const auto written = QString::fromUtf8(file.readAll());
    QVERIFY(written.contains("dark.wav"));
    QVERIFY(written.contains("bright.wav"));
    QVERIFY(written.contains("B - A"));
    // One row per third-octave band, so the comparison can be read as a curve rather than a summary.
    QVERIFY(written.contains("1.0 kHz"));
    QVERIFY(written.contains("100 Hz"));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::AudioAnalysisServiceTest)
