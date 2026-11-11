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

#include "piano_synth_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "piano_synth_device.hpp"

#include <cmath>

namespace noteahead {

PianoSynthDialogModel::PianoSynthDialogModel(std::shared_ptr<PianoSynthDevice> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

PianoSynthDialogModel::~PianoSynthDialogModel() = default;

DeviceDialogModel::DeviceS PianoSynthDialogModel::device() const
{
    return m_device;
}

bool PianoSynthDialogModel::setDevice(DeviceS device)
{
    if (const auto pianoSynth = std::dynamic_pointer_cast<PianoSynthDevice>(device)) {
        setDevice(pianoSynth);
        return true;
    }
    return false;
}

int PianoSynthDialogModel::brightness() const
{
    return m_device ? static_cast<int>(std::round(m_device->brightness() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthDialogModel::setBrightness(int value)
{
    if (m_device) {
        m_device->setBrightness(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthDialogModel::decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->decay() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthDialogModel::setDecay(int value)
{
    if (m_device) {
        m_device->setDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthDialogModel::inharmonicity() const
{
    return m_device ? static_cast<int>(std::round(m_device->inharmonicity() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthDialogModel::setInharmonicity(int value)
{
    if (m_device) {
        m_device->setInharmonicity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthDialogModel::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthDialogModel::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthDialogModel::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthDialogModel::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthDialogModel::releaseTime() const
{
    return m_device ? static_cast<int>(std::round(m_device->releaseTime() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthDialogModel::setReleaseTime(int value)
{
    if (m_device) {
        m_device->setReleaseTime(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthDialogModel::stereoWidth() const
{
    return m_device ? static_cast<int>(std::round(m_device->stereoWidth() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthDialogModel::setStereoWidth(int value)
{
    if (m_device) {
        m_device->setStereoWidth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthDialogModel::hammerHardness() const
{
    return m_device ? static_cast<int>(std::round(m_device->hammerHardness() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthDialogModel::setHammerHardness(int value)
{
    if (m_device) {
        m_device->setHammerHardness(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthDialogModel::stringDetune() const
{
    return m_device ? static_cast<int>(std::round(m_device->stringDetune() * Constants::uiInternalScaling())) : 0;
}

void PianoSynthDialogModel::setStringDetune(int value)
{
    if (m_device) {
        m_device->setStringDetune(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void PianoSynthDialogModel::requestSettings()
{
    emit brightnessChanged();
    emit decayChanged();
    emit inharmonicityChanged();
    emit lpfCutoffChanged();
    emit hpfCutoffChanged();
    emit releaseTimeChanged();
    emit stereoWidthChanged();
    emit hammerHardnessChanged();
    emit stringDetuneChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

void PianoSynthDialogModel::setDevice(std::shared_ptr<PianoSynthDevice> device)
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
