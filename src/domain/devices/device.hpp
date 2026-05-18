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

#ifndef DEVICE_HPP
#define DEVICE_HPP

#include <QObject>

#include "../parameter_container.hpp"

#include <vector>
#include <cstdint>
#include <mutex>
#include <string>

class QXmlStreamReader;
class QXmlStreamWriter;

namespace noteahead {

class Device : public QObject, public ParameterContainer
{
    Q_OBJECT

public:
    Device();
    virtual ~Device() override = default;

    virtual std::string name() const = 0;
    virtual std::string category() const = 0;
    virtual std::string typeId() const = 0;

    size_t id() const;
    void setId(size_t id);

    virtual void processMidiNoteOn(uint8_t note, uint8_t velocity) = 0;
    virtual void processMidiNoteOff(uint8_t note) = 0;
    virtual void processMidiCc(uint8_t controller, uint8_t value, uint8_t channel) = 0;
    virtual void processMidiPitchBend(uint16_t /*value*/, uint8_t /*channel*/) {}
    virtual void processMidiProgramChange(uint8_t, uint8_t) {}
    virtual void processMidiAllNotesOff() = 0;

    virtual void processAudio(float * output, uint32_t frameCount, uint32_t sampleRate) = 0;
    virtual bool hasActiveAudio() const { return true; }

    virtual void setBpm(float) {}

    virtual void reset() override;
    virtual void resetAudio();

    uint32_t sampleRate() const;

    virtual void serializeToXml(QXmlStreamWriter & writer) const;
    virtual void deserializeFromXml(QXmlStreamReader & reader);

    float volume() const;
    virtual void setVolume(float volume);

    float gain() const;
    virtual void setGain(float gain);

    float pan() const;
    virtual void setPan(float pan);

    float reverbSend(size_t index) const;
    virtual void setReverbSend(size_t index, float send);
    size_t reverbSendCount() const;

signals:
    void dataChanged();

protected:
    void serializeAttributesToXml(QXmlStreamWriter & writer) const;
    void deserializeAttributesFromXml(QXmlStreamReader & reader);

    virtual void syncParameters();

    bool updateVolumeParameter(float volume, bool updateManual);
    bool updateGainParameter(float gain, bool updateManual);
    bool updatePanParameter(float pan, bool updateManual);
    bool updateReverbSendParameter(size_t index, float send, bool updateManual);

    void setSampleRate(uint32_t sampleRate);
    std::recursive_mutex & mutex() const;

    float volumeInternal() const;
    float gainInternal() const;
    float panInternal() const;
    float reverbSendInternal(size_t index) const;
    float linearGainInternal() const;
    float manualVolumeInternal() const;
    float manualGainInternal() const;
    float manualPanInternal() const;
    float manualReverbSendInternal(size_t index) const;
    void setManualVolume(float volume);
    void setManualGain(float gain);
    void setManualPan(float pan);
    void setManualReverbSend(size_t index, float send);

private:
    size_t m_id { 0 };
    uint32_t m_sampleRate { 0 };

    float m_volume { 1.0f };
    float m_gain { 0.5f };
    float m_pan { 0.5f };
    std::vector<float> m_reverbSends;
    float m_linearGain { 1.0f };

    // Manual settings for CC reset
    float m_manualVolume { 1.0f };
    float m_manualGain { 0.5f };
    float m_manualPan { 0.5f };
    std::vector<float> m_manualReverbSends;

    mutable std::recursive_mutex m_mutex;
};

} // namespace noteahead

#endif // DEVICE_HPP
