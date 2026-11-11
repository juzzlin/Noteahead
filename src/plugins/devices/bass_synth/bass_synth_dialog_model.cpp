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

#include "bass_synth_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/utils.hpp"
#include "bass_synth_device.hpp"
#include "bass_synth_presets.hpp"

#include <QDebug>
#include <cmath>

namespace noteahead {

BassSynthDialogModel::BassSynthDialogModel(std::shared_ptr<BassSynthDevice> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

BassSynthDialogModel::~BassSynthDialogModel() = default;

DeviceDialogModel::DeviceS BassSynthDialogModel::device() const
{
    return m_device;
}

bool BassSynthDialogModel::setDevice(DeviceS device)
{
    if (const auto bassSynth = std::dynamic_pointer_cast<BassSynthDevice>(device)) {
        setDevice(bassSynth);
        return true;
    }
    return false;
}

std::shared_ptr<BassSynthDevice> BassSynthDialogModel::bassSynthDevice() const
{
    return m_device;
}

// Oscillator
int BassSynthDialogModel::waveform() const
{
    return m_device ? static_cast<int>(m_device->waveform()) : 0;
}

void BassSynthDialogModel::setWaveform(int wave)
{
    if (m_device) {
        m_device->setWaveform(static_cast<PolyBlepOscillator::Waveform>(wave));
    }
}

QStringList BassSynthDialogModel::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : BassSynthPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void BassSynthDialogModel::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

QStringList BassSynthDialogModel::vcoWaveformNames() const
{
    QStringList list;
    for (auto && name : PolyBlepOscillator::waveformNames()) {
        list << QString::fromStdString(name);
    }
    return list;
}

int BassSynthDialogModel::tuning() const
{
    return m_device ? static_cast<int>(std::round(m_device->tuning() * Constants::uiInternalScaling())) : 0;
}

void BassSynthDialogModel::setTuning(int t)
{
    if (m_device) {
        m_device->setTuning(static_cast<float>(t) / Constants::uiInternalScaling());
    }
}

int BassSynthDialogModel::subLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->subLevel() * Constants::uiInternalScaling())) : 0;
}

void BassSynthDialogModel::setSubLevel(int level)
{
    if (m_device) {
        m_device->setSubLevel(static_cast<float>(level) / Constants::uiInternalScaling());
    }
}

int BassSynthDialogModel::subOctave() const
{
    return m_device ? m_device->subOctave() : 1;
}

void BassSynthDialogModel::setSubOctave(int oct)
{
    if (m_device) {
        m_device->setSubOctave(oct);
    }
}

// Filter
int BassSynthDialogModel::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void BassSynthDialogModel::setLpfCutoff(int c)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(c) / Constants::uiInternalScaling());
    }
}

int BassSynthDialogModel::lpfResonance() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfResonance() * Constants::uiInternalScaling())) : 0;
}

void BassSynthDialogModel::setLpfResonance(int r)
{
    if (m_device) {
        m_device->setLpfResonance(static_cast<float>(r) / Constants::uiInternalScaling());
    }
}

int BassSynthDialogModel::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void BassSynthDialogModel::setHpfCutoff(int c)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(c) / Constants::uiInternalScaling());
    }
}

int BassSynthDialogModel::envMod() const
{
    return m_device ? static_cast<int>(std::round(m_device->envMod() * Constants::uiInternalScaling())) : 0;
}

void BassSynthDialogModel::setEnvMod(int m)
{
    if (m_device) {
        m_device->setEnvMod(static_cast<float>(m) / Constants::uiInternalScaling());
    }
}

int BassSynthDialogModel::decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->decay() * Constants::uiInternalScaling())) : 0;
}

void BassSynthDialogModel::setDecay(int d)
{
    if (m_device) {
        m_device->setDecay(static_cast<float>(d) / Constants::uiInternalScaling());
    }
}

// Global / Modifiers
int BassSynthDialogModel::accent() const
{
    return m_device ? static_cast<int>(std::round(m_device->accent() * Constants::uiInternalScaling())) : 0;
}

void BassSynthDialogModel::setAccent(int a)
{
    if (m_device) {
        m_device->setAccent(static_cast<float>(a) / Constants::uiInternalScaling());
    }
}

int BassSynthDialogModel::slide() const
{
    return m_device ? static_cast<int>(std::round(m_device->slide() * Constants::uiInternalScaling())) : 0;
}

void BassSynthDialogModel::setSlide(int s)
{
    if (m_device) {
        m_device->setSlide(static_cast<float>(s) / Constants::uiInternalScaling());
    }
}

int BassSynthDialogModel::pitchBendRange() const
{
    return m_device ? m_device->pitchBendRange() : 0;
}

void BassSynthDialogModel::setPitchBendRange(int r)
{
    if (m_device) {
        m_device->setPitchBendRange(r);
    }
}

int BassSynthDialogModel::distDrive() const
{
    return m_device ? static_cast<int>(std::round(m_device->distDrive() * Constants::uiInternalScaling())) : 0;
}

void BassSynthDialogModel::setDistDrive(int d)
{
    if (m_device) {
        m_device->setDistDrive(static_cast<float>(d) / Constants::uiInternalScaling());
    }
}

int BassSynthDialogModel::distTone() const
{
    return m_device ? static_cast<int>(std::round(m_device->distTone() * Constants::uiInternalScaling())) : 0;
}

void BassSynthDialogModel::setDistTone(int t)
{
    if (m_device) {
        m_device->setDistTone(static_cast<float>(t) / Constants::uiInternalScaling());
    }
}

int BassSynthDialogModel::distLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->distLevel() * Constants::uiInternalScaling())) : 0;
}

void BassSynthDialogModel::setDistLevel(int l)
{
    if (m_device) {
        m_device->setDistLevel(static_cast<float>(l) / Constants::uiInternalScaling());
    }
}

void BassSynthDialogModel::requestSettings()
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
    emit pitchBendRangeChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
    emit distDriveChanged();
    emit distToneChanged();
    emit distLevelChanged();
}

void BassSynthDialogModel::setDevice(std::shared_ptr<BassSynthDevice> device)
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
