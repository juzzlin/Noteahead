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

#include "piano_synth_v3_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/piano_synth_v3_device.hpp"

#include <cmath>

namespace noteahead {

PianoSynthV3Controller::PianoSynthV3Controller(std::shared_ptr<PianoSynthV3Device> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

PianoSynthV3Controller::~PianoSynthV3Controller() = default;

DeviceController::DeviceS PianoSynthV3Controller::device() const
{
    return m_device;
}

bool PianoSynthV3Controller::setDevice(DeviceS device)
{
    if (const auto piano = std::dynamic_pointer_cast<PianoSynthV3Device>(device)) {
        setDevice(piano);
        return true;
    }
    return false;
}

int PianoSynthV3Controller::brightness() const
{
    return m_device ? static_cast<int>(std::round(m_device->brightness() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setBrightness(int value)
{
    if (m_device) {
        m_device->setBrightness(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV3Controller::decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->decay() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setDecay(int value)
{
    if (m_device) {
        m_device->setDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV3Controller::inharmonicity() const
{
    return m_device ? static_cast<int>(std::round(m_device->inharmonicity() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setInharmonicity(int value)
{
    if (m_device) {
        m_device->setInharmonicity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV3Controller::hammerHardness() const
{
    return m_device ? static_cast<int>(std::round(m_device->hammerHardness() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setHammerHardness(int value)
{
    if (m_device) {
        m_device->setHammerHardness(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV3Controller::stringDetune() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringDetune() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setStringDetune(int value)
{
    if (m_device) {
        m_device->setStringDetune(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV3Controller::stretch() const
{
    return m_device ? static_cast<int>(std::round(m_device->stretch() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setStretch(int value)
{
    if (m_device) {
        m_device->setStretch(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV3Controller::richness() const
{
    return m_device ? static_cast<int>(std::round(m_device->richness() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setRichness(int value)
{
    if (m_device) {
        m_device->setRichness(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV3Controller::doubleDecay() const
{
    return m_device ? static_cast<int>(std::round(m_device->doubleDecay() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setDoubleDecay(int value)
{
    if (m_device) {
        m_device->setDoubleDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV3Controller::attack() const
{
    return m_device ? static_cast<int>(std::round(m_device->attack() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setAttack(int value)
{
    if (m_device) {
        m_device->setAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV3Controller::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV3Controller::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV3Controller::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV3Controller::releaseTime() const
{
    return m_device ? static_cast<int>(std::round(m_device->releaseTime() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setReleaseTime(int value)
{
    if (m_device) {
        m_device->setReleaseTime(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV3Controller::stereoWidth() const
{
    return m_device ? static_cast<int>(std::round(m_device->stereoWidth() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV3Controller::setStereoWidth(int value)
{
    if (m_device) {
        m_device->setStereoWidth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void PianoSynthV3Controller::requestSettings()
{
    emit brightnessChanged();
    emit decayChanged();
    emit inharmonicityChanged();
    emit hammerHardnessChanged();
    emit stringDetuneChanged();
    emit stretchChanged();
    emit richnessChanged();
    emit doubleDecayChanged();
    emit attackChanged();
    emit velocitySensitivityChanged();
    emit lpfCutoffChanged();
    emit hpfCutoffChanged();
    emit releaseTimeChanged();
    emit stereoWidthChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

void PianoSynthV3Controller::setDevice(std::shared_ptr<PianoSynthV3Device> device)
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
