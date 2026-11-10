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

#include "xy_pad_device.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../common/utils.hpp"
#include "../../common/xml/project_reader.hpp"
#include "../../common/xml/project_writer.hpp"
#include "../tracker/parameter_display.hpp"

#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace NahdXml = Constants::NahdXml;

namespace {

//! Smallest change worth a step: one step of the grid a project saves knob positions on.
constexpr double StepThreshold { 1.0e-4 };
//! Closer than this to where it is heading, a pad is there: an exponential never arrives.
constexpr double ArrivalThreshold { 1.0e-4 };
//! Where the return knob rests by default: a second, unhurried but not lazy.
constexpr double DefaultReturnSeconds { 1.0 };

const char * axisName(XyPadDevice::Axis axis)
{
    return axis == XyPadDevice::Axis::X ? "x" : "y";
}

float clampHand(float value)
{
    return std::isfinite(value) ? std::clamp(value, -1.0f, 1.0f) : 0.0f;
}

} // namespace

XyPadDevice::XyPadDevice(std::string name)
  : m_name { std::move(name) }
{
    for (size_t pad = 0; pad < PadCount; pad++) {
        addPadParameters(pad);
        describePadParameters(pad);
    }
    XyPadDevice::syncParameters();
}

XyPadDevice::~XyPadDevice() = default;

std::string XyPadDevice::name() const
{
    return m_name;
}

std::string XyPadDevice::category() const
{
    return NahdXml::xmlValueModulators().toStdString();
}

std::string XyPadDevice::typeName() const
{
    return Constants::xyPadDeviceName().toStdString();
}

std::string XyPadDevice::typeIdString()
{
    return "5b0f2c1e-7d43-4a8e-9c61-3e8f4a2d7b90";
}

std::string XyPadDevice::typeId() const
{
    return typeIdString();
}

size_t XyPadDevice::slot(size_t pad, Axis axis)
{
    return pad * AxisCount + static_cast<size_t>(axis);
}

std::string XyPadDevice::padKey(size_t pad, const QString & key)
{
    return "pad" + std::to_string(pad + 1) + "_" + key.toStdString();
}

std::string XyPadDevice::axisKey(size_t pad, Axis axis, const QString & key)
{
    return "pad" + std::to_string(pad + 1) + "_" + axisName(axis) + "_" + key.toStdString();
}

std::string XyPadDevice::positionKey(size_t pad, Axis axis)
{
    return axisKey(pad, axis, NahdXml::xmlKeyPadPosition());
}

void XyPadDevice::addPadParameters(size_t pad)
{
    const auto continuous = [this](const std::string & key, float value) {
        addParameter(Parameter { key, value, 0, 10000, static_cast<int>(std::lround(value * 10000.0f)), 100 });
    };
    continuous(padKey(pad, NahdXml::xmlKeyReturnTime()), static_cast<float>(ParameterMapper::unmapCubic(DefaultReturnSeconds, 0.0, MaxReturnSeconds)));
    // Full depth: the edges of a pad reach half the knob's travel either way of the setting
    continuous(axisKey(pad, Axis::X, NahdXml::xmlKeyIntensity()), 1.0f);
    continuous(axisKey(pad, Axis::Y, NahdXml::xmlKeyIntensity()), 1.0f);
    // At the centre, so that the pad rests where it would without it
    continuous(positionKey(pad, Axis::X), 0.5f);
    continuous(positionKey(pad, Axis::Y), 0.5f);
}

