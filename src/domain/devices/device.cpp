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

#include "device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../../infra/midi/midi_cc_mapping.hpp"
#include "../dsp/true_stereo_panner.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

Parameter Device::faderParameter()
{
    // Stored as a 0..1 position on the throw rather than as a gain, so the taper can change without
    // the stored number changing meaning. Projects saved before the boost range existed carry a
    // "volume" that was the linear gain itself, which unmapFader() converts back to a position.
    return Parameter {
        Constants::NahdXml::xmlKeyFader().toStdString(),
        Constants::faderUnityPosition(),
        0,
        10000,
        static_cast<int>(std::lround(Constants::faderUnityPosition() * 10000.0f)),
        100,
        Parameter::Type::Continuous,
        { Constants::NahdXml::xmlKeyVolume().toStdString() },
        [](float legacyGain) { return static_cast<float>(ParameterMapper::unmapFader(static_cast<double>(legacyGain))); }
    };
}

Device::Device()
{
    addParameter(faderParameter());
    addParameter(Parameter { Constants::NahdXml::xmlKeyGain().toStdString(), 0.5f, -3000, 3000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { Constants::NahdXml::xmlKeyPan().toStdString(), 0.5f, 0, 10000, 5000, 100 });

    // Defaults reproduce the chain as it was before the channel strip existed, so a project saved
    // back then — which carries neither key — loads and sounds exactly as it did.
    addParameter(Parameter { Constants::NahdXml::xmlKeyFaderPosition().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySendTap().toStdString(), 0.0f, 0, 1, 0, 1, Parameter::Type::Discrete });

    m_volume = Constants::faderUnityPosition();

    m_reverbSends.resize(Constants::effectRackSize(), 0.0f);
}

size_t Device::id() const
{
    return m_id;
}

void Device::setId(size_t id)
{
    m_id = id;
}

std::vector<MidiCcController> Device::availableMidiCcControllers() const
{
    return {};
}

void Device::serializeToXml(ProjectWriter & writer) const
{
    writer.writeStartElement(Constants::NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);

    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    m_insertEffectRack.serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(Constants::NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    writer.writeEndElement(); // Device
}

void Device::deserializeFromXml(ProjectReader & reader)
{
    {
        std::lock_guard<std::recursive_mutex> lock { m_mutex };
        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
            if (token == ProjectReader::TokenType::EndElement && reader.name() == Constants::NahdXml::xmlKeyDevice()) {
                break;
            }

            if (token == ProjectReader::TokenType::StartElement) {
                if (reader.name() == Constants::NahdXml::xmlKeyParameters()) {
                    deserializeParametersFromXml(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeyInsertEffects()) {
                    m_insertEffectRack.deserializeEffectsFromXml(reader);
                } else if (reader.name() == Constants::NahdXml::xmlKeyParameter()) {
                    deserializeParameter(reader);
                } else {
                    reader.skipCurrentElement();
                }
            }
        }

        syncParameters();
    }
    emit dataChanged();
}

uint32_t Device::sampleRate() const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_sampleRate;
}

void Device::setSampleRate(uint32_t sampleRate)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { m_mutex };
        if (m_sampleRate != sampleRate) {
            m_sampleRate = sampleRate;
            changed = true;
        }
    }
    if (changed) {
        emit sampleRateChanged();
    }
}

std::recursive_mutex & Device::mutex() const
{
    return m_mutex;
}

float Device::volume() const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_volume;
}

void Device::setVolume(float volume)
{
    if (updateVolumeParameter(volume, true)) {
        emit dataChanged();
    }
}

float Device::faderPositionFromMidiCc(uint8_t value)
{
    return std::min(1.0f, static_cast<float>(value) / 127.0f * Constants::faderUnityPosition());
}

MidiCcController Device::faderMidiCcController()
{
    // Named after the knob it drives rather than after the MIDI controller it rides on: every
    // device labels this Fader, and the automation editor takes its name from here.
    return { static_cast<uint8_t>(MidiCcMapping::Controller::ChannelVolumeMSB), "Fader", 0, Constants::faderMaxMidiCcValue() };
}

