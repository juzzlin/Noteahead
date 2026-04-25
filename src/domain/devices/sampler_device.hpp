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

#ifndef SAMPLER_DEVICE_HPP
#define SAMPLER_DEVICE_HPP

#include "device.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"

#include <QObject>

#include <array>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class SamplerDevice : public QObject, public Device
{
    Q_OBJECT

public:
    using AudioFileReaderU = std::unique_ptr<AudioFileReader>;

    explicit SamplerDevice(AudioFileReaderU audioFileReader = nullptr);
    ~SamplerDevice() override;

    std::string name() const override;

    void processMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processMidiNoteOff(uint8_t note) override;
    void processMidiCc(uint8_t controller, uint8_t value) override;
    void processMidiAllNotesOff() override;

    void processAudio(float * output, uint32_t nFrames, uint32_t sampleRate) override;

    void serializeToXml(QXmlStreamWriter & writer) const;
    void deserializeFromXml(QXmlStreamReader & reader);

    struct Sample
    {
        std::string filePath;
        std::shared_ptr<const std::vector<float>> data;
        int channels = 0;
        int sampleRate = 0;
        float pan = 0.5f; // 0.0 (left) to 1.0 (right)
    };

    void loadSample(uint8_t note, const std::string & filePath);
    void clearSample(uint8_t note);
    const Sample * sample(uint8_t note) const;

    void setProjectPath(const std::string & projectPath);

    void saveState();
    void restoreState();

signals:
    void samplesChanged();

private:
    struct Voice
    {
        uint8_t note = 0;
        const Sample * sample = nullptr;
        size_t position = 0;
        float velocity = 0.0f;
        bool active = false;
    };

    std::array<std::unique_ptr<Sample>, 128> m_samples;
    std::array<std::unique_ptr<Sample>, 128> m_savedSamples;
    std::vector<Voice> m_voices;
    mutable std::mutex m_mutex;

    float m_globalPan = 0.5f;
    std::string m_projectPath;
    AudioFileReaderU m_audioFileReader;
    static constexpr size_t MaxVoices = 32;
};

} // namespace noteahead

#endif // SAMPLER_DEVICE_HPP