void XyPadDevice::describePadParameters(size_t pad)
{
    using Display = ParameterDisplay;
    setParameterDisplay(padKey(pad, NahdXml::xmlKeyReturnTime()), { .mapping = Display::Mapping::Cubic, .unit = "s", .min = 0.0, .max = MaxReturnSeconds, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Return") });
    setParameterDisplay(axisKey(pad, Axis::X, NahdXml::xmlKeyIntensity()), { .unit = "%", .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "X Depth") });
    setParameterDisplay(axisKey(pad, Axis::Y, NahdXml::xmlKeyIntensity()), { .unit = "%", .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Y Depth") });
    setParameterDisplay(positionKey(pad, Axis::X), { .mapping = Display::Mapping::Bipolar, .unit = "%", .min = -100.0, .max = 100.0, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "X") });
    setParameterDisplay(positionKey(pad, Axis::Y), { .mapping = Display::Mapping::Bipolar, .unit = "%", .min = -100.0, .max = 100.0, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Y") });
}

void XyPadDevice::syncParameters()
{
    Device::syncParameters();
    const auto value = [this](const std::string & key) {
        const auto p = parameter(key);
        return p ? p->get().value() : 0.0f;
    };
    for (size_t pad = 0; pad < PadCount; pad++) {
        auto & state = m_pads.at(pad);
        state.returnTime = value(padKey(pad, NahdXml::xmlKeyReturnTime()));
        state.axes.at(static_cast<size_t>(Axis::X)).depth = value(axisKey(pad, Axis::X, NahdXml::xmlKeyIntensity()));
        state.axes.at(static_cast<size_t>(Axis::Y)).depth = value(axisKey(pad, Axis::Y, NahdXml::xmlKeyIntensity()));
        // Only an automation or a modulation moves the pad: the value it rests at is the centre
        const auto position = [this](const std::string & key) -> std::optional<double> {
            if (const auto p = parameter(key); p && (p->get().isAutomated() || p->get().isModulated())) {
                return std::clamp(static_cast<double>(p->get().value()) * 2.0 - 1.0, -1.0, 1.0);
            }
            return std::nullopt;
        };
        state.automatedX = position(positionKey(pad, Axis::X));
        state.automatedY = position(positionKey(pad, Axis::Y));
    }
}

bool XyPadDevice::supportsOctave() const
{
    return false;
}

bool XyPadDevice::producesAudio() const
{
    return false;
}

void XyPadDevice::processDeviceMidiNoteOn(uint8_t, uint8_t)
{
}

void XyPadDevice::processDeviceMidiNoteOff(uint8_t)
{
}

void XyPadDevice::processDeviceMidiCc(uint8_t, uint8_t, uint8_t)
{
}

void XyPadDevice::processMidiAllNotesOff()
{
}

void XyPadDevice::processAudio(AudioContext & context)
{
    setSampleRate(context.sampleRate);
}

bool XyPadDevice::hasActiveAudio() const
{
    return false;
}

void XyPadDevice::reset()
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    Device::reset();
    resetAudio();
}

void XyPadDevice::resetDeviceAudio()
{
    // Back to the centre, as a render begins: no hand is on the pad in a render
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    for (auto && pad : m_pads) {
        pad.x = 0.0;
        pad.y = 0.0;
        pad.restX = 0.0;
        pad.restY = 0.0;
        pad.wasPressed = false;
        pad.returnStep.reset();
        pad.shownX.store(0.0f, std::memory_order_relaxed);
        pad.shownY.store(0.0f, std::memory_order_relaxed);
        pad.moving.store(false, std::memory_order_relaxed);
        pad.handActive.store(false, std::memory_order_relaxed);
        pad.goalX = 0.0;
        pad.goalY = 0.0;
        for (auto && axis : pad.axes) {
            axis.lastSent.reset();
        }
    }
    m_gridOrigin.reset();
}

Device::AutomatableParameterList XyPadDevice::automatableParameters() const
{
    const auto padOf = [](const std::string & key) -> std::optional<size_t> {
        for (size_t pad = 0; pad < PadCount; pad++) {
            if (key.starts_with("pad" + std::to_string(pad + 1) + "_")) {
                return pad;
            }
        }
        return std::nullopt;
    };
    auto list = Device::automatableParameters();
    for (auto && entry : list) {
        if (const auto pad = padOf(entry.key); pad) {
            entry.groupName = "Pad " + std::to_string(*pad + 1);
        }
    }
    // The device's own channel strip first, then each pad in order
    std::ranges::stable_sort(list, {}, [&padOf](auto && entry) { return padOf(entry.key) ? static_cast<int>(*padOf(entry.key)) : -1; });
    return list;
}

bool XyPadDevice::updateOwnParameter(const std::string & key, float value)
{
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        const auto p = parameter(key);
        if (!p) {
            return false;
        }
        p->get().setValue(value);
        syncParameters();
    }
    emit dataChanged();
    return true;
}

float XyPadDevice::ownParameterValue(const std::string & key) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    const auto p = parameter(key);
    return p ? p->get().shownValue() : 0.0f;
}

bool XyPadDevice::updatePadParameter(size_t pad, const QString & key, float value)
{
    return updateOwnParameter(padKey(pad, key), value);
}

float XyPadDevice::padParameterValue(size_t pad, const QString & key) const
{
    return ownParameterValue(padKey(pad, key));
}

bool XyPadDevice::updateAxisParameter(size_t pad, Axis axis, const QString & key, float value)
{
    return updateOwnParameter(axisKey(pad, axis, key), value);
}

float XyPadDevice::axisParameterValue(size_t pad, Axis axis, const QString & key) const
{
    return ownParameterValue(axisKey(pad, axis, key));
}

ParameterAutomation::Target XyPadDevice::target(size_t pad, Axis axis) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    return pad < PadCount ? m_pads.at(pad).axes.at(static_cast<size_t>(axis)).target : ParameterAutomation::Target {};
}

