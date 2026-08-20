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

#include "piano_synth_v2_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/piano_synth_v2_device.hpp"

#include <cmath>

namespace noteahead {

PianoSynthV2Controller::PianoSynthV2Controller(std::shared_ptr<PianoSynthV2Device> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

PianoSynthV2Controller::~PianoSynthV2Controller() = default;

DeviceController::DeviceS PianoSynthV2Controller::device() const
{
    return m_device;
}

bool PianoSynthV2Controller::setDevice(DeviceS device)
{
    if (const auto piano = std::dynamic_pointer_cast<PianoSynthV2Device>(device)) {
        setDevice(piano);
        return true;
    }
    return false;
}

int PianoSynthV2Controller::brightness() const
{
    return m_device ? static_cast<int>(std::round(m_device->brightness() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV2Controller::setBrightness(int value)
{
    if (m_device) {
        m_device->setBrightness(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV2Controller::decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->decay() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV2Controller::setDecay(int value)
{
    if (m_device) {
        m_device->setDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV2Controller::inharmonicity() const
{
    return m_device ? static_cast<int>(std::round(m_device->inharmonicity() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV2Controller::setInharmonicity(int value)
{
    if (m_device) {
        m_device->setInharmonicity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV2Controller::hammerHardness() const
{
    return m_device ? static_cast<int>(std::round(m_device->hammerHardness() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV2Controller::setHammerHardness(int value)
{
    if (m_device) {
        m_device->setHammerHardness(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV2Controller::stringDetune() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringDetune() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV2Controller::setStringDetune(int value)
{
    if (m_device) {
        m_device->setStringDetune(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV2Controller::stretch() const
{
    return m_device ? static_cast<int>(std::round(m_device->stretch() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV2Controller::setStretch(int value)
{
    if (m_device) {
        m_device->setStretch(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV2Controller::richness() const
{
    return m_device ? static_cast<int>(std::round(m_device->richness() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV2Controller::setRichness(int value)
{
    if (m_device) {
        m_device->setRichness(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV2Controller::doubleDecay() const
{
    return m_device ? static_cast<int>(std::round(m_device->doubleDecay() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV2Controller::setDoubleDecay(int value)
{
    if (m_device) {
        m_device->setDoubleDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV2Controller::attack() const
{
    return m_device ? static_cast<int>(std::round(m_device->attack() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV2Controller::setAttack(int value)
{
    if (m_device) {
        m_device->setAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV2Controller::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV2Controller::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV2Controller::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV2Controller::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV2Controller::releaseTime() const
{
    return m_device ? static_cast<int>(std::round(m_device->releaseTime() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV2Controller::setReleaseTime(int value)
{
    if (m_device) {
        m_device->setReleaseTime(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthV2Controller::stereoWidth() const
{
    return m_device ? static_cast<int>(std::round(m_device->stereoWidth() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthV2Controller::setStereoWidth(int value)
{
    if (m_device) {
        m_device->setStereoWidth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void PianoSynthV2Controller::requestSettings()
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
    emit lpfCutoffChanged();
    emit hpfCutoffChanged();
    emit releaseTimeChanged();
    emit stereoWidthChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

void PianoSynthV2Controller::setDevice(std::shared_ptr<PianoSynthV2Device> device)
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
