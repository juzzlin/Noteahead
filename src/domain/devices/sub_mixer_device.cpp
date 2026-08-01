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

#include "sub_mixer_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "../dsp/audio_context.hpp"
#include "../dsp/true_stereo_panner.hpp"

#include <algorithm>

namespace noteahead {

SubMixerDevice::SubMixerDevice(std::string name)
  : m_name { std::move(name) }
{
}

SubMixerDevice::~SubMixerDevice() = default;

std::string SubMixerDevice::name() const
{
    return m_name;
}

std::string SubMixerDevice::category() const
{
    return Constants::NahdXml::xmlValueMixers().toStdString();
}

std::string SubMixerDevice::typeName() const
{
    return Constants::subMixerDeviceName().toStdString();
}

std::string SubMixerDevice::typeIdString()
{
    return "f4a2e7b8-6c31-4d95-8e0a-2b7f1c6d3e94";
}

std::string SubMixerDevice::typeId() const
{
    return typeIdString();
}

SubMixerDevice::SlotList SubMixerDevice::members() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_members;
}

void SubMixerDevice::setMembers(SlotList members)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        m_members = std::move(members);
    }
    emit dataChanged();
}

void SubMixerDevice::processMidiNoteOn(uint8_t, uint8_t)
{
}

void SubMixerDevice::processMidiNoteOff(uint8_t)
{
}

std::vector<MidiCcController> SubMixerDevice::availableMidiCcControllers() const
{
    using namespace MidiCcMapping;
    return {
        faderMidiCcController(),
        { static_cast<uint8_t>(Controller::PanMSB), "Pan" }
    };
}

void SubMixerDevice::processMidiCc(uint8_t controller, uint8_t value, uint8_t)
{
    using namespace MidiCcMapping;

    bool changed = false;
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };

        if (controller == static_cast<uint8_t>(Controller::ResetAllControllers)) {
            // Back to whatever the knobs were set to by hand, discarding what CC rode them to.
            changed |= updateVolumeParameter(manualVolumeInternal(), false);
            changed |= updatePanParameter(manualPanInternal(), false);
            changed |= updateGainParameter(manualGainInternal(), false);
        } else {
            const auto val = static_cast<float>(value) / 127.0f;
            if (controller == static_cast<uint8_t>(Controller::ChannelVolumeMSB)) {
                changed |= updateVolumeParameter(faderPositionFromMidiCc(value), false);
            } else if (controller == static_cast<uint8_t>(Controller::PanMSB)) {
                changed |= updatePanParameter(val, false);
            }
        }
    }

    if (changed) {
        emit dataChanged();
    }
}

void SubMixerDevice::processMidiAllNotesOff()
{
}

std::vector<size_t> SubMixerDevice::claimedOutputSlots() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return m_members;
}

std::vector<size_t> SubMixerDevice::sidechainDependencies() const
{
    auto dependencies = Device::sidechainDependencies();
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    dependencies.insert(dependencies.end(), m_members.begin(), m_members.end());
    return dependencies;
}

void SubMixerDevice::sidechainDependencies(std::vector<size_t> & out) const
{
    Device::sidechainDependencies(out);
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    out.insert(out.end(), m_members.begin(), m_members.end());
}

bool SubMixerDevice::hasActiveAudio() const
{
    return true;
}

void SubMixerDevice::processAudio(AudioContext & context)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    const auto bufferSize = context.frameCount * 2;
    const auto gain = static_cast<double>(linearGainInternal());

    TrueStereoPanner panner;
    panner.setPan(static_cast<double>(panInternal()));

    for (const auto slotIndex : m_members) {
        // A member may have been removed since the list was resolved, and a member that has not
        // rendered yet this callback would be stale rather than silent, so skip anything the engine
        // is not currently exposing.
        if (slotIndex >= context.deviceOutputBuffers.size()) {
            continue;
        }
        const auto & memberBuffer = context.deviceOutputBuffers[slotIndex];
        if (memberBuffer.size() < bufferSize) {
            continue;
        }
        for (uint32_t i = 0; i < bufferSize; i++) {
            context.buffer[i] += memberBuffer[i] * gain;
        }
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        auto & left = context.buffer[i * 2];
        auto & right = context.buffer[i * 2 + 1];
        panner.process(left, right);
    }
}

void SubMixerDevice::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };

    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyMembers());
    for (const auto slotIndex : m_members) {
        writer.writeStartElement(Constants::NahdXml::xmlKeyMember());
        writer.writeAttribute(Constants::NahdXml::xmlKeySlot(), QString::number(slotIndex));
        writer.writeEndElement();
    }
    writer.writeEndElement();

    writer.writeEndElement(); // Device
}

void SubMixerDevice::deserializeFromXml(ProjectReader & reader)
{
    deserializeAttributesFromXml(reader);

    SlotList members;
    while (reader.readNextStartElement()) {
        const auto name = reader.name();
        if (name == Constants::NahdXml::xmlKeyParameters()) {
            deserializeParametersFromXml(reader);
        } else if (name == Constants::NahdXml::xmlKeyInsertEffects()) {
            insertEffectRack().deserializeEffectsFromXml(reader);
        } else if (name == Constants::NahdXml::xmlKeyParameter()) {
            deserializeParameter(reader);
        } else if (name == Constants::NahdXml::xmlKeyMembers()) {
            while (reader.readNextStartElement()) {
                if (reader.name() == Constants::NahdXml::xmlKeyMember()) {
                    if (const auto slot = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeySlot()); slot.has_value()) {
                        members.push_back(static_cast<size_t>(slot.value()));
                    }
                }
                reader.skipCurrentElement();
            }
        } else {
            reader.skipCurrentElement();
        }
    }

    // Parameters only reach volume()/pan()/gain() through syncParameters(), which the base class
    // implementation calls for us but this override has to do itself.
    syncParameters();

    setMembers(std::move(members));
}

} // namespace noteahead