void XyPadDevice::setTarget(size_t pad, Axis axis, ParameterAutomation::Target target)
{
    if (pad >= PadCount) {
        return;
    }
    {
        const std::lock_guard<std::recursive_mutex> lock { mutex() };
        auto & axisSlot = m_pads.at(pad).axes.at(static_cast<size_t>(axis));
        if (axisSlot.target == target) {
            return;
        }
        // Let go of the old one at the next step, unless it never got anything
        if (!axisSlot.target.ownerId.empty() && axisSlot.lastSent) {
            axisSlot.released = axisSlot.target;
        }
        axisSlot.target = std::move(target);
        axisSlot.lastSent.reset();
    }
    emit dataChanged();
}

void XyPadDevice::press(size_t pad, float x, float y)
{
    if (pad < PadCount) {
        auto & state = m_pads.at(pad);
        state.handX.store(clampHand(x), std::memory_order_relaxed);
        state.handY.store(clampHand(y), std::memory_order_relaxed);
        // Released after the position, so the audio thread never sees a press without where it is
        state.pressed.store(true, std::memory_order_release);
        emit padPressed(static_cast<int>(pad));
    }
}

void XyPadDevice::move(size_t pad, float x, float y)
{
    if (pad < PadCount) {
        m_pads.at(pad).handX.store(clampHand(x), std::memory_order_relaxed);
        m_pads.at(pad).handY.store(clampHand(y), std::memory_order_relaxed);
    }
}

void XyPadDevice::release(size_t pad)
{
    if (pad < PadCount) {
        m_pads.at(pad).pressed.store(false, std::memory_order_release);
    }
}

float XyPadDevice::positionX(size_t pad) const
{
    return pad < PadCount ? m_pads.at(pad).shownX.load(std::memory_order_relaxed) : 0.0f;
}

float XyPadDevice::positionY(size_t pad) const
{
    return pad < PadCount ? m_pads.at(pad).shownY.load(std::memory_order_relaxed) : 0.0f;
}

bool XyPadDevice::isPressed(size_t pad) const
{
    return pad < PadCount && m_pads.at(pad).pressed.load(std::memory_order_acquire);
}

bool XyPadDevice::isMoving(size_t pad) const
{
    return pad < PadCount && m_pads.at(pad).moving.load(std::memory_order_relaxed);
}

bool XyPadDevice::isHandActive(size_t pad) const
{
    return pad < PadCount && (m_pads.at(pad).pressed.load(std::memory_order_acquire) || m_pads.at(pad).handActive.load(std::memory_order_acquire));
}