float Device::gain() const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_gain;
}

void Device::setGain(float gain)
{
    if (updateGainParameter(gain, true)) {
        emit dataChanged();
    }
}

float Device::pan() const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_pan;
}

void Device::setPan(float pan)
{
    if (updatePanParameter(pan, true)) {
        emit dataChanged();
    }
}

float Device::reverbSend(size_t index) const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    if (index < m_reverbSends.size()) {
        return m_reverbSends[index];
    }
    return 0.0f;
}

void Device::setReverbSend(size_t index, float send)
{
    if (updateReverbSendParameter(index, send)) {
        emit dataChanged();
    }
}

size_t Device::reverbSendCount() const
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_reverbSends.size();
}

bool Device::updateVolumeParameter(float volume, bool authored)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    if (auto p = parameter(Constants::NahdXml::xmlKeyFader().toStdString()); p) {
        const float oldVal = p->get().value();
        if (authored) {
            p->get().setValue(volume);
        } else {
            p->get().setAutomationValue(volume);
        }
        syncParameters();
        return !qFuzzyCompare(p->get().value(), oldVal);
    }
    return false;
}

bool Device::updateGainParameter(float gain, bool authored)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    if (auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        const float oldVal = p->get().value();
        if (authored) {
            p->get().setValue(gain);
        } else {
            p->get().setAutomationValue(gain);
        }
        syncParameters();
        return !qFuzzyCompare(p->get().value(), oldVal);
    }
    return false;
}

bool Device::updatePanParameter(float pan, bool authored)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    if (auto p = parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p) {
        const float oldVal = p->get().value();
        if (authored) {
            p->get().setValue(pan);
        } else {
            p->get().setAutomationValue(pan);
        }
        syncParameters();
        return !qFuzzyCompare(p->get().value(), oldVal);
    }
    return false;
}

bool Device::updateReverbSendParameter(size_t index, float send)
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };

    // Reverb sends are not parameters and nothing automates them, so there is no live layer here.
    if (index < m_reverbSends.size()) {
        const float oldVal = m_reverbSends[index];
        m_reverbSends[index] = send;
        return !qFuzzyCompare(m_reverbSends[index], oldVal);
    }
    return false;
}

void Device::syncParameters()
{
    if (auto p = parameter(Constants::NahdXml::xmlKeyFader().toStdString()); p) {
        m_volume = p->get().value();
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyGain().toStdString()); p) {
        m_gain = p->get().value();
        m_linearGain = static_cast<float>(ParameterMapper::mapDecibel(m_gain, 30.0));
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyPan().toStdString()); p) {
        m_pan = p->get().value();
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeyFaderPosition().toStdString()); p) {
        m_faderPosition = p->get().xmlValue() ? FaderPosition::PostInserts : FaderPosition::PreInserts;
    }
    if (auto p = parameter(Constants::NahdXml::xmlKeySendTap().toStdString()); p) {
        m_sendTap = p->get().xmlValue() ? SendTap::PreFader : SendTap::PostFader;
    }
}

void Device::setContinuousParameterValue(const std::string & key, float value)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { m_mutex };
        if (const auto p = parameter(key); p) {
            p->get().setValue(value);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

void Device::setDiscreteParameterValue(const std::string & key, int value)
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { m_mutex };
        if (const auto p = parameter(key); p) {
            p->get().setFromXml(value);
            syncParameters();
            changed = true;
        }
    }
    if (changed)
        emit dataChanged();
}

void Device::setBpm(float bpm)
{
    m_insertEffectRack.setBpm(bpm);
}

void Device::reset()
{
    ParameterContainer::reset();
    syncParameters();
    m_insertEffectRack.reset();
}

void Device::resetAudio()
{
    m_insertEffectRack.reset();
}

void Device::saveState()
{
    std::lock_guard<std::recursive_mutex> lock { m_mutex };
    m_savedParameters = parameterSnapshot();
}

void Device::restoreState()
{
    {
        std::lock_guard<std::recursive_mutex> lock { m_mutex };
        restoreParameterSnapshot(m_savedParameters);
        syncParameters();
    }
    emit dataChanged();
}

