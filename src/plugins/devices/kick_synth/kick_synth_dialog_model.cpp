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

#include "kick_synth_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "kick_synth_device.hpp"
#include "kick_synth_presets.hpp"
#include "kick_synth_preview.hpp"

#include <cmath>

namespace noteahead {

namespace {
namespace Xml = Constants::NahdXml;
} // namespace

KickSynthDialogModel::KickSynthDialogModel(std::shared_ptr<KickSynthDevice> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    m_waveformTimer.setSingleShot(true);
    connect(&m_waveformTimer, &QTimer::timeout, this, &KickSynthDialogModel::renderWaveform);
    connectDeviceSignals();
}

KickSynthDialogModel::~KickSynthDialogModel() = default;

DeviceDialogModel::DeviceS KickSynthDialogModel::device() const
{
    return m_device;
}

bool KickSynthDialogModel::setDevice(DeviceDialogModel::DeviceS device)
{
    if (const auto kick = std::dynamic_pointer_cast<KickSynthDevice>(device)) {
        setDevice(kick);
        return true;
    }
    return false;
}

void KickSynthDialogModel::setDevice(std::shared_ptr<KickSynthDevice> device)
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

QStringList KickSynthDialogModel::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : KickSynthPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void KickSynthDialogModel::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int KickSynthDialogModel::knob(const QString & key) const
{
    return m_device ? static_cast<int>(std::round(m_device->knob(key) * Constants::uiInternalScaling())) : 0;
}

void KickSynthDialogModel::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->setKnob(key, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int KickSynthDialogModel::tune() const
{
    return knob(Xml::xmlKeyTune());
}

void KickSynthDialogModel::setTune(int value)
{
    setKnob(Xml::xmlKeyTune(), value);
}

int KickSynthDialogModel::decay() const
{
    return knob(Xml::xmlKeyDecay());
}

void KickSynthDialogModel::setDecay(int value)
{
    setKnob(Xml::xmlKeyDecay(), value);
}

int KickSynthDialogModel::tone() const
{
    return knob(Xml::xmlKeyTone());
}

void KickSynthDialogModel::setTone(int value)
{
    setKnob(Xml::xmlKeyTone(), value);
}

int KickSynthDialogModel::pitchDrop() const
{
    return knob(Xml::xmlKeyPitchDrop());
}

void KickSynthDialogModel::setPitchDrop(int value)
{
    setKnob(Xml::xmlKeyPitchDrop(), value);
}

int KickSynthDialogModel::damping() const
{
    return knob(Xml::xmlKeyDamping());
}

void KickSynthDialogModel::setDamping(int value)
{
    setKnob(Xml::xmlKeyDamping(), value);
}

int KickSynthDialogModel::ring() const
{
    return knob(Xml::xmlKeyRing());
}

void KickSynthDialogModel::setRing(int value)
{
    setKnob(Xml::xmlKeyRing(), value);
}

int KickSynthDialogModel::ringDecay() const
{
    return knob(Xml::xmlKeyRingDecay());
}

void KickSynthDialogModel::setRingDecay(int value)
{
    setKnob(Xml::xmlKeyRingDecay(), value);
}

int KickSynthDialogModel::beater() const
{
    return knob(Xml::xmlKeyHardness());
}

void KickSynthDialogModel::setBeater(int value)
{
    setKnob(Xml::xmlKeyHardness(), value);
}

int KickSynthDialogModel::click() const
{
    return knob(Xml::xmlKeyClick());
}

void KickSynthDialogModel::setClick(int value)
{
    setKnob(Xml::xmlKeyClick(), value);
}

int KickSynthDialogModel::position() const
{
    return knob(Xml::xmlKeyPosition());
}

void KickSynthDialogModel::setPosition(int value)
{
    setKnob(Xml::xmlKeyPosition(), value);
}

int KickSynthDialogModel::frontHead() const
{
    return knob(Xml::xmlKeyFrontHead());
}

void KickSynthDialogModel::setFrontHead(int value)
{
    setKnob(Xml::xmlKeyFrontHead(), value);
}

int KickSynthDialogModel::frontTune() const
{
    return knob(Xml::xmlKeyFrontTune());
}

void KickSynthDialogModel::setFrontTune(int value)
{
    setKnob(Xml::xmlKeyFrontTune(), value);
}

int KickSynthDialogModel::shell() const
{
    return knob(Xml::xmlKeyShell());
}

void KickSynthDialogModel::setShell(int value)
{
    setKnob(Xml::xmlKeyShell(), value);
}

int KickSynthDialogModel::mic() const
{
    return knob(Xml::xmlKeyMicPosition());
}

void KickSynthDialogModel::setMic(int value)
{
    setKnob(Xml::xmlKeyMicPosition(), value);
}

int KickSynthDialogModel::drive() const
{
    return knob(Xml::xmlKeyDrive());
}

void KickSynthDialogModel::setDrive(int value)
{
    setKnob(Xml::xmlKeyDrive(), value);
}

int KickSynthDialogModel::ampAttack() const
{
    return knob(Xml::xmlKeyAmpAttack());
}

void KickSynthDialogModel::setAmpAttack(int value)
{
    setKnob(Xml::xmlKeyAmpAttack(), value);
}

int KickSynthDialogModel::ampHold() const
{
    return knob(Xml::xmlKeyAmpHold());
}

void KickSynthDialogModel::setAmpHold(int value)
{
    setKnob(Xml::xmlKeyAmpHold(), value);
}

int KickSynthDialogModel::ampDecay() const
{
    return knob(Xml::xmlKeyAmpDecay());
}

void KickSynthDialogModel::setAmpDecay(int value)
{
    setKnob(Xml::xmlKeyAmpDecay(), value);
}

int KickSynthDialogModel::ampSustain() const
{
    return knob(Xml::xmlKeyAmpSustain());
}

void KickSynthDialogModel::setAmpSustain(int value)
{
    setKnob(Xml::xmlKeyAmpSustain(), value);
}

int KickSynthDialogModel::ampRelease() const
{
    return knob(Xml::xmlKeyAmpRelease());
}

void KickSynthDialogModel::setAmpRelease(int value)
{
    setKnob(Xml::xmlKeyAmpRelease(), value);
}

int KickSynthDialogModel::ampCurve() const
{
    return knob(Xml::xmlKeyAmpCurve());
}

void KickSynthDialogModel::setAmpCurve(int value)
{
    setKnob(Xml::xmlKeyAmpCurve(), value);
}

int KickSynthDialogModel::velocitySensitivity() const
{
    return knob(Xml::xmlKeyVelocitySensitivity());
}

void KickSynthDialogModel::setVelocitySensitivity(int value)
{
    setKnob(Xml::xmlKeyVelocitySensitivity(), value);
}

int KickSynthDialogModel::lpfCutoff() const
{
    return knob(Xml::xmlKeyLpfCutoff());
}

void KickSynthDialogModel::setLpfCutoff(int value)
{
    setKnob(Xml::xmlKeyLpfCutoff(), value);
}

int KickSynthDialogModel::hpfCutoff() const
{
    return knob(Xml::xmlKeyHpfCutoff());
}

void KickSynthDialogModel::setHpfCutoff(int value)
{
    setKnob(Xml::xmlKeyHpfCutoff(), value);
}

bool KickSynthDialogModel::chromatic() const
{
    return m_device ? m_device->chromatic() : false;
}

void KickSynthDialogModel::setChromatic(bool value)
{
    if (m_device) {
        m_device->setChromatic(value);
    }
}

QVariantList KickSynthDialogModel::waveformData() const
{
    return m_waveformData;
}

double KickSynthDialogModel::waveformDuration() const
{
    return m_waveformDuration;
}

double KickSynthDialogModel::audibleLength() const
{
    return m_audibleLength;
}

double KickSynthDialogModel::waveformEnvelopeAttack() const
{
    return m_waveformEnvelope.attack;
}

double KickSynthDialogModel::waveformEnvelopeHold() const
{
    return m_waveformEnvelope.hold;
}

double KickSynthDialogModel::waveformEnvelopeDecay() const
{
    return m_waveformEnvelope.decay;
}

double KickSynthDialogModel::waveformEnvelopeSustain() const
{
    return m_waveformEnvelope.sustain;
}

double KickSynthDialogModel::waveformEnvelopeRelease() const
{
    return m_waveformEnvelope.release;
}

double KickSynthDialogModel::waveformEnvelopeCurve() const
{
    return m_waveformEnvelope.curve;
}

void KickSynthDialogModel::setWaveformRequest(int peakCount, bool visible)
{
    const bool changed = peakCount != m_waveformPeakCount || visible != m_waveformVisible;
    m_waveformPeakCount = peakCount;
    m_waveformVisible = visible;
    if (changed) {
        scheduleWaveform();
    }
}

void KickSynthDialogModel::scheduleWaveform(int delayMs)
{
    if (!m_waveformVisible || m_waveformPeakCount <= 0) {
        m_waveformTimer.stop();
        return;
    }
    m_waveformTimer.setInterval(delayMs);
    m_waveformTimer.start();
}

void KickSynthDialogModel::renderWaveform()
{
    m_waveformTimer.stop();
    if (!m_device || !m_waveformVisible || m_waveformPeakCount <= 0) {
        return;
    }

    const auto preview = KickSynthPreview::render(*m_device, m_waveformPeakCount, Constants::defaultSampleRate());
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

void KickSynthDialogModel::requestSettings()
{
    scheduleWaveform();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit knobsChanged();
}

} // namespace noteahead