void XyPadDevice::evaluate(uint64_t startFrame, uint32_t frameCount, double sampleRate, double, std::vector<Step> & steps)
{
    if (sampleRate <= 0.0 || !frameCount) {
        return;
    }
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    const auto gridFrames = std::max<uint64_t>(1, static_cast<uint64_t>(std::llround(sampleRate / ControlRateHz)));
    const double controlRate = sampleRate / static_cast<double>(gridFrames);
    const double slewCoefficient = std::exp(-1.0 / (SlewSeconds * controlRate));

    for (size_t pad = 0; pad < PadCount; pad++) {
        for (size_t axis = 0; axis < AxisCount; axis++) {
            if (auto & axisSlot = m_pads.at(pad).axes.at(axis); axisSlot.released) {
                steps.push_back({ slot(pad, static_cast<Axis>(axis)), startFrame, *axisSlot.released, std::nullopt, 0.0, 0.0 });
                axisSlot.released.reset();
            }
        }
    }

    if (!m_gridOrigin || *m_gridOrigin > startFrame) {
        m_gridOrigin = startFrame;
    }
    const uint64_t end = startFrame + frameCount;
    const uint64_t sinceOrigin = startFrame - *m_gridOrigin;
    for (uint64_t frame = *m_gridOrigin + (sinceOrigin + gridFrames - 1) / gridFrames * gridFrames; frame < end; frame += gridFrames) {
        for (size_t pad = 0; pad < PadCount; pad++) {
            auto & state = m_pads.at(pad);
            const bool pressed = state.pressed.load(std::memory_order_acquire);
            const double returnSeconds = ParameterMapper::mapCubic(static_cast<double>(state.returnTime), 0.0, MaxReturnSeconds);
            if (pressed) {
                // Where the hand leaves it is where a held pad stays
                state.restX = static_cast<double>(state.handX.load(std::memory_order_relaxed));
                state.restY = static_cast<double>(state.handY.load(std::memory_order_relaxed));
                state.returnStep.reset();
            } else if (state.wasPressed && returnSeconds > 0.0) {
                state.returnFromX = state.x;
                state.returnFromY = state.y;
                state.returnStep = 0;
                state.restX = 0.0;
                state.restY = 0.0;
            }
            state.wasPressed = pressed;

            if (state.returnStep) {
                // A half cosine from where the hand let go to the centre: it eases off the hand and
                // settles without a bump, and is home exactly when the return time says
                const double progress = static_cast<double>(++*state.returnStep) / std::max(1.0, returnSeconds * controlRate);
                const double remaining = progress >= 1.0 ? 0.0 : 0.5 * (1.0 + std::cos(std::numbers::pi * progress));
                state.x = state.returnFromX * remaining;
                state.y = state.returnFromY * remaining;
                if (progress >= 1.0) {
                    state.returnStep.reset();
                }
            } else {
                // An automation of the pad moves it only once the hand is done with it
                state.goalX = !pressed && state.automatedX ? *state.automatedX : state.restX;
                state.goalY = !pressed && state.automatedY ? *state.automatedY : state.restY;
                state.x = state.goalX + slewCoefficient * (state.x - state.goalX);
                state.y = state.goalY + slewCoefficient * (state.y - state.goalY);
                if (std::abs(state.x - state.goalX) < ArrivalThreshold) {
                    state.x = state.goalX;
                }
                if (std::abs(state.y - state.goalY) < ArrivalThreshold) {
                    state.y = state.goalY;
                }
            }

            for (size_t axis = 0; axis < AxisCount; axis++) {
                auto & axisSlot = state.axes.at(axis);
                if (axisSlot.target.ownerId.empty() || axisSlot.target.parameter.empty()) {
                    continue;
                }
                const double source = axis == static_cast<size_t>(Axis::X) ? state.x : state.y;
                const double depth = static_cast<double>(axisSlot.depth);
                if (source == 0.0 && !pressed) {
                    // At rest it moves nothing, so it lets go rather than sit there as an offset of zero
                    if (axisSlot.lastSent) {
                        steps.push_back({ slot(pad, static_cast<Axis>(axis)), frame, axisSlot.target, std::nullopt, 0.0, 0.0 });
                        axisSlot.lastSent.reset();
                    }
                    continue;
                }
                const bool changed = !axisSlot.lastSent || std::abs(source - axisSlot.lastSent->first) >= StepThreshold || std::abs(depth - axisSlot.lastSent->second) >= StepThreshold;
                if (changed) {
                    steps.push_back({ slot(pad, static_cast<Axis>(axis)), frame, axisSlot.target, ParameterModulation::Mode::Bipolar, source, depth });
                    axisSlot.lastSent = std::pair { source, depth };
                }
            }
        }
    }

    for (auto && state : m_pads) {
        state.shownX.store(static_cast<float>(state.x), std::memory_order_relaxed);
        state.shownY.store(static_cast<float>(state.y), std::memory_order_relaxed);
        state.moving.store(state.returnStep.has_value() || state.x != state.goalX || state.y != state.goalY, std::memory_order_relaxed);
        state.handActive.store(state.wasPressed || state.returnStep.has_value(), std::memory_order_release);
    }
}