void Device::clearAutomation()
{
    bool changed = false;
    {
        std::lock_guard<std::recursive_mutex> lock { m_mutex };
        changed = clearAutomationInternal();
    }
    // Never emitted under the lock: the receivers read back from the audio engine, whose callback
    // holds the engine mutex and then waits for this one.
    if (changed) {
        emit parametersChanged();
    }
}

bool Device::clearAutomationInternal()
{
    if (!isAutomated()) {
        return false;
    }
    ParameterContainer::clearAutomation();
    syncParameters();
    return true;
}

bool Device::insertEffectsSettled() const
{
    return m_insertEffectRack.isSettled();
}

void Device::renderBlock(AudioContext & context)
{
    uint32_t rendered = 0;
    const uint64_t blockEnd = context.startFrame + context.frameCount;

    while (rendered < context.frameCount) {
        const uint64_t at = context.startFrame + rendered;
        applyScheduledEvents(at);

        // Where the next event falls, and so where this piece has to stop. Looking from the frame
        // after this one, since whatever was due at `at` has just been applied.
        const auto next = nextScheduledEventFrame(at + 1, blockEnd);
        const uint32_t until = next ? static_cast<uint32_t>(*next - context.startFrame) : context.frameCount;

        AudioContext piece = context;
        piece.buffer = context.buffer.subspan(static_cast<size_t>(rendered) * 2, static_cast<size_t>(until - rendered) * 2);
        piece.frameCount = until - rendered;
        piece.startFrame = at;
        processAudio(piece);

        rendered = until;
    }
}

void Device::scheduleMidiEvent(const ScheduledEvent & event)
{
    const std::lock_guard<std::recursive_mutex> lock { m_mutex };
    m_scheduledEvents.push_back(event);
}

void Device::applyScheduledEvents(uint64_t frame)
{
    const std::lock_guard<std::recursive_mutex> lock { m_mutex };
    if (m_scheduledEvents.empty()) {
        return;
    }

    // The queue is filled in time order, so everything due sits at the front and the rest keeps its
    // order. The events are applied through the immediate entry points, which is what makes this
    // work for every device without any of them having to know the frame exists.
    auto due = m_scheduledEvents.begin();
    while (due != m_scheduledEvents.end() && due->frame <= frame) {
        switch (due->type) {
        case ScheduledEvent::Type::NoteOn:
            processMidiNoteOn(due->note, due->velocity);
            break;
        case ScheduledEvent::Type::NoteOff:
            processMidiNoteOff(due->note);
            break;
        case ScheduledEvent::Type::Cc:
            processMidiCc(due->controller, due->velocity, due->channel);
            break;
        case ScheduledEvent::Type::PitchBend:
            processMidiPitchBend(due->value, due->channel);
            break;
        case ScheduledEvent::Type::ProgramChange:
            processMidiProgramChange(due->programme, due->channel);
            break;
        case ScheduledEvent::Type::AllNotesOff:
            processMidiAllNotesOff();
            break;
        }
        due++;
    }
    m_scheduledEvents.erase(m_scheduledEvents.begin(), due);
}

std::optional<uint64_t> Device::nextScheduledEventFrame(uint64_t from, uint64_t to) const
{
    const std::lock_guard<std::recursive_mutex> lock { m_mutex };
    for (auto && event : m_scheduledEvents) {
        if (event.frame >= to) {
            break; // Time order, so nothing behind this one can fall inside the block either
        }
        if (event.frame >= from) {
            return event.frame;
        }
    }
    return std::nullopt;
}

void Device::clearScheduledEvents()
{
    const std::lock_guard<std::recursive_mutex> lock { m_mutex };
    m_scheduledEvents.clear();
}

size_t Device::scheduledEventCount() const
{
    const std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_scheduledEvents.size();
}

bool Device::hasScheduledEvents() const
{
    const std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return !m_scheduledEvents.empty();
}

void Device::processInsertEffects(AudioContext & context)
{
    m_insertEffectRack.processInPlace(context);
}

