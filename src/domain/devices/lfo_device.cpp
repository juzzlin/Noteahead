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

#include "lfo_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../tracker/parameter_display.hpp"

#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace NahdXml = Constants::NahdXml;

namespace {

//! Smallest change worth a step: one step of the grid a project saves knob positions on.
constexpr double StepThreshold { 1.0e-4 };

} // namespace

LfoDevice::LfoDevice(std::string name)
  : m_name { std::move(name) }
{
    for (size_t lfo = 0; lfo < LfoCount; lfo++) {
        addLfoParameters(lfo);
        describeLfoParameters(lfo);
    }
    LfoDevice::syncParameters();
    for (auto && slot : m_slots) {
        slot.lfo.reset();
        slot.lfo.setPhase(slot.phase);
    }
}

LfoDevice::~LfoDevice() = default;

std::string LfoDevice::name() const
{
    return m_name;
}

std::string LfoDevice::category() const
{
    return NahdXml::xmlValueModulators().toStdString();
}

std::string LfoDevice::typeName() const
{
    return Constants::lfoDeviceName().toStdString();
}

std::string LfoDevice::typeIdString()
{
    return "eaa763fd-5640-4676-8a5f-76963b051615";
}

std::string LfoDevice::typeId() const
{
    return typeIdString();
}

std::string LfoDevice::lfoKey(size_t lfo, const QString & key)
{
    return "lfo" + std::to_string(lfo + 1) + "_" + key.toStdString();
}

