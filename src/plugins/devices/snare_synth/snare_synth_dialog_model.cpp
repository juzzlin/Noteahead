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

#include "snare_synth_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "snare_synth_device.hpp"
#include "snare_synth_presets.hpp"
#include "snare_synth_preview.hpp"

#include <cmath>

namespace noteahead {

SnareSynthDialogModel::SnareSynthDialogModel(std::shared_ptr<SnareSynthDevice> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    m_waveformTimer.setSingleShot(true);
    connect(&m_waveformTimer, &QTimer::timeout, this, &SnareSynthDialogModel::renderWaveform);
    connectDeviceSignals();
}

SnareSynthDialogModel::~SnareSynthDialogModel() = default;

DeviceDialogModel::DeviceS SnareSynthDialogModel::device() const
{
    return m_device;
}

bool SnareSynthDialogModel::setDevice(DeviceDialogModel::DeviceS device)
{
    if (const auto snare = std::dynamic_pointer_cast<SnareSynthDevice>(device)) {
        setDevice(snare);
        return true;
    }
    return false;
}

void SnareSynthDialogModel::setDevice(std::shared_ptr<SnareSynthDevice> device)
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

QStringList SnareSynthDialogModel::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : SnareSynthPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void SnareSynthDialogModel::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int SnareSynthDialogModel::tune() const
{
    return m_device ? static_cast<int>(std::round(m_device->tune() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setTune(int value)
{
    if (m_device) {
        m_device->setTune(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::tone() const
{
    return m_device ? static_cast<int>(std::round(m_device->tone() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setTone(int value)
{
    if (m_device) {
        m_device->setTone(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->decay() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setDecay(int value)
{
    if (m_device) {
        m_device->setDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::headTension() const
{
    return m_device ? static_cast<int>(std::round(m_device->headTension() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setHeadTension(int value)
{
    if (m_device) {
        m_device->setHeadTension(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::pitchDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchDepth() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setPitchDepth(int value)
{
    if (m_device) {
        m_device->setPitchDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::pitchDecay() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchDecay() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setPitchDecay(int value)
{
    if (m_device) {
        m_device->setPitchDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::snappy() const
{
    return m_device ? static_cast<int>(std::round(m_device->snappy() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setSnappy(int value)
{
    if (m_device) {
        m_device->setSnappy(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::snareDecay() const
{
    return m_device ? static_cast<int>(std::round(m_device->snareDecay() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setSnareDecay(int value)
{
    if (m_device) {
        m_device->setSnareDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::snareTension() const
{
    return m_device ? static_cast<int>(std::round(m_device->snareTension() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setSnareTension(int value)
{
    if (m_device) {
        m_device->setSnareTension(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::snareBuzz() const
{
    return m_device ? static_cast<int>(std::round(m_device->snareBuzz() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setSnareBuzz(int value)
{
    if (m_device) {
        m_device->setSnareBuzz(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::click() const
{
    return m_device ? static_cast<int>(std::round(m_device->click() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setClick(int value)
{
    if (m_device) {
        m_device->setClick(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::hardness() const
{
    return m_device ? static_cast<int>(std::round(m_device->hardness() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setHardness(int value)
{
    if (m_device) {
        m_device->setHardness(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::position() const
{
    return m_device ? static_cast<int>(std::round(m_device->position() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setPosition(int value)
{
    if (m_device) {
        m_device->setPosition(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::body() const
{
    return m_device ? static_cast<int>(std::round(m_device->body() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setBody(int value)
{
    if (m_device) {
        m_device->setBody(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::crack() const
{
    return m_device ? static_cast<int>(std::round(m_device->crack() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setCrack(int value)
{
    if (m_device) {
        m_device->setCrack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::punch() const
{
    return m_device ? static_cast<int>(std::round(m_device->punch() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setPunch(int value)
{
    if (m_device) {
        m_device->setPunch(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::wireWidth() const
{
    return m_device ? static_cast<int>(std::round(m_device->wireWidth() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setWireWidth(int value)
{
    if (m_device) {
        m_device->setWireWidth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::ampAttack() const
{
    return m_device ? static_cast<int>(std::round(m_device->ampAttack() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setAmpAttack(int value)
{
    if (m_device) {
        m_device->setAmpAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::ampHold() const
{
    return m_device ? static_cast<int>(std::round(m_device->ampHold() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setAmpHold(int value)
{
    if (m_device) {
        m_device->setAmpHold(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::ampDecay() const
{
    return m_device ? static_cast<int>(std::round(m_device->ampDecay() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setAmpDecay(int value)
{
    if (m_device) {
        m_device->setAmpDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::ampSustain() const
{
    return m_device ? static_cast<int>(std::round(m_device->ampSustain() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setAmpSustain(int value)
{
    if (m_device) {
        m_device->setAmpSustain(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::ampRelease() const
{
    return m_device ? static_cast<int>(std::round(m_device->ampRelease() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setAmpRelease(int value)
{
    if (m_device) {
        m_device->setAmpRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::ampCurve() const
{
    return m_device ? static_cast<int>(std::round(m_device->ampCurve() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setAmpCurve(int value)
{
    if (m_device) {
        m_device->setAmpCurve(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

QVariantList SnareSynthDialogModel::waveformData() const
{
    return m_waveformData;
}

double SnareSynthDialogModel::waveformDuration() const
{
    return m_waveformDuration;
}

double SnareSynthDialogModel::audibleLength() const
{
    return m_audibleLength;
}

double SnareSynthDialogModel::waveformEnvelopeAttack() const
{
    return m_waveformEnvelope.attack;
}

double SnareSynthDialogModel::waveformEnvelopeHold() const
{
    return m_waveformEnvelope.hold;
}

double SnareSynthDialogModel::waveformEnvelopeDecay() const
{
    return m_waveformEnvelope.decay;
}

double SnareSynthDialogModel::waveformEnvelopeSustain() const
{
    return m_waveformEnvelope.sustain;
}

double SnareSynthDialogModel::waveformEnvelopeRelease() const
{
    return m_waveformEnvelope.release;
}

double SnareSynthDialogModel::waveformEnvelopeCurve() const
{
    return m_waveformEnvelope.curve;
}

void SnareSynthDialogModel::setWaveformRequest(int peakCount, bool visible)
{
    const bool changed = peakCount != m_waveformPeakCount || visible != m_waveformVisible;
    m_waveformPeakCount = peakCount;
    m_waveformVisible = visible;
    if (changed) {
        scheduleWaveform();
    }
}

void SnareSynthDialogModel::scheduleWaveform(int delayMs)
{
    if (!m_waveformVisible || m_waveformPeakCount <= 0) {
        m_waveformTimer.stop();
        return;
    }
    m_waveformTimer.setInterval(delayMs);
    m_waveformTimer.start();
}

void SnareSynthDialogModel::renderWaveform()
{
    m_waveformTimer.stop();
    if (!m_device || !m_waveformVisible || m_waveformPeakCount <= 0) {
        return;
    }

    const auto preview = SnareSynthPreview::render(*m_device, m_waveformPeakCount, Constants::defaultSampleRate());
    m_waveformData = preview.peaks;
    m_waveformDuration = preview.durationSeconds;
    m_audibleLength = preview.audibleSeconds;
    m_waveformEnvelope = {
        .attack = m_device->ampAttackSeconds(),
        .hold = m_device->ampHoldSeconds(),
        .decay = m_device->ampDecaySeconds(),
        .sustain = static_cast<double>(m_device->ampSustain()),
        .release = m_device->ampReleaseSeconds(),
        .curve = static_cast<double>(m_device->ampCurve())
    };
    emit waveformChanged();
}

int SnareSynthDialogModel::drive() const
{
    return m_device ? static_cast<int>(std::round(m_device->drive() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setDrive(int value)
{
    if (m_device) {
        m_device->setDrive(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::panSpread() const
{
    return m_device ? static_cast<int>(std::round(m_device->panSpread() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setPanSpread(int value)
{
    if (m_device) {
        m_device->setPanSpread(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool SnareSynthDialogModel::chromatic() const
{
    return m_device ? m_device->chromatic() : false;
}

void SnareSynthDialogModel::setChromatic(bool value)
{
    if (m_device) {
        m_device->setChromatic(value);
    }
}

int SnareSynthDialogModel::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthDialogModel::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthDialogModel::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void SnareSynthDialogModel::requestSettings()
{
    scheduleWaveform();

    emit volumeChanged();
    emit gainChanged();
    emit panChanged();

    emit tuneChanged();
    emit toneChanged();
    emit decayChanged();
    emit headTensionChanged();
    emit pitchDepthChanged();
    emit pitchDecayChanged();

    emit snappyChanged();
    emit snareDecayChanged();
    emit snareTensionChanged();
    emit snareBuzzChanged();

    emit clickChanged();
    emit hardnessChanged();
    emit positionChanged();
    emit bodyChanged();
    emit driveChanged();
    emit panSpreadChanged();
    emit ampAttackChanged();
    emit ampHoldChanged();
    emit ampDecayChanged();
    emit ampSustainChanged();
    emit ampReleaseChanged();
    emit ampCurveChanged();
    emit crackChanged();
    emit punchChanged();
    emit wireWidthChanged();

    emit chromaticChanged();
    emit velocitySensitivityChanged();

    emit lpfCutoffChanged();
    emit hpfCutoffChanged();
}

} // namespace noteahead
