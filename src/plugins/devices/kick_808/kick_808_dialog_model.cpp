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

#include "kick_808_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "kick_808_device.hpp"

#include <cmath>

namespace noteahead {

Kick808DialogModel::Kick808DialogModel(std::shared_ptr<Kick808Device> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

Kick808DialogModel::~Kick808DialogModel() = default;

DeviceDialogModel::DeviceS Kick808DialogModel::device() const
{
    return m_device;
}

bool Kick808DialogModel::setDevice(DeviceS device)
{
    if (const auto kick = std::dynamic_pointer_cast<Kick808Device>(device)) {
        setDevice(kick);
        return true;
    }
    return false;
}

int Kick808DialogModel::tune() const
{
    return m_device ? static_cast<int>(std::round(m_device->tune() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setTune(int value)
{
    if (m_device) {
        m_device->setTune(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::tone() const
{
    return m_device ? static_cast<int>(std::round(m_device->tone() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setTone(int value)
{
    if (m_device) {
        m_device->setTone(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->decay() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setDecay(int value)
{
    if (m_device) {
        m_device->setDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::pitchDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchDepth() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setPitchDepth(int value)
{
    if (m_device) {
        m_device->setPitchDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::pitchDecay() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchDecay() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setPitchDecay(int value)
{
    if (m_device) {
        m_device->setPitchDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::drive() const
{
    return m_device ? static_cast<int>(std::round(m_device->drive() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setDrive(int value)
{
    if (m_device) {
        m_device->setDrive(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::glide() const
{
    return m_device ? static_cast<int>(std::round(m_device->glide() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setGlide(int value)
{
    if (m_device) {
        m_device->setGlide(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool Kick808DialogModel::keyTrack() const
{
    return m_device ? m_device->keyTrack() : false;
}

void Kick808DialogModel::setKeyTrack(bool value)
{
    if (m_device) {
        m_device->setKeyTrack(value);
    }
}

int Kick808DialogModel::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void Kick808DialogModel::requestSettings()
{
    emit tuneChanged();
    emit toneChanged();
    emit decayChanged();
    emit pitchDepthChanged();
    emit pitchDecayChanged();
    emit driveChanged();
    emit glideChanged();
    emit keyTrackChanged();
    emit lpfCutoffChanged();
    emit hpfCutoffChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

void Kick808DialogModel::setDevice(std::shared_ptr<Kick808Device> device)
{
    if (m_device != device) {
        if (m_device) {
            disconnect(m_device.get(), nullptr, this, nullptr);
        }
        m_device = std::move(device);
        connectDeviceSignals();
        emit deviceChanged();
        requestSettings();
    }
}

} // namespace noteahead
