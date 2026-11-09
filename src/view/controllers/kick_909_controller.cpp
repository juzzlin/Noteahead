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

#include "kick_909_controller.hpp"

#include "../../application/service/kick_909_preview.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/kick_909_device.hpp"
#include "../../domain/devices/kick_909_presets.hpp"

#include <cmath>

namespace noteahead {

namespace {
namespace Xml = Constants::NahdXml;
} // namespace

Kick909Controller::Kick909Controller(std::shared_ptr<Kick909Device> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    m_waveformTimer.setSingleShot(true);
    connect(&m_waveformTimer, &QTimer::timeout, this, &Kick909Controller::renderWaveform);
    connectDeviceSignals();
}

Kick909Controller::~Kick909Controller() = default;

DeviceController::DeviceS Kick909Controller::device() const
{
    return m_device;
}

bool Kick909Controller::setDevice(DeviceController::DeviceS device)
{
    if (const auto kick = std::dynamic_pointer_cast<Kick909Device>(device)) {
        setDevice(kick);
        return true;
    }
    return false;
}

void Kick909Controller::setDevice(std::shared_ptr<Kick909Device> device)
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

QStringList Kick909Controller::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : Kick909Presets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void Kick909Controller::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int Kick909Controller::knob(const QString & key) const
{
    return m_device ? static_cast<int>(std::round(m_device->knob(key) * Constants::uiInternalScaling())) : 0;
}

void Kick909Controller::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->setKnob(key, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick909Controller::tune() const
{
    return knob(Xml::xmlKeyTune());
}

void Kick909Controller::setTune(int value)
{
    setKnob(Xml::xmlKeyTune(), value);
}

int Kick909Controller::attack() const
{
    return knob(Xml::xmlKeyAttack());
}

void Kick909Controller::setAttack(int value)
{
    setKnob(Xml::xmlKeyAttack(), value);
}

int Kick909Controller::decay() const
{
    return knob(Xml::xmlKeyDecay());
}

void Kick909Controller::setDecay(int value)
{
    setKnob(Xml::xmlKeyDecay(), value);
}

int Kick909Controller::pitch() const
{
    return knob(Xml::xmlKeyPitch());
}

void Kick909Controller::setPitch(int value)
{
    setKnob(Xml::xmlKeyPitch(), value);
}

int Kick909Controller::pitchDepth() const
{
    return knob(Xml::xmlKeyPitchDepth());
}

void Kick909Controller::setPitchDepth(int value)
{
    setKnob(Xml::xmlKeyPitchDepth(), value);
}

int Kick909Controller::drive() const
{
    return knob(Xml::xmlKeyDrive());
}

void Kick909Controller::setDrive(int value)
{
    setKnob(Xml::xmlKeyDrive(), value);
}

int Kick909Controller::ampAttack() const
{
    return knob(Xml::xmlKeyAmpAttack());
}

void Kick909Controller::setAmpAttack(int value)
{
    setKnob(Xml::xmlKeyAmpAttack(), value);
}

int Kick909Controller::ampHold() const
{
    return knob(Xml::xmlKeyAmpHold());
}

void Kick909Controller::setAmpHold(int value)
{
    setKnob(Xml::xmlKeyAmpHold(), value);
}

int Kick909Controller::ampDecay() const
{
    return knob(Xml::xmlKeyAmpDecay());
}

void Kick909Controller::setAmpDecay(int value)
{
    setKnob(Xml::xmlKeyAmpDecay(), value);
}

int Kick909Controller::ampSustain() const
{
    return knob(Xml::xmlKeyAmpSustain());
}

void Kick909Controller::setAmpSustain(int value)
{
    setKnob(Xml::xmlKeyAmpSustain(), value);
}

int Kick909Controller::ampRelease() const
{
    return knob(Xml::xmlKeyAmpRelease());
}

void Kick909Controller::setAmpRelease(int value)
{
    setKnob(Xml::xmlKeyAmpRelease(), value);
}

int Kick909Controller::ampCurve() const
{
    return knob(Xml::xmlKeyAmpCurve());
}

void Kick909Controller::setAmpCurve(int value)
{
    setKnob(Xml::xmlKeyAmpCurve(), value);
}

int Kick909Controller::velocitySensitivity() const
{
    return knob(Xml::xmlKeyVelocitySensitivity());
}

void Kick909Controller::setVelocitySensitivity(int value)
{
    setKnob(Xml::xmlKeyVelocitySensitivity(), value);
}

int Kick909Controller::lpfCutoff() const
{
    return knob(Xml::xmlKeyLpfCutoff());
}

void Kick909Controller::setLpfCutoff(int value)
{
    setKnob(Xml::xmlKeyLpfCutoff(), value);
}

int Kick909Controller::hpfCutoff() const
{
    return knob(Xml::xmlKeyHpfCutoff());
}

void Kick909Controller::setHpfCutoff(int value)
{
    setKnob(Xml::xmlKeyHpfCutoff(), value);
}

bool Kick909Controller::chromatic() const
{
    return m_device ? m_device->chromatic() : false;
}

void Kick909Controller::setChromatic(bool value)
{
    if (m_device) {
        m_device->setChromatic(value);
    }
}

QVariantList Kick909Controller::waveformData() const
{
    return m_waveformData;
}

double Kick909Controller::waveformDuration() const
{
    return m_waveformDuration;
}

double Kick909Controller::audibleLength() const
{
    return m_audibleLength;
}

double Kick909Controller::waveformEnvelopeAttack() const
{
    return m_waveformEnvelope.attack;
}

double Kick909Controller::waveformEnvelopeHold() const
{
    return m_waveformEnvelope.hold;
}

double Kick909Controller::waveformEnvelopeDecay() const
{
    return m_waveformEnvelope.decay;
}

double Kick909Controller::waveformEnvelopeSustain() const
{
    return m_waveformEnvelope.sustain;
}

double Kick909Controller::waveformEnvelopeRelease() const
{
    return m_waveformEnvelope.release;
}

double Kick909Controller::waveformEnvelopeCurve() const
{
    return m_waveformEnvelope.curve;
}

void Kick909Controller::setWaveformRequest(int peakCount, bool visible)
{
    const bool changed = peakCount != m_waveformPeakCount || visible != m_waveformVisible;
    m_waveformPeakCount = peakCount;
    m_waveformVisible = visible;
    if (changed) {
        scheduleWaveform();
    }
}

void Kick909Controller::scheduleWaveform(int delayMs)
{
    if (!m_waveformVisible || m_waveformPeakCount <= 0) {
        m_waveformTimer.stop();
        return;
    }
    m_waveformTimer.setInterval(delayMs);
    m_waveformTimer.start();
}

void Kick909Controller::renderWaveform()
{
    m_waveformTimer.stop();
    if (!m_device || !m_waveformVisible || m_waveformPeakCount <= 0) {
        return;
    }

    const auto preview = Kick909Preview::render(*m_device, m_waveformPeakCount, Constants::defaultSampleRate());
    m_waveformData = preview.peaks;
    m_waveformDuration = preview.durationSeconds;
    m_audibleLength = preview.audibleSeconds;
    m_waveformEnvelope = {
        .attack = m_device->ampAttackSeconds(),
        .hold = m_device->ampHoldSeconds(),
        .decay = m_device->ampDecaySeconds(),
        .sustain = static_cast<double>(m_device->knob(Xml::xmlKeyAmpSustain())),
        .release = m_device->ampReleaseSeconds(),
        .curve = static_cast<double>(m_device->knob(Xml::xmlKeyAmpCurve()))
    };
    emit waveformChanged();
}

void Kick909Controller::requestSettings()
{
    scheduleWaveform();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit knobsChanged();
}

} // namespace noteahead
