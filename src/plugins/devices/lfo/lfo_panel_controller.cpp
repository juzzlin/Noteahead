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

#include "lfo_panel_controller.hpp"

#include "../../../application/service/device_service.hpp"
#include "../../../common/constants.hpp"
#include "lfo_controller.hpp"
#include "lfo_device.hpp"

#include <cmath>

namespace noteahead {

namespace NahdXml = Constants::NahdXml;

LfoPanelController::LfoPanelController(size_t index, LfoController & owner)
  : QObject { &owner }
  , m_index { index }
  , m_owner { owner }
{
}

int LfoPanelController::index() const
{
    return static_cast<int>(m_index);
}

int LfoPanelController::knob(const QString & key) const
{
    const auto device = m_owner.lfoDevice();
    return device ? static_cast<int>(std::round(device->lfoParameterValue(m_index, key) * Constants::uiInternalScaling())) : 0;
}

void LfoPanelController::setKnob(const QString & key, int value)
{
    if (const auto device = m_owner.lfoDevice(); device) {
        device->updateLfoParameter(m_index, key, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int LfoPanelController::choice(const QString & key) const
{
    const auto device = m_owner.lfoDevice();
    return device ? static_cast<int>(std::lround(device->lfoParameterValue(m_index, key))) : 0;
}

void LfoPanelController::setChoice(const QString & key, int value)
{
    if (const auto device = m_owner.lfoDevice(); device) {
        device->updateLfoParameter(m_index, key, static_cast<float>(value));
    }
}

int LfoPanelController::waveform() const
{
    return choice(NahdXml::xmlKeyWaveform());
}

void LfoPanelController::setWaveform(int value)
{
    setChoice(NahdXml::xmlKeyWaveform(), value);
}

int LfoPanelController::mode() const
{
    return choice(NahdXml::xmlKeyMode());
}

void LfoPanelController::setMode(int value)
{
    setChoice(NahdXml::xmlKeyMode(), value);
}

int LfoPanelController::rate() const
{
    return knob(NahdXml::xmlKeyRate());
}

void LfoPanelController::setRate(int value)
{
    setKnob(NahdXml::xmlKeyRate(), value);
}

int LfoPanelController::rateDivider() const
{
    return choice(NahdXml::xmlKeyRateDivider());
}

void LfoPanelController::setRateDivider(int value)
{
    setChoice(NahdXml::xmlKeyRateDivider(), value);
}

int LfoPanelController::depth() const
{
    return knob(NahdXml::xmlKeyIntensity());
}

void LfoPanelController::setDepth(int value)
{
    setKnob(NahdXml::xmlKeyIntensity(), value);
}

int LfoPanelController::modulationMode() const
{
    return choice(NahdXml::xmlKeyModulationMode());
}

void LfoPanelController::setModulationMode(int value)
{
    setChoice(NahdXml::xmlKeyModulationMode(), value);
}

int LfoPanelController::delay() const
{
    return knob(NahdXml::xmlKeyDelay());
}

void LfoPanelController::setDelay(int value)
{
    setKnob(NahdXml::xmlKeyDelay(), value);
}

int LfoPanelController::fade() const
{
    return knob(NahdXml::xmlKeyFade());
}

void LfoPanelController::setFade(int value)
{
    setKnob(NahdXml::xmlKeyFade(), value);
}

int LfoPanelController::phase() const
{
    return knob(NahdXml::xmlKeyPhase());
}

void LfoPanelController::setPhase(int value)
{
    setKnob(NahdXml::xmlKeyPhase(), value);
}

int LfoPanelController::smoothing() const
{
    return knob(NahdXml::xmlKeySmoothing());
}

void LfoPanelController::setSmoothing(int value)
{
    setKnob(NahdXml::xmlKeySmoothing(), value);
}

bool LfoPanelController::retrigger() const
{
    return choice(NahdXml::xmlKeyRetrigger()) != 0;
}

void LfoPanelController::setRetrigger(bool value)
{
    setChoice(NahdXml::xmlKeyRetrigger(), value ? 1 : 0);
}

int LfoPanelController::velocitySensitivity() const
{
    return knob(NahdXml::xmlKeyVelocitySensitivity());
}

void LfoPanelController::setVelocitySensitivity(int value)
{
    setKnob(NahdXml::xmlKeyVelocitySensitivity(), value);
}

QString LfoPanelController::targetOwner() const
{
    const auto device = m_owner.lfoDevice();
    if (!device || device->target(m_index).ownerId.empty()) {
        return {};
    }
    return ParameterAutomation::Target::ownerToString(device->target(m_index).owner);
}

QString LfoPanelController::targetOwnerId() const
{
    const auto device = m_owner.lfoDevice();
    return device ? QString::fromStdString(device->target(m_index).ownerId) : QString {};
}

QString LfoPanelController::targetParameter() const
{
    const auto device = m_owner.lfoDevice();
    return device ? QString::fromStdString(device->target(m_index).parameter) : QString {};
}

bool LfoPanelController::targetShared() const
{
    const auto device = m_owner.lfoDevice();
    return device && m_owner.deviceService() && m_owner.deviceService()->modulationSourceCount(device->target(m_index)) > 1;
}

bool LfoPanelController::pitchBend() const
{
    const auto device = m_owner.lfoDevice();
    const auto address = device ? device->target(m_index).midiAddress() : std::nullopt;
    return address && address->message == ParameterAutomation::Target::MidiMessage::PitchBend;
}

int LfoPanelController::midiMin() const
{
    const auto device = m_owner.lfoDevice();
    if (!device) {
        return 0;
    }
    const auto target = device->target(m_index);
    return target.displayedMidiValue(target.midiMin);
}

void LfoPanelController::setMidiMin(int value)
{
    if (const auto device = m_owner.lfoDevice(); device) {
        auto target = device->target(m_index);
        target.setMidiRange(target.midiValueFromDisplayed(value), target.midiMax);
        device->setTarget(m_index, std::move(target));
    }
}

int LfoPanelController::midiMax() const
{
    const auto device = m_owner.lfoDevice();
    if (!device) {
        return 127;
    }
    const auto target = device->target(m_index);
    return target.displayedMidiValue(target.midiMax);
}

void LfoPanelController::setMidiMax(int value)
{
    if (const auto device = m_owner.lfoDevice(); device) {
        auto target = device->target(m_index);
        target.setMidiRange(target.midiMin, target.midiValueFromDisplayed(value));
        device->setTarget(m_index, std::move(target));
    }
}

void LfoPanelController::setTarget(const QString & owner, const QString & ownerId, const QString & parameter)
{
    if (const auto device = m_owner.lfoDevice(); device) {
        device->setTarget(m_index, ParameterAutomation::Target { ParameterAutomation::Target::ownerFromString(owner), ownerId.toStdString(), parameter.toStdString() }.withRangeOf(device->target(m_index)));
    }
}

void LfoPanelController::chooseTargetOwner(const QString & owner, const QString & ownerId, const QStringList & parameters)
{
    const auto device = m_owner.lfoDevice();
    if (!device) {
        return;
    }
    const auto ownerType = ParameterAutomation::Target::ownerFromString(owner);
    std::vector<std::string> candidates;
    for (auto && parameter : parameters) {
        candidates.push_back(parameter.toStdString());
    }
    std::vector<std::string> keys;
    if (const auto & deviceService = m_owner.deviceService(); deviceService) {
        keys = deviceService->suggestedModulationParameters(ownerType, ownerId.toStdString(), candidates, 1, { device->target(m_index) });
    } else if (!candidates.empty()) {
        keys = { candidates.front() };
    }
    device->setTarget(m_index, { ownerType, ownerId.toStdString(), keys.empty() ? std::string {} : keys.front() });
}

void LfoPanelController::clearTarget()
{
    if (const auto device = m_owner.lfoDevice(); device) {
        device->setTarget(m_index, {});
    }
}

} // namespace noteahead
