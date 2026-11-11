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

#include "xy_pad_panel_controller.hpp"

#include "../../../application/service/device_service.hpp"
#include "../../../common/constants.hpp"
#include "xy_pad_controller.hpp"
#include "xy_pad_device.hpp"

#include <cmath>

namespace noteahead {

namespace NahdXml = Constants::NahdXml;

namespace {

XyPadDevice::Axis toAxis(int axis)
{
    return axis == 1 ? XyPadDevice::Axis::Y : XyPadDevice::Axis::X;
}

} // namespace

XyPadPanelController::XyPadPanelController(size_t index, XyPadController & owner)
  : QObject { &owner }
  , m_index { index }
  , m_owner { owner }
{
}

int XyPadPanelController::index() const
{
    return static_cast<int>(m_index);
}

int XyPadPanelController::returnTime() const
{
    const auto device = m_owner.xyPadDevice();
    return device ? static_cast<int>(std::round(device->padParameterValue(m_index, NahdXml::xmlKeyReturnTime()) * Constants::uiInternalScaling())) : 0;
}

void XyPadPanelController::setReturnTime(int value)
{
    if (const auto device = m_owner.xyPadDevice(); device) {
        device->updatePadParameter(m_index, NahdXml::xmlKeyReturnTime(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int XyPadPanelController::depth(int axis) const
{
    const auto device = m_owner.xyPadDevice();
    return device ? static_cast<int>(std::round(device->axisParameterValue(m_index, toAxis(axis), NahdXml::xmlKeyIntensity()) * Constants::uiInternalScaling())) : 0;
}

void XyPadPanelController::setDepth(int axis, int value)
{
    if (const auto device = m_owner.xyPadDevice(); device) {
        device->updateAxisParameter(m_index, toAxis(axis), NahdXml::xmlKeyIntensity(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int XyPadPanelController::xDepth() const
{
    return depth(0);
}

void XyPadPanelController::setXDepth(int value)
{
    setDepth(0, value);
}

int XyPadPanelController::yDepth() const
{
    return depth(1);
}

void XyPadPanelController::setYDepth(int value)
{
    setDepth(1, value);
}

QString XyPadPanelController::targetOwner(int axis) const
{
    const auto device = m_owner.xyPadDevice();
    if (!device) {
        return {};
    }
    const auto target = device->target(m_index, toAxis(axis));
    return target.ownerId.empty() ? QString {} : ParameterAutomation::Target::ownerToString(target.owner);
}

QString XyPadPanelController::targetOwnerId(int axis) const
{
    const auto device = m_owner.xyPadDevice();
    return device ? QString::fromStdString(device->target(m_index, toAxis(axis)).ownerId) : QString {};
}

QString XyPadPanelController::targetParameter(int axis) const
{
    const auto device = m_owner.xyPadDevice();
    return device ? QString::fromStdString(device->target(m_index, toAxis(axis)).parameter) : QString {};
}

bool XyPadPanelController::targetShared(int axis) const
{
    const auto device = m_owner.xyPadDevice();
    return device && m_owner.deviceService() && m_owner.deviceService()->modulationSourceCount(device->target(m_index, toAxis(axis))) > 1;
}

QString XyPadPanelController::xTargetOwner() const
{
    return targetOwner(0);
}

QString XyPadPanelController::xTargetOwnerId() const
{
    return targetOwnerId(0);
}

QString XyPadPanelController::xTargetParameter() const
{
    return targetParameter(0);
}

QString XyPadPanelController::yTargetOwner() const
{
    return targetOwner(1);
}

QString XyPadPanelController::yTargetOwnerId() const
{
    return targetOwnerId(1);
}

QString XyPadPanelController::yTargetParameter() const
{
    return targetParameter(1);
}

bool XyPadPanelController::xTargetShared() const
{
    return targetShared(0);
}

bool XyPadPanelController::yTargetShared() const
{
    return targetShared(1);
}

int XyPadPanelController::midiEnd(int axis, bool top) const
{
    const auto device = m_owner.xyPadDevice();
    if (!device) {
        return top ? 127 : 0;
    }
    const auto target = device->target(m_index, toAxis(axis));
    return target.displayedMidiValue(top ? target.midiMax : target.midiMin);
}

bool XyPadPanelController::isPitchBend(int axis) const
{
    const auto device = m_owner.xyPadDevice();
    const auto address = device ? device->target(m_index, toAxis(axis)).midiAddress() : std::nullopt;
    return address && address->message == ParameterAutomation::Target::MidiMessage::PitchBend;
}

bool XyPadPanelController::xPitchBend() const
{
    return isPitchBend(0);
}

bool XyPadPanelController::yPitchBend() const
{
    return isPitchBend(1);
}

void XyPadPanelController::setMidiEnd(int axis, bool top, int value)
{
    if (const auto device = m_owner.xyPadDevice(); device) {
        auto target = device->target(m_index, toAxis(axis));
        const auto end = target.midiValueFromDisplayed(value);
        target.setMidiRange(top ? target.midiMin : end, top ? end : target.midiMax);
        device->setTarget(m_index, toAxis(axis), std::move(target));
    }
}

int XyPadPanelController::xMidiMin() const
{
    return midiEnd(0, false);
}

void XyPadPanelController::setXMidiMin(int value)
{
    setMidiEnd(0, false, value);
}

int XyPadPanelController::xMidiMax() const
{
    return midiEnd(0, true);
}

void XyPadPanelController::setXMidiMax(int value)
{
    setMidiEnd(0, true, value);
}

int XyPadPanelController::yMidiMin() const
{
    return midiEnd(1, false);
}

void XyPadPanelController::setYMidiMin(int value)
{
    setMidiEnd(1, false, value);
}

int XyPadPanelController::yMidiMax() const
{
    return midiEnd(1, true);
}

void XyPadPanelController::setYMidiMax(int value)
{
    setMidiEnd(1, true, value);
}

void XyPadPanelController::setTarget(int axis, const QString & owner, const QString & ownerId, const QString & parameter)
{
    if (const auto device = m_owner.xyPadDevice(); device) {
        device->setTarget(m_index, toAxis(axis), ParameterAutomation::Target { ParameterAutomation::Target::ownerFromString(owner), ownerId.toStdString(), parameter.toStdString() }.withRangeOf(device->target(m_index, toAxis(axis))));
    }
}

void XyPadPanelController::chooseTargetOwner(int axis, const QString & owner, const QString & ownerId, const QStringList & parameters)
{
    const auto device = m_owner.xyPadDevice();
    if (!device) {
        return;
    }
    const auto chosenAxis = toAxis(axis);
    const auto otherAxis = chosenAxis == XyPadDevice::Axis::X ? XyPadDevice::Axis::Y : XyPadDevice::Axis::X;
    const bool fillOther = device->target(m_index, otherAxis).ownerId.empty();
    const auto ownerType = ParameterAutomation::Target::ownerFromString(owner);
    std::vector<std::string> candidates;
    for (auto && parameter : parameters) {
        candidates.push_back(parameter.toStdString());
    }
    std::vector<std::string> keys;
    if (const auto & deviceService = m_owner.deviceService(); deviceService) {
        // The axis being re-aimed lets go of what it had, so that is free for it to take again
        keys = deviceService->suggestedModulationParameters(ownerType, ownerId.toStdString(), candidates, fillOther ? 2 : 1, { device->target(m_index, chosenAxis) });
    } else if (!candidates.empty()) {
        keys = { candidates.front() };
    }
    if (keys.empty()) {
        device->setTarget(m_index, chosenAxis, { ownerType, ownerId.toStdString(), {} });
        return;
    }
    device->setTarget(m_index, chosenAxis, { ownerType, ownerId.toStdString(), keys.at(0) });
    if (fillOther && keys.size() > 1) {
        device->setTarget(m_index, otherAxis, { ownerType, ownerId.toStdString(), keys.at(1) });
    }
}

void XyPadPanelController::clearTarget(int axis)
{
    if (const auto device = m_owner.xyPadDevice(); device) {
        device->setTarget(m_index, toAxis(axis), {});
    }
}

} // namespace noteahead
