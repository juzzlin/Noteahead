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

#include "snare_synth_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/snare_synth_device.hpp"
#include "../../domain/devices/snare_synth_presets.hpp"

#include <cmath>

namespace noteahead {

SnareSynthController::SnareSynthController(std::shared_ptr<SnareSynthDevice> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

SnareSynthController::~SnareSynthController() = default;

DeviceController::DeviceS SnareSynthController::device() const
{
    return m_device;
}

bool SnareSynthController::setDevice(DeviceController::DeviceS device)
{
    if (const auto snare = std::dynamic_pointer_cast<SnareSynthDevice>(device)) {
        setDevice(snare);
        return true;
    }
    return false;
}

void SnareSynthController::setDevice(std::shared_ptr<SnareSynthDevice> device)
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

void SnareSynthController::connectDeviceSignals()
{
    if (!m_device) {
        return;
    }

    connect(m_device.get(), &Device::parametersChanged, this, [this]() {
        requestSettings();
    });

    connect(m_device.get(), &Device::dataChanged, this, [this]() {
        requestSettings();
    });
}

QStringList SnareSynthController::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : SnareSynthPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void SnareSynthController::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int SnareSynthController::tune() const
{
    return m_device ? static_cast<int>(std::round(m_device->tune() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setTune(int value)
{
    if (m_device) {
        m_device->setTune(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::tone() const
{
    return m_device ? static_cast<int>(std::round(m_device->tone() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setTone(int value)
{
    if (m_device) {
        m_device->setTone(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->decay() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setDecay(int value)
{
    if (m_device) {
        m_device->setDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::headTension() const
{
    return m_device ? static_cast<int>(std::round(m_device->headTension() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setHeadTension(int value)
{
    if (m_device) {
        m_device->setHeadTension(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::pitchDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchDepth() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setPitchDepth(int value)
{
    if (m_device) {
        m_device->setPitchDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::pitchDecay() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchDecay() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setPitchDecay(int value)
{
    if (m_device) {
        m_device->setPitchDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::snappy() const
{
    return m_device ? static_cast<int>(std::round(m_device->snappy() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setSnappy(int value)
{
    if (m_device) {
        m_device->setSnappy(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::snareDecay() const
{
    return m_device ? static_cast<int>(std::round(m_device->snareDecay() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setSnareDecay(int value)
{
    if (m_device) {
        m_device->setSnareDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::snareTension() const
{
    return m_device ? static_cast<int>(std::round(m_device->snareTension() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setSnareTension(int value)
{
    if (m_device) {
        m_device->setSnareTension(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::snareBuzz() const
{
    return m_device ? static_cast<int>(std::round(m_device->snareBuzz() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setSnareBuzz(int value)
{
    if (m_device) {
        m_device->setSnareBuzz(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::click() const
{
    return m_device ? static_cast<int>(std::round(m_device->click() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setClick(int value)
{
    if (m_device) {
        m_device->setClick(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::hardness() const
{
    return m_device ? static_cast<int>(std::round(m_device->hardness() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setHardness(int value)
{
    if (m_device) {
        m_device->setHardness(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::position() const
{
    return m_device ? static_cast<int>(std::round(m_device->position() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setPosition(int value)
{
    if (m_device) {
        m_device->setPosition(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::body() const
{
    return m_device ? static_cast<int>(std::round(m_device->body() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setBody(int value)
{
    if (m_device) {
        m_device->setBody(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::drive() const
{
    return m_device ? static_cast<int>(std::round(m_device->drive() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setDrive(int value)
{
    if (m_device) {
        m_device->setDrive(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::panSpread() const
{
    return m_device ? static_cast<int>(std::round(m_device->panSpread() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setPanSpread(int value)
{
    if (m_device) {
        m_device->setPanSpread(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool SnareSynthController::chromatic() const
{
    return m_device ? m_device->chromatic() : false;
}

void SnareSynthController::setChromatic(bool value)
{
    if (m_device) {
        m_device->setChromatic(value);
    }
}

int SnareSynthController::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void SnareSynthController::requestSettings()
{
    emit tuneChanged();
    emit toneChanged();
    emit decayChanged();
    emit headTensionChanged();
    emit pitchDepthChanged();
    emit pitchDecayChanged();

    emit snappyChanged();
    emit snareDecayChanged();
    emit snareTensionChanged();
    emit snareBuzzChanged();

    emit clickChanged();
    emit hardnessChanged();
    emit positionChanged();
    emit bodyChanged();
    emit driveChanged();
    emit panSpreadChanged();

    emit chromaticChanged();
    emit velocitySensitivityChanged();

    emit lpfCutoffChanged();
    emit hpfCutoffChanged();
}

} // namespace noteahead
