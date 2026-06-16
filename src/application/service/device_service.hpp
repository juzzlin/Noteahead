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

#ifndef DEVICE_SERVICE_HPP
#define DEVICE_SERVICE_HPP

#include "../../domain/devices/device.hpp"
#include "../../domain/devices/synth_presets.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../infra/audio/backend/audio_file_reader.hpp"

#include <QObject>
#include <QStringList>

#include <functional>
#include <memory>
#include <string>

namespace noteahead {

class AudioEngine;
class AudioFileReader;
class DataService;
class ProjectReader;
class ProjectWriter;
class SynthDevice;

class DeviceService : public QObject
{
    Q_OBJECT

public:
    using AudioEngineS = std::shared_ptr<AudioEngine>;
    using DataServiceS = std::shared_ptr<DataService>;
    explicit DeviceService(AudioEngineS audioEngine, DataServiceS dataService, QObject * parent = nullptr);
    ~DeviceService() override;

    using DeviceS = std::shared_ptr<Device>;
    void setDevice(size_t slotIndex, DeviceS device);
    void clearDevice(size_t slotIndex);
    virtual DeviceS device(size_t slotIndex) const;
    virtual DeviceS device(const std::string & name) const;

    Q_INVOKABLE virtual bool isInternalDevice(const QString & portName) const;
    void processMidiNoteOn(const QString & portName, uint8_t note, uint8_t velocity);
    void processMidiNoteOff(const QString & portName, uint8_t note);
    void processMidiCc(const QString & portName, uint8_t controller, uint8_t value, uint8_t channel);
    void processMidiPitchBend(const QString & portName, uint16_t value, uint8_t channel);
    void processMidiProgramChange(const QString & portName, uint8_t program, uint8_t channel);
    void processMidiAllNotesOff(const QString & portName);
    void processMidiAllNotesOff();

    using InternalDeviceNames = std::vector<std::string>;
    virtual InternalDeviceNames internalDeviceNames() const;

    Q_INVOKABLE virtual QStringList internalDeviceNamesQt() const;

    Q_INVOKABLE virtual QStringList categories() const;
    Q_INVOKABLE virtual QStringList devicesByCategory(const QString & category) const;

    void setSynthUserPresets(const UserPresets & presets);
    UserPresets synthUserPresets() const;
    void saveSynthUserPreset(int index, const SynthPreset & preset);

    void setProjectPath(const std::string & projectPath);

    void serializeToXml(ProjectWriter & writer) const;
    void deserializeFromXml(ProjectReader & reader);

    using SamplerAudioFileReaderFactory = std::function<std::unique_ptr<AudioFileReader>()>;
    void setSamplerAudioFileReaderFactory(SamplerAudioFileReaderFactory factory);

    Q_INVOKABLE bool exportDeviceSettings(int slotIndex, const QString & filePath) const;
    bool exportDeviceSettings(int slotIndex, ProjectWriter & writer) const;

    Q_INVOKABLE bool importDeviceSettings(int slotIndex, const QString & filePath);
    bool importDeviceSettings(int slotIndex, ProjectReader & reader);

    struct DeviceTypeInfo
    {
        QString typeId;
        QString typeName;
    };

    DeviceTypeInfo peekDeviceTypeInfo(const QString & filePath) const;
    DeviceTypeInfo peekDeviceTypeInfo(ProjectReader & reader) const;

    std::map<QString, QString> getFilesToEmbed() const;

    void reset();

    EffectRack & sendEffectRack();
    EffectRack & insertEffectRack();

signals:
    void dataChanged();
    void synthUserPresetsChanged(const UserPresets & presets);

private:
    DeviceService::DeviceS getDevice(std::string name, std::string typeId);

    std::shared_ptr<SynthDevice> findFirstSynthDevice() const;

    void serializeDevices(ProjectWriter & writer) const;
    void serializeMasterEffects(ProjectWriter & writer) const;
    void serializeSendEffects(ProjectWriter & writer) const;
    void serializeReverbSends(ProjectWriter & writer) const;
    void serializeUserPresets(ProjectWriter & writer) const;
    void serializePreset(ProjectWriter & writer, int index, const SynthPreset & preset, const std::shared_ptr<SynthDevice> & synth) const;
    void serializePresetParameter(ProjectWriter & writer, const std::string & paramName, float value, const std::shared_ptr<SynthDevice> & synth) const;

    void deserializeDevice(ProjectReader & reader);
    void deserializeMasterEffects(ProjectReader & reader);
    void deserializeSendEffects(ProjectReader & reader);
    void deserializeEffectSend(ProjectReader & reader);
    void deserializeUserPresets(ProjectReader & reader);
    SynthPreset deserializePreset(ProjectReader & reader) const;
    void deserializePresetParameter(ProjectReader & reader, SynthPreset & preset) const;
    float legacyPresetParameterValue(ProjectReader & reader, const std::string & paramName, const QString & xmlValue) const;

    AudioEngineS m_audioEngine;
    DataServiceS m_dataService;
    UserPresets m_synthUserPresets;
    std::string m_projectPath;
    SamplerAudioFileReaderFactory m_samplerAudioFileReaderFactory;
};

} // namespace noteahead

#endif // DEVICE_SERVICE_HPP
