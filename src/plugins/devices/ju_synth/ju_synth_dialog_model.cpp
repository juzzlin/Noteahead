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

#include "ju_synth_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "ju_synth_device.hpp"
#include "ju_synth_presets.hpp"

#include <cmath>

namespace noteahead {

JuSynthDialogModel::JuSynthDialogModel(std::shared_ptr<JuSynthDevice> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

JuSynthDialogModel::~JuSynthDialogModel() = default;

DeviceDialogModel::DeviceS JuSynthDialogModel::device() const
{
    return m_device;
}

bool JuSynthDialogModel::setDevice(DeviceS device)
{
    if (const auto juSynth = std::dynamic_pointer_cast<JuSynthDevice>(device)) {
        setDevice(juSynth);
        return true;
    }
    return false;
}

void JuSynthDialogModel::setDevice(std::shared_ptr<JuSynthDevice> device)
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

QStringList JuSynthDialogModel::factoryPresetNames() const
{
    QStringList names;
    for (auto && preset : JuSynthPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void JuSynthDialogModel::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int JuSynthDialogModel::lfoRate() const
{
    return m_device ? static_cast<int>(std::round(m_device->lfoRate() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setLfoRate(int value)
{
    if (m_device) {
        m_device->setLfoRate(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::lfoDelay() const
{
    return m_device ? static_cast<int>(std::round(m_device->lfoDelay() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setLfoDelay(int value)
{
    if (m_device) {
        m_device->setLfoDelay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::pitchLfoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchLfoDepth() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setPitchLfoDepth(int value)
{
    if (m_device) {
        m_device->setPitchLfoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::pulseWidth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pulseWidth() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setPulseWidth(int value)
{
    if (m_device) {
        m_device->setPulseWidth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::subLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->subLevel() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setSubLevel(int value)
{
    if (m_device) {
        m_device->setSubLevel(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::noiseLevel() const
{
    return m_device ? static_cast<int>(std::round(m_device->noiseLevel() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setNoiseLevel(int value)
{
    if (m_device) {
        m_device->setNoiseLevel(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::lpfResonance() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfResonance() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setLpfResonance(int value)
{
    if (m_device) {
        m_device->setLpfResonance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::envMod() const
{
    return m_device ? static_cast<int>(std::round(m_device->envMod() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setEnvMod(int value)
{
    if (m_device) {
        m_device->setEnvMod(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::filterLfoDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->filterLfoDepth() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setFilterLfoDepth(int value)
{
    if (m_device) {
        m_device->setFilterLfoDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::keyTrack() const
{
    return m_device ? static_cast<int>(std::round(m_device->keyTrack() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setKeyTrack(int value)
{
    if (m_device) {
        m_device->setKeyTrack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::attack() const
{
    return m_device ? static_cast<int>(std::round(m_device->attack() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setAttack(int value)
{
    if (m_device) {
        m_device->setAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->decay() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setDecay(int value)
{
    if (m_device) {
        m_device->setDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::sustain() const
{
    return m_device ? static_cast<int>(std::round(m_device->sustain() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setSustain(int value)
{
    if (m_device) {
        m_device->setSustain(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::release() const
{
    return m_device ? static_cast<int>(std::round(m_device->release() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setRelease(int value)
{
    if (m_device) {
        m_device->setRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::portamento() const
{
    return m_device ? static_cast<int>(std::round(m_device->portamento() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setPortamento(int value)
{
    if (m_device) {
        m_device->setPortamento(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::panSpread() const
{
    return m_device ? static_cast<int>(std::round(m_device->panSpread() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setPanSpread(int value)
{
    if (m_device) {
        m_device->setPanSpread(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::oscillatorDrift() const
{
    return m_device ? static_cast<int>(std::round(m_device->oscillatorDrift() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setOscillatorDrift(int value)
{
    if (m_device) {
        m_device->setOscillatorDrift(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::voiceTolerance() const
{
    return m_device ? static_cast<int>(std::round(m_device->voiceTolerance() * Constants::uiInternalScaling())) : 0;
}

void JuSynthDialogModel::setVoiceTolerance(int value)
{
    if (m_device) {
        m_device->setVoiceTolerance(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int JuSynthDialogModel::range() const
{
    return m_device ? static_cast<int>(m_device->range()) : 0;
}

void JuSynthDialogModel::setRange(int value)
{
    if (m_device) {
        m_device->setRange(value);
        emit rangeChanged();
    }
}

int JuSynthDialogModel::pwmSource() const
{
    return m_device ? static_cast<int>(m_device->pwmSource()) : 0;
}

void JuSynthDialogModel::setPwmSource(int value)
{
    if (m_device) {
        m_device->setPwmSource(static_cast<JuSynthDevice::PwmSource>(value));
        emit pwmSourceChanged();
    }
}

int JuSynthDialogModel::vcaMode() const
{
    return m_device ? static_cast<int>(m_device->vcaMode()) : 0;
}

void JuSynthDialogModel::setVcaMode(int value)
{
    if (m_device) {
        m_device->setVcaMode(static_cast<JuSynthDevice::VcaMode>(value));
        emit vcaModeChanged();
    }
}

int JuSynthDialogModel::chorusMode() const
{
    return m_device ? static_cast<int>(m_device->chorusMode()) : 0;
}

void JuSynthDialogModel::setChorusMode(int value)
{
    if (m_device) {
        m_device->setChorusMode(static_cast<JunoChorus::Mode>(value));
        emit chorusModeChanged();
    }
}

int JuSynthDialogModel::pitchBendRange() const
{
    return m_device ? static_cast<int>(m_device->pitchBendRange()) : 0;
}

void JuSynthDialogModel::setPitchBendRange(int value)
{
    if (m_device) {
        m_device->setPitchBendRange(value);
        emit pitchBendRangeChanged();
    }
}

bool JuSynthDialogModel::sawEnabled() const
{
    return m_device && m_device->sawEnabled();
}

void JuSynthDialogModel::setSawEnabled(bool value)
{
    if (m_device) {
        m_device->setSawEnabled(value);
        emit sawEnabledChanged();
    }
}

bool JuSynthDialogModel::pulseEnabled() const
{
    return m_device && m_device->pulseEnabled();
}

void JuSynthDialogModel::setPulseEnabled(bool value)
{
    if (m_device) {
        m_device->setPulseEnabled(value);
        emit pulseEnabledChanged();
    }
}

bool JuSynthDialogModel::envInverted() const
{
    return m_device && m_device->envInverted();
}

void JuSynthDialogModel::setEnvInverted(bool value)
{
    if (m_device) {
        m_device->setEnvInverted(value);
        emit envInvertedChanged();
    }
}

void JuSynthDialogModel::requestSettings()
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
    emit panSpreadChanged();
    emit oscillatorDriftChanged();
    emit voiceToleranceChanged();
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
