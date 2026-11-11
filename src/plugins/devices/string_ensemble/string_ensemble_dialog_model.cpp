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

#include "string_ensemble_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "string_ensemble_device.hpp"

#include <cmath>

namespace noteahead {

StringEnsembleDialogModel::StringEnsembleDialogModel(std::shared_ptr<StringEnsembleDevice> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

StringEnsembleDialogModel::~StringEnsembleDialogModel() = default;

DeviceDialogModel::DeviceS StringEnsembleDialogModel::device() const
{
    return m_device;
}

bool StringEnsembleDialogModel::setDevice(DeviceS device)
{
    if (const auto stringEnsemble = std::dynamic_pointer_cast<StringEnsembleDevice>(device)) {
        setDevice(stringEnsemble);
        return true;
    }
    return false;
}

bool StringEnsembleDialogModel::contrabassEnabled() const
{
    return m_device ? m_device->contrabassEnabled() : false;
}

void StringEnsembleDialogModel::setContrabassEnabled(bool value)
{
    if (m_device) {
        m_device->setContrabassEnabled(value);
    }
}

bool StringEnsembleDialogModel::celloEnabled() const
{
    return m_device ? m_device->celloEnabled() : false;
}

void StringEnsembleDialogModel::setCelloEnabled(bool value)
{
    if (m_device) {
        m_device->setCelloEnabled(value);
    }
}

bool StringEnsembleDialogModel::violaEnabled() const
{
    return m_device ? m_device->violaEnabled() : false;
}

void StringEnsembleDialogModel::setViolaEnabled(bool value)
{
    if (m_device) {
        m_device->setViolaEnabled(value);
    }
}

bool StringEnsembleDialogModel::violinEnabled() const
{
    return m_device ? m_device->violinEnabled() : false;
}

void StringEnsembleDialogModel::setViolinEnabled(bool value)
{
    if (m_device) {
        m_device->setViolinEnabled(value);
    }
}

bool StringEnsembleDialogModel::trumpetEnabled() const
{
    return m_device ? m_device->trumpetEnabled() : false;
}

void StringEnsembleDialogModel::setTrumpetEnabled(bool value)
{
    if (m_device) {
        m_device->setTrumpetEnabled(value);
    }
}

bool StringEnsembleDialogModel::hornEnabled() const
{
    return m_device ? m_device->hornEnabled() : false;
}

void StringEnsembleDialogModel::setHornEnabled(bool value)
{
    if (m_device) {
        m_device->setHornEnabled(value);
    }
}

bool StringEnsembleDialogModel::modulationEnabled() const
{
    return m_device ? m_device->modulationEnabled() : false;
}

void StringEnsembleDialogModel::setModulationEnabled(bool value)
{
    if (m_device) {
        m_device->setModulationEnabled(value);
    }
}

bool StringEnsembleDialogModel::phaserEnabled() const
{
    return m_device ? m_device->phaserEnabled() : false;
}

void StringEnsembleDialogModel::setPhaserEnabled(bool value)
{
    if (m_device) {
        m_device->setPhaserEnabled(value);
    }
}

int StringEnsembleDialogModel::volumeBass() const
{
    return m_device ? static_cast<int>(std::round(m_device->volumeBass() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleDialogModel::setVolumeBass(int value)
{
    if (m_device) {
        m_device->setVolumeBass(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleDialogModel::crescendo() const
{
    return m_device ? static_cast<int>(std::round(m_device->crescendo() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleDialogModel::setCrescendo(int value)
{
    if (m_device) {
        m_device->setCrescendo(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleDialogModel::sustainLength() const
{
    return m_device ? static_cast<int>(std::round(m_device->sustainLength() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleDialogModel::setSustainLength(int value)
{
    if (m_device) {
        m_device->setSustainLength(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleDialogModel::phaserColor() const
{
    return m_device ? static_cast<int>(std::round(m_device->phaserColor() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleDialogModel::setPhaserColor(int value)
{
    if (m_device) {
        m_device->setPhaserColor(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleDialogModel::phaserRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->phaserRate() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleDialogModel::setPhaserRate(int value)
{
    if (m_device) {
        m_device->setPhaserRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleDialogModel::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleDialogModel::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleDialogModel::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleDialogModel::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int StringEnsembleDialogModel::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void StringEnsembleDialogModel::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void StringEnsembleDialogModel::requestSettings()
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

void StringEnsembleDialogModel::setDevice(std::shared_ptr<StringEnsembleDevice> device)
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
