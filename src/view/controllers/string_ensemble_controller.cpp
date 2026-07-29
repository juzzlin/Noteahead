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

#include "string_ensemble_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/string_ensemble_device.hpp"

#include <cmath>

namespace noteahead {

StringEnsembleController::StringEnsembleController(std::shared_ptr<StringEnsembleDevice> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

StringEnsembleController::~StringEnsembleController() = default;

DeviceController::DeviceS StringEnsembleController::device() const
{
    return m_device;
}

bool StringEnsembleController::setDevice(DeviceS device)
{
    if (const auto stringEnsemble = std::dynamic_pointer_cast<StringEnsembleDevice>(device)) {
        setDevice(stringEnsemble);
        return true;
    }
    return false;
}

bool StringEnsembleController::contrabassEnabled() const
{
    return m_device ? m_device->contrabassEnabled() : false;
}

void StringEnsembleController::setContrabassEnabled(bool value)
{
    if (m_device) {
        m_device->setContrabassEnabled(value);
    }
}

bool StringEnsembleController::celloEnabled() const
{
    return m_device ? m_device->celloEnabled() : false;
}

void StringEnsembleController::setCelloEnabled(bool value)
{
    if (m_device) {
        m_device->setCelloEnabled(value);
    }
}

bool StringEnsembleController::violaEnabled() const
{
    return m_device ? m_device->violaEnabled() : false;
}

void StringEnsembleController::setViolaEnabled(bool value)
{
    if (m_device) {
        m_device->setViolaEnabled(value);
    }
}

bool StringEnsembleController::violinEnabled() const
{
    return m_device ? m_device->violinEnabled() : false;
}

void StringEnsembleController::setViolinEnabled(bool value)
{
    if (m_device) {
        m_device->setViolinEnabled(value);
    }
}

bool StringEnsembleController::trumpetEnabled() const
{
    return m_device ? m_device->trumpetEnabled() : false;
}

void StringEnsembleController::setTrumpetEnabled(bool value)
{
    if (m_device) {
        m_device->setTrumpetEnabled(value);
    }
}

bool StringEnsembleController::hornEnabled() const
{
    return m_device ? m_device->hornEnabled() : false;
}

void StringEnsembleController::setHornEnabled(bool value)
{
    if (m_device) {
        m_device->setHornEnabled(value);
    }
}

bool StringEnsembleController::modulationEnabled() const
{
    return m_device ? m_device->modulationEnabled() : false;
}

void StringEnsembleController::setModulationEnabled(bool value)
{
    if (m_device) {
        m_device->setModulationEnabled(value);
    }
}

bool StringEnsembleController::phaserEnabled() const
{
    return m_device ? m_device->phaserEnabled() : false;
}

void StringEnsembleController::setPhaserEnabled(bool value)
{
    if (m_device) {
        m_device->setPhaserEnabled(value);
    }
}

int StringEnsembleController::volumeBass() const
{
    return m_device ? static_cast<int>(std::round(m_device->volumeBass() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleController::setVolumeBass(int value)
{
    if (m_device) {
        m_device->setVolumeBass(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleController::crescendo() const
{
    return m_device ? static_cast<int>(std::round(m_device->crescendo() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleController::setCrescendo(int value)
{
    if (m_device) {
        m_device->setCrescendo(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleController::sustainLength() const
{
    return m_device ? static_cast<int>(std::round(m_device->sustainLength() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleController::setSustainLength(int value)
{
    if (m_device) {
        m_device->setSustainLength(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleController::phaserColor() const
{
    return m_device ? static_cast<int>(std::round(m_device->phaserColor() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleController::setPhaserColor(int value)
{
    if (m_device) {
        m_device->setPhaserColor(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleController::phaserRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->phaserRate() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleController::setPhaserRate(int value)
{
    if (m_device) {
        m_device->setPhaserRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleController::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleController::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleController::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleController::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleController::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleController::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void StringEnsembleController::requestSettings()
{
    emit contrabassEnabledChanged();
    emit celloEnabledChanged();
    emit violaEnabledChanged();
    emit violinEnabledChanged();
    emit trumpetEnabledChanged();
    emit hornEnabledChanged();
    emit modulationEnabledChanged();
    emit phaserEnabledChanged();
    emit volumeBassChanged();
    emit crescendoChanged();
    emit sustainLengthChanged();
    emit phaserColorChanged();
    emit phaserRateChanged();
    emit velocitySensitivityChanged();
    emit lpfCutoffChanged();
    emit hpfCutoffChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

void StringEnsembleController::setDevice(std::shared_ptr<StringEnsembleDevice> device)
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
