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

#include "snare_synth_controller.hpp"

#include "../../application/service/snare_synth_preview.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/snare_synth_device.hpp"
#include "../../domain/devices/snare_synth_presets.hpp"

#include <cmath>

namespace noteahead {

SnareSynthController::SnareSynthController(std::shared_ptr<SnareSynthDevice> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    m_waveformTimer.setSingleShot(true);
    connect(&m_waveformTimer, &QTimer::timeout, this, &SnareSynthController::renderWaveform);
    connectDeviceSignals();
}

SnareSynthController::~SnareSynthController() = default;

DeviceController::DeviceS SnareSynthController::device() const
{
    return m_device;
}

bool SnareSynthController::setDevice(DeviceController::DeviceS device)
{
    if (const auto snare = std::dynamic_pointer_cast<SnareSynthDevice>(device)) {
        setDevice(snare);
        return true;
    }
    return false;
}

void SnareSynthController::setDevice(std::shared_ptr<SnareSynthDevice> device)
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

QStringList SnareSynthController::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : SnareSynthPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void SnareSynthController::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int SnareSynthController::tune() const
{
    return m_device ? static_cast<int>(std::round(m_device->tune() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setTune(int value)
{
    if (m_device) {
        m_device->setTune(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::tone() const
{
    return m_device ? static_cast<int>(std::round(m_device->tone() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setTone(int value)
{
    if (m_device) {
        m_device->setTone(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->decay() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setDecay(int value)
{
    if (m_device) {
        m_device->setDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::headTension() const
{
    return m_device ? static_cast<int>(std::round(m_device->headTension() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setHeadTension(int value)
{
    if (m_device) {
        m_device->setHeadTension(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::pitchDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchDepth() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setPitchDepth(int value)
{
    if (m_device) {
        m_device->setPitchDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::pitchDecay() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchDecay() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setPitchDecay(int value)
{
    if (m_device) {
        m_device->setPitchDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::snappy() const
{
    return m_device ? static_cast<int>(std::round(m_device->snappy() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setSnappy(int value)
{
    if (m_device) {
        m_device->setSnappy(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::snareDecay() const
{
    return m_device ? static_cast<int>(std::round(m_device->snareDecay() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setSnareDecay(int value)
{
    if (m_device) {
        m_device->setSnareDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::snareTension() const
{
    return m_device ? static_cast<int>(std::round(m_device->snareTension() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setSnareTension(int value)
{
    if (m_device) {
        m_device->setSnareTension(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::snareBuzz() const
{
    return m_device ? static_cast<int>(std::round(m_device->snareBuzz() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setSnareBuzz(int value)
{
    if (m_device) {
        m_device->setSnareBuzz(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::click() const
{
    return m_device ? static_cast<int>(std::round(m_device->click() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setClick(int value)
{
    if (m_device) {
        m_device->setClick(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::hardness() const
{
    return m_device ? static_cast<int>(std::round(m_device->hardness() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setHardness(int value)
{
    if (m_device) {
        m_device->setHardness(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::position() const
{
    return m_device ? static_cast<int>(std::round(m_device->position() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setPosition(int value)
{
    if (m_device) {
        m_device->setPosition(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::body() const
{
    return m_device ? static_cast<int>(std::round(m_device->body() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setBody(int value)
{
    if (m_device) {
        m_device->setBody(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::crack() const
{
    return m_device ? static_cast<int>(std::round(m_device->crack() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setCrack(int value)
{
    if (m_device) {
        m_device->setCrack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::punch() const
{
    return m_device ? static_cast<int>(std::round(m_device->punch() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setPunch(int value)
{
    if (m_device) {
        m_device->setPunch(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::wireWidth() const
{
    return m_device ? static_cast<int>(std::round(m_device->wireWidth() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setWireWidth(int value)
{
    if (m_device) {
        m_device->setWireWidth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::ampAttack() const
{
    return m_device ? static_cast<int>(std::round(m_device->ampAttack() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setAmpAttack(int value)
{
    if (m_device) {
        m_device->setAmpAttack(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::ampHold() const
{
    return m_device ? static_cast<int>(std::round(m_device->ampHold() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setAmpHold(int value)
{
    if (m_device) {
        m_device->setAmpHold(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::ampDecay() const
{
    return m_device ? static_cast<int>(std::round(m_device->ampDecay() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setAmpDecay(int value)
{
    if (m_device) {
        m_device->setAmpDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::ampSustain() const
{
    return m_device ? static_cast<int>(std::round(m_device->ampSustain() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setAmpSustain(int value)
{
    if (m_device) {
        m_device->setAmpSustain(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::ampRelease() const
{
    return m_device ? static_cast<int>(std::round(m_device->ampRelease() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setAmpRelease(int value)
{
    if (m_device) {
        m_device->setAmpRelease(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::ampCurve() const
{
    return m_device ? static_cast<int>(std::round(m_device->ampCurve() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setAmpCurve(int value)
{
    if (m_device) {
        m_device->setAmpCurve(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

QVariantList SnareSynthController::waveformData() const
{
    return m_waveformData;
}

double SnareSynthController::waveformDuration() const
{
    return m_waveformDuration;
}

double SnareSynthController::audibleLength() const
{
    return m_audibleLength;
}

double SnareSynthController::waveformEnvelopeAttack() const
{
    return m_waveformEnvelope.attack;
}

double SnareSynthController::waveformEnvelopeHold() const
{
    return m_waveformEnvelope.hold;
}

double SnareSynthController::waveformEnvelopeDecay() const
{
    return m_waveformEnvelope.decay;
}

double SnareSynthController::waveformEnvelopeSustain() const
{
    return m_waveformEnvelope.sustain;
}

double SnareSynthController::waveformEnvelopeRelease() const
{
    return m_waveformEnvelope.release;
}

double SnareSynthController::waveformEnvelopeCurve() const
{
    return m_waveformEnvelope.curve;
}

void SnareSynthController::setWaveformRequest(int peakCount, bool visible)
{
    const bool changed = peakCount != m_waveformPeakCount || visible != m_waveformVisible;
    m_waveformPeakCount = peakCount;
    m_waveformVisible = visible;
    if (changed) {
        scheduleWaveform();
    }
}

void SnareSynthController::scheduleWaveform(int delayMs)
{
    if (!m_waveformVisible || m_waveformPeakCount <= 0) {
        m_waveformTimer.stop();
        return;
    }
    m_waveformTimer.setInterval(delayMs);
    m_waveformTimer.start();
}

void SnareSynthController::renderWaveform()
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

int SnareSynthController::drive() const
{
    return m_device ? static_cast<int>(std::round(m_device->drive() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setDrive(int value)
{
    if (m_device) {
        m_device->setDrive(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::panSpread() const
{
    return m_device ? static_cast<int>(std::round(m_device->panSpread() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setPanSpread(int value)
{
    if (m_device) {
        m_device->setPanSpread(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool SnareSynthController::chromatic() const
{
    return m_device ? m_device->chromatic() : false;
}

void SnareSynthController::setChromatic(bool value)
{
    if (m_device) {
        m_device->setChromatic(value);
    }
}

int SnareSynthController::velocitySensitivity() const
{
    return m_device ? static_cast<int>(std::round(m_device->velocitySensitivity() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setVelocitySensitivity(int value)
{
    if (m_device) {
        m_device->setVelocitySensitivity(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int SnareSynthController::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void SnareSynthController::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

void SnareSynthController::requestSettings()
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
