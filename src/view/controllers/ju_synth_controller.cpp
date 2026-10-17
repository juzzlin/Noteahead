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

#include "ju_synth_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/ju_synth_device.hpp"
#include "../../domain/devices/ju_synth_presets.hpp"

#include <cmath>

namespace noteahead {

JuSynthController::JuSynthController(std::shared_ptr<JuSynthDevice> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

JuSynthController::~JuSynthController() = default;

DeviceController::DeviceS JuSynthController::device() const
{
    return m_device;
}

bool JuSynthController::setDevice(DeviceS device)
{
    if (const auto juSynth = std::dynamic_pointer_cast<JuSynthDevice>(device)) {
        setDevice(juSynth);
        return true;
    }
    return false;
}

void JuSynthController::setDevice(std::shared_ptr<JuSynthDevice> device)
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

QStringList JuSynthController::factoryPresetNames() const
{
    QStringList names;
    for (auto && preset : JuSynthPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void JuSynthController::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int JuSynthController::lfoRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->lfoRate() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setLfoRate(int value)
{
    if (m_device) {
        m_device->setLfoRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::lfoDelay() const
{
    return m_device ? static_cast<int>(std::round(m_device->lfoDelay() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setLfoDelay(int value)
{
    if (m_device) {
        m_device->setLfoDelay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::pitchLfoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchLfoDepth() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setPitchLfoDepth(int value)
{
    if (m_device) {
        m_device->setPitchLfoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::pulseWidth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pulseWidth() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setPulseWidth(int value)
{
    if (m_device) {
        m_device->setPulseWidth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::subLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->subLevel() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setSubLevel(int value)
{
    if (m_device) {
        m_device->setSubLevel(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::noiseLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->noiseLevel() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setNoiseLevel(int value)
{
    if (m_device) {
        m_device->setNoiseLevel(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::lpfResonance() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfResonance() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setLpfResonance(int value)
{
    if (m_device) {
        m_device->setLpfResonance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::envMod() const
{
    return m_device ? static_cast<int>(std::round(m_device->envMod() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setEnvMod(int value)
{
    if (m_device) {
        m_device->setEnvMod(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::filterLfoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->filterLfoDepth() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setFilterLfoDepth(int value)
{
    if (m_device) {
        m_device->setFilterLfoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::keyTrack() const
{
    return m_device ? static_cast<int>(std::round(m_device->keyTrack() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setKeyTrack(int value)
{
    if (m_device) {
        m_device->setKeyTrack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::attack() const
{
    return m_device ? static_cast<int>(std::round(m_device->attack() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setAttack(int value)
{
    if (m_device) {
        m_device->setAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->decay() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setDecay(int value)
{
    if (m_device) {
        m_device->setDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::sustain() const
{
    return m_device ? static_cast<int>(std::round(m_device->sustain() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setSustain(int value)
{
    if (m_device) {
        m_device->setSustain(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::release() const
{
    return m_device ? static_cast<int>(std::round(m_device->release() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setRelease(int value)
{
    if (m_device) {
        m_device->setRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::portamento() const
{
    return m_device ? static_cast<int>(std::round(m_device->portamento() * Constants::uiInternalScaling())) : 0;
}

void JuSynthController::setPortamento(int value)
{
    if (m_device) {
        m_device->setPortamento(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthController::range() const
{
    return m_device ? static_cast<int>(m_device->range()) : 0;
}

void JuSynthController::setRange(int value)
{
    if (m_device) {
        m_device->setRange(value);
        emit rangeChanged();
    }
}

int JuSynthController::pwmSource() const
{
    return m_device ? static_cast<int>(m_device->pwmSource()) : 0;
}

void JuSynthController::setPwmSource(int value)
{
    if (m_device) {
        m_device->setPwmSource(static_cast<JuSynthDevice::PwmSource>(value));
        emit pwmSourceChanged();
    }
}

int JuSynthController::vcaMode() const
{
    return m_device ? static_cast<int>(m_device->vcaMode()) : 0;
}

void JuSynthController::setVcaMode(int value)
{
    if (m_device) {
        m_device->setVcaMode(static_cast<JuSynthDevice::VcaMode>(value));
        emit vcaModeChanged();
    }
}

int JuSynthController::chorusMode() const
{
    return m_device ? static_cast<int>(m_device->chorusMode()) : 0;
}

void JuSynthController::setChorusMode(int value)
{
    if (m_device) {
        m_device->setChorusMode(static_cast<JunoChorus::Mode>(value));
        emit chorusModeChanged();
    }
}

int JuSynthController::pitchBendRange() const
{
    return m_device ? static_cast<int>(m_device->pitchBendRange()) : 0;
}

void JuSynthController::setPitchBendRange(int value)
{
    if (m_device) {
        m_device->setPitchBendRange(value);
        emit pitchBendRangeChanged();
    }
}

bool JuSynthController::sawEnabled() const
{
    return m_device && m_device->sawEnabled();
}

void JuSynthController::setSawEnabled(bool value)
{
    if (m_device) {
        m_device->setSawEnabled(value);
        emit sawEnabledChanged();
    }
}

bool JuSynthController::pulseEnabled() const
{
    return m_device && m_device->pulseEnabled();
}

void JuSynthController::setPulseEnabled(bool value)
{
    if (m_device) {
        m_device->setPulseEnabled(value);
        emit pulseEnabledChanged();
    }
}

bool JuSynthController::envInverted() const
{
    return m_device && m_device->envInverted();
}

void JuSynthController::setEnvInverted(bool value)
{
    if (m_device) {
        m_device->setEnvInverted(value);
        emit envInvertedChanged();
    }
}

void JuSynthController::requestSettings()
{
    emit lfoRateChanged();
    emit lfoDelayChanged();
    emit pitchLfoDepthChanged();
    emit pulseWidthChanged();
    emit subLevelChanged();
    emit noiseLevelChanged();
    emit hpfCutoffChanged();
    emit lpfCutoffChanged();
    emit lpfResonanceChanged();
    emit envModChanged();
    emit filterLfoDepthChanged();
    emit keyTrackChanged();
    emit velocitySensitivityChanged();
    emit attackChanged();
    emit decayChanged();
    emit sustainChanged();
    emit releaseChanged();
    emit portamentoChanged();
    emit rangeChanged();
    emit pwmSourceChanged();
    emit vcaModeChanged();
    emit chorusModeChanged();
    emit pitchBendRangeChanged();
    emit sawEnabledChanged();
    emit pulseEnabledChanged();
    emit envInvertedChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

} // namespace noteahead
