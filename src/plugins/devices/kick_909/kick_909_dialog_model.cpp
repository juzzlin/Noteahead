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

#include "kick_909_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "kick_909_device.hpp"
#include "kick_909_presets.hpp"
#include "kick_909_preview.hpp"

#include <cmath>

namespace noteahead {

namespace {
namespace Xml = Constants::NahdXml;
} // namespace

Kick909DialogModel::Kick909DialogModel(std::shared_ptr<Kick909Device> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    m_waveformTimer.setSingleShot(true);
    connect(&m_waveformTimer, &QTimer::timeout, this, &Kick909DialogModel::renderWaveform);
    connectDeviceSignals();
}

Kick909DialogModel::~Kick909DialogModel() = default;

DeviceDialogModel::DeviceS Kick909DialogModel::device() const
{
    return m_device;
}

bool Kick909DialogModel::setDevice(DeviceDialogModel::DeviceS device)
{
    if (const auto kick = std::dynamic_pointer_cast<Kick909Device>(device)) {
        setDevice(kick);
        return true;
    }
    return false;
}

void Kick909DialogModel::setDevice(std::shared_ptr<Kick909Device> device)
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

QStringList Kick909DialogModel::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : Kick909Presets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void Kick909DialogModel::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int Kick909DialogModel::knob(const QString & key) const
{
    return m_device ? static_cast<int>(std::round(m_device->knob(key) * Constants::uiInternalScaling())) : 0;
}

void Kick909DialogModel::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->setKnob(key, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick909DialogModel::tune() const
{
    return knob(Xml::xmlKeyTune());
}

void Kick909DialogModel::setTune(int value)
{
    setKnob(Xml::xmlKeyTune(), value);
}

int Kick909DialogModel::attack() const
{
    return knob(Xml::xmlKeyAttack());
}

void Kick909DialogModel::setAttack(int value)
{
    setKnob(Xml::xmlKeyAttack(), value);
}

int Kick909DialogModel::decay() const
{
    return knob(Xml::xmlKeyDecay());
}

void Kick909DialogModel::setDecay(int value)
{
    setKnob(Xml::xmlKeyDecay(), value);
}

int Kick909DialogModel::pitch() const
{
    return knob(Xml::xmlKeyPitch());
}

void Kick909DialogModel::setPitch(int value)
{
    setKnob(Xml::xmlKeyPitch(), value);
}

int Kick909DialogModel::pitchDepth() const
{
    return knob(Xml::xmlKeyPitchDepth());
}

void Kick909DialogModel::setPitchDepth(int value)
{
    setKnob(Xml::xmlKeyPitchDepth(), value);
}

int Kick909DialogModel::drive() const
{
    return knob(Xml::xmlKeyDrive());
}

void Kick909DialogModel::setDrive(int value)
{
    setKnob(Xml::xmlKeyDrive(), value);
}

int Kick909DialogModel::ampAttack() const
{
    return knob(Xml::xmlKeyAmpAttack());
}

void Kick909DialogModel::setAmpAttack(int value)
{
    setKnob(Xml::xmlKeyAmpAttack(), value);
}

int Kick909DialogModel::ampHold() const
{
    return knob(Xml::xmlKeyAmpHold());
}

void Kick909DialogModel::setAmpHold(int value)
{
    setKnob(Xml::xmlKeyAmpHold(), value);
}

int Kick909DialogModel::ampDecay() const
{
    return knob(Xml::xmlKeyAmpDecay());
}

void Kick909DialogModel::setAmpDecay(int value)
{
    setKnob(Xml::xmlKeyAmpDecay(), value);
}

int Kick909DialogModel::ampSustain() const
{
    return knob(Xml::xmlKeyAmpSustain());
}

void Kick909DialogModel::setAmpSustain(int value)
{
    setKnob(Xml::xmlKeyAmpSustain(), value);
}

int Kick909DialogModel::ampRelease() const
{
    return knob(Xml::xmlKeyAmpRelease());
}

void Kick909DialogModel::setAmpRelease(int value)
{
    setKnob(Xml::xmlKeyAmpRelease(), value);
}

int Kick909DialogModel::ampCurve() const
{
    return knob(Xml::xmlKeyAmpCurve());
}

void Kick909DialogModel::setAmpCurve(int value)
{
    setKnob(Xml::xmlKeyAmpCurve(), value);
}

int Kick909DialogModel::velocitySensitivity() const
{
    return knob(Xml::xmlKeyVelocitySensitivity());
}

void Kick909DialogModel::setVelocitySensitivity(int value)
{
    setKnob(Xml::xmlKeyVelocitySensitivity(), value);
}

int Kick909DialogModel::lpfCutoff() const
{
    return knob(Xml::xmlKeyLpfCutoff());
}

void Kick909DialogModel::setLpfCutoff(int value)
{
    setKnob(Xml::xmlKeyLpfCutoff(), value);
}

int Kick909DialogModel::hpfCutoff() const
{
    return knob(Xml::xmlKeyHpfCutoff());
}

void Kick909DialogModel::setHpfCutoff(int value)
{
    setKnob(Xml::xmlKeyHpfCutoff(), value);
}

bool Kick909DialogModel::chromatic() const
{
    return m_device ? m_device->chromatic() : false;
}

void Kick909DialogModel::setChromatic(bool value)
{
    if (m_device) {
        m_device->setChromatic(value);
    }
}

QVariantList Kick909DialogModel::waveformData() const
{
    return m_waveformData;
}

double Kick909DialogModel::waveformDuration() const
{
    return m_waveformDuration;
}

double Kick909DialogModel::audibleLength() const
{
    return m_audibleLength;
}

double Kick909DialogModel::waveformEnvelopeAttack() const
{
    return m_waveformEnvelope.attack;
}

double Kick909DialogModel::waveformEnvelopeHold() const
{
    return m_waveformEnvelope.hold;
}

double Kick909DialogModel::waveformEnvelopeDecay() const
{
    return m_waveformEnvelope.decay;
}

double Kick909DialogModel::waveformEnvelopeSustain() const
{
    return m_waveformEnvelope.sustain;
}

double Kick909DialogModel::waveformEnvelopeRelease() const
{
    return m_waveformEnvelope.release;
}

double Kick909DialogModel::waveformEnvelopeCurve() const
{
    return m_waveformEnvelope.curve;
}

void Kick909DialogModel::setWaveformRequest(int peakCount, bool visible)
{
    const bool changed = peakCount != m_waveformPeakCount || visible != m_waveformVisible;
    m_waveformPeakCount = peakCount;
    m_waveformVisible = visible;
    if (changed) {
        scheduleWaveform();
    }
}

void Kick909DialogModel::scheduleWaveform(int delayMs)
{
    if (!m_waveformVisible || m_waveformPeakCount <= 0) {
        m_waveformTimer.stop();
        return;
    }
    m_waveformTimer.setInterval(delayMs);
    m_waveformTimer.start();
}

void Kick909DialogModel::renderWaveform()
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

void Kick909DialogModel::requestSettings()
{
    scheduleWaveform();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit knobsChanged();
}

} // namespace noteahead
