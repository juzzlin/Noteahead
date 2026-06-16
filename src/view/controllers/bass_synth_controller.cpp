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

#include "bass_synth_controller.hpp"

#include "../../common/constants.hpp"
#include "../../common/utils.hpp"
#include "../../domain/devices/bass_synth_device.hpp"

#include <QDebug>
#include <cmath>

namespace noteahead {

BassSynthController::BassSynthController(std::shared_ptr<BassSynthDevice> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

BassSynthController::~BassSynthController() = default;

DeviceController::DeviceS BassSynthController::device() const
{
    return m_device;
}

bool BassSynthController::setDevice(DeviceS device)
{
    if (const auto bassSynth = std::dynamic_pointer_cast<BassSynthDevice>(device)) {
        setDevice(bassSynth);
        return true;
    }
    return false;
}

std::shared_ptr<BassSynthDevice> BassSynthController::bassSynthDevice() const
{
    return m_device;
}

// Oscillator
int BassSynthController::waveform() const
{
    return m_device ? static_cast<int>(m_device->waveform()) : 0;
}

void BassSynthController::setWaveform(int wave)
{
    if (m_device) {
        m_device->setWaveform(static_cast<PolyBlepOscillator::Waveform>(wave));
    }
}

QStringList BassSynthController::vcoWaveformNames() const
{
    QStringList list;
    for (auto && name : PolyBlepOscillator::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

int BassSynthController::tuning() const
{
    return m_device ? static_cast<int>(std::round(m_device->tuning() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setTuning(int t)
{
    if (m_device) {
        m_device->setTuning(static_cast<float>(t) / Constants::uiInternalScaling());
    }
}

int BassSynthController::subLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->subLevel() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setSubLevel(int level)
{
    if (m_device) {
        m_device->setSubLevel(static_cast<float>(level) / Constants::uiInternalScaling());
    }
}

int BassSynthController::subOctave() const
{
    return m_device ? m_device->subOctave() : 1;
}

void BassSynthController::setSubOctave(int oct)
{
    if (m_device) {
        m_device->setSubOctave(oct);
    }
}

// Filter
int BassSynthController::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setLpfCutoff(int c)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(c) / Constants::uiInternalScaling());
    }
}

int BassSynthController::lpfResonance() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfResonance() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setLpfResonance(int r)
{
    if (m_device) {
        m_device->setLpfResonance(static_cast<float>(r) / Constants::uiInternalScaling());
    }
}

int BassSynthController::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setHpfCutoff(int c)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(c) / Constants::uiInternalScaling());
    }
}

int BassSynthController::envMod() const
{
    return m_device ? static_cast<int>(std::round(m_device->envMod() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setEnvMod(int m)
{
    if (m_device) {
        m_device->setEnvMod(static_cast<float>(m) / Constants::uiInternalScaling());
    }
}

int BassSynthController::decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->decay() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setDecay(int d)
{
    if (m_device) {
        m_device->setDecay(static_cast<float>(d) / Constants::uiInternalScaling());
    }
}

// Global / Modifiers
int BassSynthController::accent() const
{
    return m_device ? static_cast<int>(std::round(m_device->accent() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setAccent(int a)
{
    if (m_device) {
        m_device->setAccent(static_cast<float>(a) / Constants::uiInternalScaling());
    }
}

int BassSynthController::slide() const
{
    return m_device ? static_cast<int>(std::round(m_device->slide() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setSlide(int s)
{
    if (m_device) {
        m_device->setSlide(static_cast<float>(s) / Constants::uiInternalScaling());
    }
}

int BassSynthController::distDrive() const
{
    return m_device ? static_cast<int>(std::round(m_device->distDrive() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setDistDrive(int d)
{
    if (m_device) {
        m_device->setDistDrive(static_cast<float>(d) / Constants::uiInternalScaling());
    }
}

int BassSynthController::distTone() const
{
    return m_device ? static_cast<int>(std::round(m_device->distTone() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setDistTone(int t)
{
    if (m_device) {
        m_device->setDistTone(static_cast<float>(t) / Constants::uiInternalScaling());
    }
}

int BassSynthController::distLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->distLevel() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setDistLevel(int l)
{
    if (m_device) {
        m_device->setDistLevel(static_cast<float>(l) / Constants::uiInternalScaling());
    }
}

void BassSynthController::requestSettings()
{
    emit waveformChanged();
    emit tuningChanged();
    emit subLevelChanged();
    emit subOctaveChanged();
    emit lpfCutoffChanged();
    emit lpfResonanceChanged();
    emit hpfCutoffChanged();
    emit envModChanged();
    emit decayChanged();
    emit accentChanged();
    emit slideChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
    emit distDriveChanged();
    emit distToneChanged();
    emit distLevelChanged();
}

void BassSynthController::setDevice(std::shared_ptr<BassSynthDevice> device)
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
