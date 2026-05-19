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

#include <cmath>

namespace noteahead {

BassSynthController::BassSynthController(std::shared_ptr<BassSynthDevice> device, QObject * parent)
  : QObject { parent }
  , m_device { std::move(device) }
{
    if (m_device) {
        connect(m_device.get(), &Device::dataChanged, this, &BassSynthController::sampleRateChanged);
    }
}

BassSynthController::~BassSynthController() = default;

std::shared_ptr<BassSynthDevice> BassSynthController::device() const
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
        emit waveformChanged();
    }
}

int BassSynthController::tuning() const
{
    return m_device ? static_cast<int>(std::round(m_device->tuning() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setTuning(int t)
{
    if (m_device) {
        m_device->setTuning(static_cast<float>(t) / Constants::uiInternalScaling());
        emit tuningChanged();
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
        emit subLevelChanged();
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
        emit subOctaveChanged();
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
        emit lpfCutoffChanged();
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
        emit lpfResonanceChanged();
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
        emit hpfCutoffChanged();
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
        emit envModChanged();
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
        emit decayChanged();
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
        emit accentChanged();
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
        emit slideChanged();
    }
}

int BassSynthController::volume() const
{
    return m_device ? static_cast<int>(std::round(m_device->volume() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setVolume(int v)
{
    if (m_device) {
        m_device->setVolume(static_cast<float>(v) / Constants::uiInternalScaling());
        emit volumeChanged();
    }
}

int BassSynthController::gain() const
{
    return m_device ? static_cast<int>(std::round(m_device->gain() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setGain(int g)
{
    if (m_device) {
        m_device->setGain(static_cast<float>(g) / Constants::uiInternalScaling());
        emit gainChanged();
    }
}

int BassSynthController::pan() const
{
    return m_device ? static_cast<int>(std::round(m_device->pan() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setPan(int p)
{
    if (m_device) {
        m_device->setPan(static_cast<float>(p) / Constants::uiInternalScaling());
        emit panChanged();
    }
}

uint32_t BassSynthController::sampleRate() const
{
    return m_device ? m_device->sampleRate() : static_cast<uint32_t>(Constants::defaultSampleRate());
}

float BassSynthController::cutoffToHz(float cutoff) const
{
    return Utils::Dsp::cutoffToHz(cutoff / Constants::uiInternalScaling(), static_cast<float>(sampleRate()));
}

int BassSynthController::distDrive() const
{
    return m_device ? static_cast<int>(std::round(m_device->distDrive() * Constants::uiInternalScaling())) : 0;
}

void BassSynthController::setDistDrive(int d)
{
    if (m_device) {
        m_device->setDistDrive(static_cast<float>(d) / Constants::uiInternalScaling());
        emit distDriveChanged();
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
        emit distToneChanged();
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
        emit distLevelChanged();
    }
}

void BassSynthController::reset()
{
    if (m_device) {
        m_device->reset();
    }
    requestSettings();
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
    emit distDriveChanged();
    emit distToneChanged();
    emit distLevelChanged();
}

void BassSynthController::accept()
{
}

void BassSynthController::reject()
{
}

void BassSynthController::playNote(int note, double velocity)
{
    if (m_device) {
        m_device->processMidiNoteOn(note, static_cast<uint8_t>(velocity * 127.0));
    }
}

void BassSynthController::stopNote(int note)
{
    if (m_device) {
        m_device->processMidiNoteOff(note);
    }
}

void BassSynthController::setDevice(std::shared_ptr<BassSynthDevice> device)
{
    if (m_device != device) {
        m_device = std::move(device);
        if (m_device) {
            connect(m_device.get(), &Device::dataChanged, this, &BassSynthController::sampleRateChanged);
        }
        emit deviceChanged();
        requestSettings();
    }
}

} // namespace noteahead