void Device::applyFader(AudioContext & context) const
{
    double volume {};
    {
        const std::lock_guard<std::recursive_mutex> lock { m_mutex };
        volume = ParameterMapper::mapFader(static_cast<double>(m_volume));
    }

    const uint32_t sampleCount = context.frameCount * 2;
    for (uint32_t i = 0; i < sampleCount; i++) {
        context.buffer[i] *= volume;
    }
}

Device::FaderPosition Device::faderPosition() const
{
    const std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_faderPosition;
}

void Device::setFaderPosition(FaderPosition position)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeyFaderPosition().toStdString(), static_cast<int>(position));
}

Device::SendTap Device::sendTap() const
{
    const std::lock_guard<std::recursive_mutex> lock { m_mutex };
    return m_sendTap;
}

void Device::setSendTap(SendTap tap)
{
    setDiscreteParameterValue(Constants::NahdXml::xmlKeySendTap().toStdString(), static_cast<int>(tap));
}

std::vector<size_t> Device::claimedOutputSlots() const
{
    return {};
}

std::vector<size_t> Device::sidechainDependencies() const
{
    return m_insertEffectRack.sidechainDependencies();
}

void Device::sidechainDependencies(std::vector<size_t> & out) const
{
    m_insertEffectRack.sidechainDependencies(out);
}

EffectRack & Device::insertEffectRack()
{
    return m_insertEffectRack;
}

const EffectRack & Device::insertEffectRack() const
{
    return m_insertEffectRack;
}

void Device::adoptChannelStripFrom(const Device & other)
{
    if (&other == this) {
        return;
    }

    m_insertEffectRack.copyFrom(other.insertEffectRack());

    // Through the virtual setters, so that a device folding pan or gain into its voices picks the
    // values up rather than only storing them.
    setVolume(other.volume());
    setGain(other.gain());
    setPan(other.pan());
    setFaderPosition(other.faderPosition());
    setSendTap(other.sendTap());

    // The send count is fixed by the constructor and identical for every device, but take the
    // smaller of the two anyway so this can never write past either end.
    for (size_t i = 0; i < std::min(reverbSendCount(), other.reverbSendCount()); i++) {
        setReverbSend(i, other.reverbSend(i));
    }

    emit dataChanged();
}

AudioScope & Device::scope()
{
    return m_scope;
}

LevelMeter & Device::meter()
{
    return m_meter;
}

const LevelMeter & Device::meter() const
{
    return m_meter;
}

LoadMeter & Device::loadMeter()
{
    return m_loadMeter;
}

ClipDetector & Device::clipDetector()
{
    return m_clipDetector;
}

const ClipDetector & Device::clipDetector() const
{
    return m_clipDetector;
}

const LoadMeter & Device::loadMeter() const
{
    return m_loadMeter;
}

float Device::volumeInternal() const
{
    return m_volume;
}

float Device::gainInternal() const
{
    return m_gain;
}

float Device::panInternal() const
{
    return m_pan;
}

float Device::linearGainInternal() const
{
    return m_linearGain;
}

float Device::reverbSendInternal(size_t index) const
{
    if (index < m_reverbSends.size()) {
        return m_reverbSends[index];
    }
    return 0.0f;
}

void Device::serializeAttributesToXml(ProjectWriter & writer) const
{
    writer.writeAttribute(Constants::NahdXml::xmlKeySlot(), QString::number(m_id));
    writer.writeAttribute(Constants::NahdXml::xmlKeyName(), QString::fromStdString(name()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeName(), QString::fromStdString(typeName()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyCategory(), QString::fromStdString(category()));
    writer.writeAttribute(Constants::NahdXml::xmlKeyTypeId(), QString::fromStdString(typeId()));
}

void Device::deserializeAttributesFromXml(ProjectReader & reader)
{
    if (const auto slot = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeySlot(), false); slot.has_value()) {
        m_id = slot.value();
    } else if (const auto id = Utils::Xml::readUIntAttribute(reader, Constants::NahdXml::xmlKeyId(), false); id.has_value()) {
        m_id = id.value();
    }
}

} // namespace noteahead