void LfoDevice::addLfoParameters(size_t lfo)
{
    const auto continuous = [this, lfo](const QString & key, float value) {
        addParameter(Parameter { lfoKey(lfo, key), value, 0, 10000, static_cast<int>(std::lround(value * 10000.0f)), 100 });
    };
    const auto discrete = [this, lfo](const QString & key, int value, int min, int max) {
        addParameter(Parameter { lfoKey(lfo, key), static_cast<float>(value), min, max, value, 1, Parameter::Type::Discrete });
    };
    // Triangle and free-running, the synths' LFO defaults, at half depth either way of the setting
    discrete(NahdXml::xmlKeyWaveform(), static_cast<int>(Lfo::Waveform::Triangle), 0, 4);
    discrete(NahdXml::xmlKeyMode(), static_cast<int>(Lfo::Mode::Normal), 0, 2);
    continuous(NahdXml::xmlKeyRate(), 0.5f);
    discrete(NahdXml::xmlKeyRateDivider(), 1, 1, Lfo::MaxRateDivider);
    continuous(NahdXml::xmlKeyIntensity(), 0.5f);
    discrete(NahdXml::xmlKeyModulationMode(), static_cast<int>(ParameterModulation::Mode::Bipolar), 0, ParameterModulation::ModeCount - 1);
    continuous(NahdXml::xmlKeyDelay(), 0.0f);
    continuous(NahdXml::xmlKeyFade(), 0.0f);
    continuous(NahdXml::xmlKeyPhase(), 0.0f);
    continuous(NahdXml::xmlKeySmoothing(), 0.0f);
    addParameter(Parameter { lfoKey(lfo, NahdXml::xmlKeyRetrigger()), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    continuous(NahdXml::xmlKeyVelocitySensitivity(), 0.0f);
}

void LfoDevice::describeLfoParameters(size_t lfo)
{
    using Display = ParameterDisplay;
    const auto describe = [this, lfo](const QString & key, Display display) {
        setParameterDisplay(lfoKey(lfo, key), std::move(display));
    };
    const auto percent = [&describe](const QString & key, const char * label) {
        describe(key, { .unit = "%", .max = 100.0, .label = label });
    };
    // As the free-running knob reads it; a synced LFO's rate is a note length, which the dialog shows
    describe(NahdXml::xmlKeyRate(), { .mapping = Display::Mapping::LfoFrequency, .unit = "Hz", .min = MinRateHz, .max = MaxRateHz, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Rate") });
    percent(NahdXml::xmlKeyIntensity(), QT_TRANSLATE_NOOP("ParameterDisplay", "Depth"));
    describe(NahdXml::xmlKeyDelay(), { .mapping = Display::Mapping::Cubic, .unit = "s", .min = 0.0, .max = Constants::maxLfoEnvelopeTimeSeconds(), .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Delay") });
    describe(NahdXml::xmlKeyFade(), { .mapping = Display::Mapping::Cubic, .unit = "s", .min = 0.0, .max = Constants::maxLfoEnvelopeTimeSeconds(), .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Fade") });
    describe(NahdXml::xmlKeyPhase(), { .mapping = Display::Mapping::Value, .unit = "°", .min = 0.0, .max = 360.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Phase") });
    describe(NahdXml::xmlKeySmoothing(), { .mapping = Display::Mapping::Cubic, .unit = "s", .min = 0.0, .max = MaxSmoothingSeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Smoothing") });
    percent(NahdXml::xmlKeyVelocitySensitivity(), QT_TRANSLATE_NOOP("ParameterDisplay", "Velocity"));
}

void LfoDevice::syncParameters()
{
    Device::syncParameters();
    for (size_t lfo = 0; lfo < LfoCount; lfo++) {
        auto & slot = m_slots.at(lfo);
        const auto value = [this, lfo](const QString & key) {
            const auto p = parameter(lfoKey(lfo, key));
            return p ? p->get().value() : 0.0f;
        };
        slot.lfo.setWaveform(static_cast<Lfo::Waveform>(std::clamp(static_cast<int>(std::lround(value(NahdXml::xmlKeyWaveform()))), 0, 4)));
        slot.mode = static_cast<Lfo::Mode>(std::clamp(static_cast<int>(std::lround(value(NahdXml::xmlKeyMode()))), 0, 2));
        slot.lfo.setMode(slot.mode);
        slot.rate = value(NahdXml::xmlKeyRate());
        slot.lfo.setRateDivider(static_cast<int>(std::lround(value(NahdXml::xmlKeyRateDivider()))));
        slot.depth = value(NahdXml::xmlKeyIntensity());
        slot.modulationMode = static_cast<ParameterModulation::Mode>(std::clamp(static_cast<int>(std::lround(value(NahdXml::xmlKeyModulationMode()))), 0, ParameterModulation::ModeCount - 1));
        slot.lfo.setDelayTime(ParameterMapper::mapCubic(value(NahdXml::xmlKeyDelay()), 0.0, Constants::maxLfoEnvelopeTimeSeconds()));
        slot.lfo.setFadeTime(ParameterMapper::mapCubic(value(NahdXml::xmlKeyFade()), 0.0, Constants::maxLfoEnvelopeTimeSeconds()));
        slot.phase = value(NahdXml::xmlKeyPhase());
        slot.smoothing = value(NahdXml::xmlKeySmoothing());
        slot.retrigger = value(NahdXml::xmlKeyRetrigger()) > 0.5f;
        slot.velocitySensitivity = value(NahdXml::xmlKeyVelocitySensitivity());
    }
}

bool LfoDevice::supportsOctave() const
{
    return false;
}

void LfoDevice::restart(Slot & slot) const
{
    slot.lfo.trigger();
    slot.lfo.setPhase(slot.phase);
}

void LfoDevice::processDeviceMidiNoteOn(uint8_t, uint8_t velocity)
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && slot : m_slots) {
        slot.velocity = static_cast<float>(velocity) / 127.0f;
        // A one-shot runs once per note, which is what it is for, whether or not it is set to restart
        if (slot.retrigger || slot.mode == Lfo::Mode::OneShot) {
            restart(slot);
        }
    }
}

void LfoDevice::processDeviceMidiNoteOff(uint8_t)
{
}

void LfoDevice::processDeviceMidiCc(uint8_t, uint8_t, uint8_t)
{
}

void LfoDevice::processMidiAllNotesOff()
{
}

void LfoDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
}

bool LfoDevice::hasActiveAudio() const
{
    return false;
}

void LfoDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
}

void LfoDevice::resetAudio()
{
    // Back to the start of the shape, as a render begins: the same render twice is the same render
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && slot : m_slots) {
        slot.lfo.reset();
        slot.lfo.setPhase(slot.phase);
        slot.smoothed = 0.0;
        slot.velocity = 1.0f;
        slot.lastSent.reset();
        slot.lastMode.reset();
    }
    m_gridOrigin.reset();
}

double LfoDevice::effectiveDepth(const Slot & slot) const
{
    const double sensitivity = static_cast<double>(slot.velocitySensitivity);
    return static_cast<double>(slot.depth) * (1.0 - sensitivity + sensitivity * static_cast<double>(slot.velocity));
}

bool LfoDevice::updateLfoParameter(size_t lfo, const QString & key, float value)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        const auto p = parameter(lfoKey(lfo, key));
        if (!p) {
            return false;
        }
        p->get().setValue(value);
        syncParameters();
    }
    emit dataChanged();
    return true;
}

float LfoDevice::lfoParameterValue(size_t lfo, const QString & key) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    const auto p = parameter(lfoKey(lfo, key));
    return p ? p->get().shownValue() : 0.0f;
}

ParameterAutomation::Target LfoDevice::target(size_t lfo) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return lfo < LfoCount ? m_slots.at(lfo).target : ParameterAutomation::Target {};
}

void LfoDevice::setTarget(size_t lfo, ParameterAutomation::Target target)
{
    if (lfo >= LfoCount) {
        return;
    }
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        auto & slot = m_slots.at(lfo);
        if (slot.target == target) {
            return;
        }
        // Let go of the old one at the next step, unless it never got anything
        if (!slot.target.ownerId.empty() && slot.lastSent) {
            slot.released = slot.target;
        }
        slot.target = std::move(target);
        slot.lastSent.reset();
        slot.lastMode.reset();
    }
    emit dataChanged();
}

void LfoDevice::evaluate(uint64_t startFrame, uint32_t frameCount, double sampleRate, double bpm, std::vector<Step> & steps)
{
    if (sampleRate <= 0.0 || !frameCount) {
        return;
    }
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    m_bpm = bpm;
    const auto gridFrames = std::max<uint64_t>(1, static_cast<uint64_t>(std::llround(sampleRate / ControlRateHz)));
    const double controlRate = sampleRate / static_cast<double>(gridFrames);

    for (size_t lfo = 0; lfo < LfoCount; lfo++) {
        auto & slot = m_slots.at(lfo);
        slot.lfo.setSampleRate(controlRate);
        if (slot.mode == Lfo::Mode::BPM) {
            slot.lfo.setFrequency(bpm, static_cast<double>(slot.rate));
        } else {
            slot.lfo.setFrequency(ParameterMapper::mapLfoFrequency(static_cast<double>(slot.rate), MinRateHz, MaxRateHz));
        }
        if (slot.released) {
            steps.push_back({ lfo, startFrame, *slot.released, std::nullopt, 0.0, 0.0 });
            slot.released.reset();
        }
    }

    if (!m_gridOrigin || *m_gridOrigin > startFrame) {
        m_gridOrigin = startFrame;
    }
    const uint64_t end = startFrame + frameCount;
    const uint64_t sinceOrigin = startFrame - *m_gridOrigin;
    for (uint64_t frame = *m_gridOrigin + (sinceOrigin + gridFrames - 1) / gridFrames * gridFrames; frame < end; frame += gridFrames) {
        // Notes due by this step restart what they restart before it is taken
        applyScheduledEvents(frame);
        for (size_t lfo = 0; lfo < LfoCount; lfo++) {
            auto & slot = m_slots.at(lfo);
            const double value = slot.lfo.nextSample();
            const double tau = ParameterMapper::mapCubic(static_cast<double>(slot.smoothing), 0.0, MaxSmoothingSeconds);
            if (tau > 0.0) {
                const double coefficient = std::exp(-1.0 / (tau * controlRate));
                slot.smoothed = coefficient * slot.smoothed + (1.0 - coefficient) * value;
            } else {
                slot.smoothed = value;
            }
            if (slot.target.ownerId.empty() || slot.target.parameter.empty()) {
                continue;
            }
            const double depth = effectiveDepth(slot);
            const bool changed = !slot.lastSent || slot.lastMode != slot.modulationMode
              || std::abs(slot.smoothed - slot.lastSent->first) >= StepThreshold || std::abs(depth - slot.lastSent->second) >= StepThreshold;
            if (changed) {
                steps.push_back({ lfo, frame, slot.target, slot.modulationMode, slot.smoothed, depth });
                slot.lastSent = std::pair { slot.smoothed, depth };
                slot.lastMode = slot.modulationMode;
            }
        }
    }
}

Device::AutomatableParameterList LfoDevice::automatableParameters() const
{
    const auto lfoOf = [](const std::string & key) -> std::optional<size_t> {
        for (size_t lfo = 0; lfo < LfoCount; lfo++) {
            if (key.starts_with("lfo" + std::to_string(lfo + 1) + "_")) {
                return lfo;
            }
        }
        return std::nullopt;
    };
    auto list = Device::automatableParameters();
    for (auto && entry : list) {
        if (const auto lfo = lfoOf(entry.key); lfo) {
            entry.groupName = "LFO " + std::to_string(*lfo + 1);
        }
    }
    // The device's own channel strip first, then each LFO in order
    std::ranges::stable_sort(list, {}, [&lfoOf](auto && entry) { return lfoOf(entry.key) ? static_cast<int>(*lfoOf(entry.key)) : -1; });
    return list;
}

void LfoDevice::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);

    writer.writeStartElement(NahdXml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    // Only the LFOs that move something: an untouched device writes nothing here
    writer.writeStartElement(NahdXml::xmlKeyLfos());
    for (size_t lfo = 0; lfo < LfoCount; lfo++) {
        if (const auto & target = m_slots.at(lfo).target; !target.ownerId.empty()) {
            writer.writeStartElement(NahdXml::xmlKeyLfo());
            writer.writeAttribute(NahdXml::xmlKeyIndex(), QString::number(lfo));
            writer.writeAttribute(NahdXml::xmlKeyOwner(), ParameterAutomation::Target::ownerToString(target.owner));
            writer.writeAttribute(NahdXml::xmlKeyOwnerId(), QString::fromStdString(target.ownerId));
            writer.writeAttribute(NahdXml::xmlKeyParameterName(), QString::fromStdString(target.parameter));
            writer.writeEndElement();
        }
    }
    writer.writeEndElement();

    writer.writeEndElement();
}

void LfoDevice::deserializeFromXml(ProjectReader & reader)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        deserializeAttributesFromXml(reader);

        while (!reader.atEnd() && !reader.hasError()) {
            const auto token = reader.readNext();
            if (token == ProjectReader::TokenType::EndElement && reader.name() == NahdXml::xmlKeyDevice()) {
                break;
            }
            if (token == ProjectReader::TokenType::StartElement) {
                if (reader.name() == NahdXml::xmlKeyParameters()) {
                    deserializeParametersFromXml(reader);
                } else if (reader.name() == NahdXml::xmlKeyInsertEffects()) {
                    insertEffectRack().deserializeEffectsFromXml(reader);
                } else if (reader.name() == NahdXml::xmlKeyParameter()) {
                    deserializeParameter(reader);
                } else if (reader.name() == NahdXml::xmlKeyLfos()) {
                    while (reader.readNextStartElement()) {
                        if (reader.name() == NahdXml::xmlKeyLfo()) {
                            const auto index = Utils::Xml::readIntAttribute(reader, NahdXml::xmlKeyIndex(), false);
                            ParameterAutomation::Target target;
                            target.owner = ParameterAutomation::Target::ownerFromString(Utils::Xml::readStringAttribute(reader, NahdXml::xmlKeyOwner(), false).value_or(QString {}));
                            target.ownerId = Utils::Xml::readStringAttribute(reader, NahdXml::xmlKeyOwnerId(), false).value_or(QString {}).toStdString();
                            target.parameter = Utils::Xml::readStringAttribute(reader, NahdXml::xmlKeyParameterName(), false).value_or(QString {}).toStdString();
                            if (index && *index >= 0 && *index < static_cast<int>(LfoCount)) {
                                m_slots.at(static_cast<size_t>(*index)).target = std::move(target);
                            }
                        }
                        reader.skipCurrentElement();
                    }
                } else {
                    reader.skipCurrentElement();
                }
            }
        }

        syncParameters();
        for (auto && slot : m_slots) {
            slot.lfo.setPhase(slot.phase);
        }
    }
    emit dataChanged();
}

} // namespace noteahead
