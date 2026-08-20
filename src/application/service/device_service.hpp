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
#include <QVariantList>

#include <functional>
#include <memory>
#include <string>

namespace noteahead {

class AudioEngine;
class AudioFileReader;
class DataService;
class Instrument;
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

    //! Applies an instrument's patch to the internal device it plays through, as a program change.
    //! Bank select is left out on purpose: an internal device has no banks to select from.
    void applyInstrumentPatch(const Instrument & instrument);

    //! Applies an instrument's MIDI CC settings, preceded by a reset so a controller dropped from
    //! the settings stops affecting the device rather than lingering at its last value.
    //!
    //! Kept apart from the patch because the reset is not free: it drops whatever automation is in
    //! effect, which is right when the settings are being applied and wrong for a bare patch change.
    void applyInstrumentMidiCcSettings(const Instrument & instrument);

    //! Both of the above, in the order the wire would carry them. What a render applies before its
    //! first tick, so that an export starts from the same place playback does.
    void applyInstrumentSettings(const Instrument & instrument);

    //! Hands every device back its authored values after playback.
    //!
    //! MIDI CC -- from automations, from a controller, from a track's instrument settings -- writes
    //! only the live layer of a parameter, so this is what makes the panels and the sound return to
    //! the patch once the transport stops. Devices also do it themselves on CC 121; this is the
    //! sweep for everything the message never reached.
    void clearAutomation();

    using InternalDeviceNames = std::vector<std::string>;
    virtual InternalDeviceNames internalDeviceNames() const;

    //! Add a device slot to a SubMixer's member list.
    //!
    //! Fails if either slot is empty, if the target is not a SubMixer, or if the addition would
    //! create a cycle. A device can only belong to one SubMixer, so it is removed from any other
    //! first -- otherwise it would be summed twice and heard twice.
    //! Whether addSubMixerMember() would succeed. The UI asks rather than reimplementing the
    //! rules, so a greyed-out entry always matches what the service would actually do.
    Q_INVOKABLE bool canAddSubMixerMember(int subMixerSlot, int memberSlot) const;
    Q_INVOKABLE bool addSubMixerMember(int subMixerSlot, int memberSlot);
    Q_INVOKABLE bool removeSubMixerMember(int subMixerSlot, int memberSlot);
    Q_INVOKABLE QVariantList subMixerMembers(int subMixerSlot) const;
    //! Slot of the SubMixer that claims this device, or -1 when it goes straight to the master.
    Q_INVOKABLE int subMixerOwningSlot(int memberSlot) const;

    Q_INVOKABLE virtual QStringList internalDeviceNamesQt() const;

    Q_INVOKABLE virtual QStringList categories() const;
    Q_INVOKABLE virtual QStringList devicesByCategory(const QString & category) const;

    void setSynthUserPresets(const UserPresets & presets);
    UserPresets synthUserPresets() const;
    void saveSynthUserPreset(int index, const SynthPreset & preset);

    void setProjectPath(const std::string & projectPath);

    void serializeToXml(ProjectWriter & writer) const;
    void deserializeFromXml(ProjectReader & reader);

    //! The engine, for callers that need its whole-callback load meter.
    AudioEngineS audioEngine() const;

    using SamplerAudioFileReaderFactory = std::function<std::unique_ptr<AudioFileReader>()>;
    void setSamplerAudioFileReaderFactory(SamplerAudioFileReaderFactory factory);

    Q_INVOKABLE bool exportDeviceSettings(int slotIndex, const QString & filePath) const;
    bool exportDeviceSettings(int slotIndex, ProjectWriter & writer) const;

    Q_INVOKABLE bool importDeviceSettings(int slotIndex, const QString & filePath);
    bool importDeviceSettings(int slotIndex, ProjectReader & reader);

    //! Duplicate the device in sourceSlot into targetSlot (in-memory clone). Returns false if the
    //! source slot is empty or source and target are the same slot.
    bool copyDevice(int sourceSlot, int targetSlot);

    struct DeviceTypeInfo
    {
        QString typeId;
        QString typeName;
    };

    DeviceTypeInfo peekDeviceTypeInfo(const QString & filePath) const;
    DeviceTypeInfo peekDeviceTypeInfo(ProjectReader & reader) const;

    std::map<QString, QString> getFilesToEmbed() const;

    void reset();

    //! Clear every LUFS meter in the master racks and in the devices' insert racks, so that an
    //! integrated reading covers the take about to play rather than everything heard since startup.
    void resetLoudnessMeters();

    EffectRack & sendEffectRack();
    EffectRack & insertEffectRack();

signals:
    void dataChanged();
    void synthUserPresetsChanged(const UserPresets & presets);

private:
    bool importDeviceSettingsFromXml(int slotIndex, const QString & xml);

    //! Whether routing memberSlot into subMixerSlot would close a loop, walking the membership
    //! chain upwards. The audio engine tolerates cycles by dumping them into a trailing layer, but
    //! the result is meaningless, so they are rejected here instead.
    bool wouldCreateCycle(size_t subMixerSlot, size_t memberSlot) const;
    //! Drops member slots that no longer hold a device, so a deleted device cannot leave a
    //! SubMixer silently claiming an empty slot.
    void pruneSubMixerMembers();

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