std::vector<ParameterAutomation::Target> XyPadDevice::modulationTargets() const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    std::vector<ParameterAutomation::Target> targets;
    for (auto && pad : m_pads) {
        for (auto && axis : pad.axes) {
            if (!axis.target.ownerId.empty()) {
                targets.push_back(axis.target);
            }
        }
    }
    return targets;
}

void XyPadDevice::serializeToXml(ProjectWriter & writer) const
{
    const std::lock_guard<std::recursive_mutex> lock { mutex() };
    writer.writeStartElement(NahdXml::xmlKeyDevice());
    serializeAttributesToXml(writer);
    serializeMidiMappingsToXml(writer);

    writer.writeStartElement(NahdXml::xmlKeyInsertEffects());
    insertEffectRack().serializeEffectsToXml(writer);
    writer.writeEndElement();

    writer.writeStartElement(NahdXml::xmlKeyParameters());
    serializeParametersToXml(writer);
    writer.writeEndElement();

    // Only the axes that move something: an untouched device writes nothing here
    writer.writeStartElement(NahdXml::xmlKeyPads());
    for (size_t pad = 0; pad < PadCount; pad++) {
        for (size_t axis = 0; axis < AxisCount; axis++) {
            if (const auto & target = m_pads.at(pad).axes.at(axis).target; !target.ownerId.empty()) {
                writer.writeStartElement(NahdXml::xmlKeyPad());
                writer.writeAttribute(NahdXml::xmlKeyIndex(), QString::number(pad));
                writer.writeAttribute(NahdXml::xmlKeyAxis(), QString::number(axis));
                writer.writeAttribute(NahdXml::xmlKeyOwner(), ParameterAutomation::Target::ownerToString(target.owner));
                writer.writeAttribute(NahdXml::xmlKeyOwnerId(), QString::fromStdString(target.ownerId));
                writer.writeAttribute(NahdXml::xmlKeyParameterName(), QString::fromStdString(target.parameter));
                writer.writeEndElement();
            }
        }
    }
    writer.writeEndElement();

    writer.writeEndElement();
}

void XyPadDevice::deserializeFromXml(ProjectReader & reader)
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
                } else if (reader.name() == NahdXml::xmlKeyMidiMappings()) {
                    deserializeMidiMappingsFromXml(reader);
                } else if (reader.name() == NahdXml::xmlKeyInsertEffects()) {
                    insertEffectRack().deserializeEffectsFromXml(reader);
                } else if (reader.name() == NahdXml::xmlKeyParameter()) {
                    deserializeParameter(reader);
                } else if (reader.name() == NahdXml::xmlKeyPads()) {
                    while (reader.readNextStartElement()) {
                        if (reader.name() == NahdXml::xmlKeyPad()) {
                            const auto pad = Utils::Xml::readIntAttribute(reader, NahdXml::xmlKeyIndex(), false);
                            const auto axis = Utils::Xml::readIntAttribute(reader, NahdXml::xmlKeyAxis(), false);
                            ParameterAutomation::Target target;
                            target.owner = ParameterAutomation::Target::ownerFromString(Utils::Xml::readStringAttribute(reader, NahdXml::xmlKeyOwner(), false).value_or(QString {}));
                            target.ownerId = Utils::Xml::readStringAttribute(reader, NahdXml::xmlKeyOwnerId(), false).value_or(QString {}).toStdString();
                            target.parameter = Utils::Xml::readStringAttribute(reader, NahdXml::xmlKeyParameterName(), false).value_or(QString {}).toStdString();
                            if (pad && *pad >= 0 && *pad < static_cast<int>(PadCount) && axis && *axis >= 0 && *axis < static_cast<int>(AxisCount)) {
                                m_pads.at(static_cast<size_t>(*pad)).axes.at(static_cast<size_t>(*axis)).target = std::move(target);
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
    }
    emit dataChanged();
}

} // namespace noteahead
