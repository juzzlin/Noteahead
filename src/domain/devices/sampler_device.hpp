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
#include "effect.hpp"
#include "high_pass_filter_effect.hpp"
#include "low_pass_filter_effect.hpp"
#include "panning_effect.hpp"
#include "volume_effect.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"

#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class SamplerDevice : public Device
{
public:
    using AudioFileReaderU = std::unique_ptr<AudioFileReader>;

    explicit SamplerDevice(AudioFileReaderU audioFileReader = nullptr);
    ~SamplerDevice() override;

    std::string name() const override;

    void processMidiNoteOn(uint8_t note, uint8_t velocity) override;
    void processMidiNoteOff(uint8_t note) override;
    void processMidiCc(uint8_t controller, uint8_t value, uint8_t channel) override;
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
        float volume = 1.0f; // 0.0 to 1.0
        float cutoff = 1.0f; // LPF cutoff: 0.0 to 1.0
        float hpfCutoff = 0.0f; // HPF cutoff: 0.0 to 1.0
        float manualPan = 0.5f;
        float manualVolume = 1.0f;
        float manualCutoff = 1.0f;
        float manualHpfCutoff = 0.0f;
    };

    void loadSample(uint8_t note, const std::string & filePath);
    void clearSample(uint8_t note);
    const Sample * sample(uint8_t note) const;
    std::string absoluteFilePath(uint8_t note) const;

    float samplePan(uint8_t note) const;
    void setSamplePan(uint8_t note, float pan);

    float sampleVolume(uint8_t note) const;
    void setSampleVolume(uint8_t note, float volume);

    float sampleCutoff(uint8_t note) const;
    void setSampleCutoff(uint8_t note, float cutoff);

    float sampleHpfCutoff(uint8_t note) const;
    void setSampleHpfCutoff(uint8_t note, float cutoff);

    bool channelMode() const;
    void setChannelMode(bool enabled);

    double playbackPosition(uint8_t note) const;
    bool isFinished(uint8_t note) const;

    void setProjectPath(const std::string & projectPath);

    void saveState();
    void restoreState();

private:
    struct Voice;
    void updateVoiceEffects(Voice & voice);

    struct Voice
    {
        Voice();

        uint8_t note = 0;
        Sample * sample = nullptr;
        double position = 0.0;
        float velocity = 1.0f;
        float pan = 0.5f;
        float volume = 1.0f;
        float cutoff = 1.0f;
        float hpfCutoff = 0.0f;

        std::shared_ptr<LowPassFilterEffect> lpf;
        std::shared_ptr<HighPassFilterEffect> hpf;
        std::shared_ptr<VolumeEffect> volumeEffect;
        std::shared_ptr<PanningEffect> panningEffect;
        std::vector<std::shared_ptr<Effect>> effects;

        bool active = false;
        bool releasing = false;
        float releaseGain = 1.0f;
    };

    std::array<std::unique_ptr<Sample>, 128> m_samples;
    std::array<std::unique_ptr<Sample>, 128> m_savedSamples;
    std::vector<Voice> m_voices;
    mutable std::mutex m_mutex;

    float m_globalPan = 0.5f;
    float m_globalVolume = 1.0f;
    float m_globalCutoff = 1.0f;
    float m_globalHpfCutoff = 0.0f;
    float m_manualGlobalPan = 0.5f;
    float m_manualGlobalVolume = 1.0f;
    float m_manualGlobalCutoff = 1.0f;
    float m_manualGlobalHpfCutoff = 0.0f;
    bool m_channelMode = false;
    std::string m_projectPath;
    AudioFileReaderU m_audioFileReader;
    const size_t m_maxVoices = 32;
};

} // namespace noteahead

#endif // SAMPLER_DEVICE_HPP
