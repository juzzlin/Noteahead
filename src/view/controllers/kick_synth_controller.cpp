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

#include "kick_synth_controller.hpp"

#include "../../application/service/kick_synth_preview.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/kick_synth_device.hpp"
#include "../../domain/devices/kick_synth_presets.hpp"

#include <cmath>

namespace noteahead {

namespace {
namespace Xml = Constants::NahdXml;
} // namespace

KickSynthController::KickSynthController(std::shared_ptr<KickSynthDevice> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    m_waveformTimer.setSingleShot(true);
    connect(&m_waveformTimer, &QTimer::timeout, this, &KickSynthController::renderWaveform);
    connectDeviceSignals();
}

KickSynthController::~KickSynthController() = default;

DeviceController::DeviceS KickSynthController::device() const
{
    return m_device;
}

bool KickSynthController::setDevice(DeviceController::DeviceS device)
{
    if (const auto kick = std::dynamic_pointer_cast<KickSynthDevice>(device)) {
        setDevice(kick);
        return true;
    }
    return false;
}

void KickSynthController::setDevice(std::shared_ptr<KickSynthDevice> device)
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

void KickSynthController::connectDeviceSignals()
{
    if (!m_device) {
        return;
    }
    connect(m_device.get(), &Device::parametersChanged, this, [this]() {
        requestSettings();
    });
    connect(m_device.get(), &Device::dataChanged, this, [this]() {
        requestSettings();
    });
}

QStringList KickSynthController::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : KickSynthPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void KickSynthController::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int KickSynthController::knob(const QString & key) const
{
    return m_device ? static_cast<int>(std::round(m_device->knob(key) * Constants::uiInternalScaling())) : 0;
}

void KickSynthController::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->setKnob(key, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int KickSynthController::tune() const
{
    return knob(Xml::xmlKeyTune());
}

void KickSynthController::setTune(int value)
{
    setKnob(Xml::xmlKeyTune(), value);
}

int KickSynthController::decay() const
{
    return knob(Xml::xmlKeyDecay());
}

void KickSynthController::setDecay(int value)
{
    setKnob(Xml::xmlKeyDecay(), value);
}

int KickSynthController::tone() const
{
    return knob(Xml::xmlKeyTone());
}

void KickSynthController::setTone(int value)
{
    setKnob(Xml::xmlKeyTone(), value);
}

int KickSynthController::pitchDrop() const
{
    return knob(Xml::xmlKeyPitchDrop());
}

void KickSynthController::setPitchDrop(int value)
{
    setKnob(Xml::xmlKeyPitchDrop(), value);
}

int KickSynthController::damping() const
{
    return knob(Xml::xmlKeyDamping());
}

void KickSynthController::setDamping(int value)
{
    setKnob(Xml::xmlKeyDamping(), value);
}

int KickSynthController::ring() const
{
    return knob(Xml::xmlKeyRing());
}

void KickSynthController::setRing(int value)
{
    setKnob(Xml::xmlKeyRing(), value);
}

int KickSynthController::ringDecay() const
{
    return knob(Xml::xmlKeyRingDecay());
}

void KickSynthController::setRingDecay(int value)
{
    setKnob(Xml::xmlKeyRingDecay(), value);
}

int KickSynthController::beater() const
{
    return knob(Xml::xmlKeyHardness());
}

void KickSynthController::setBeater(int value)
{
    setKnob(Xml::xmlKeyHardness(), value);
}

int KickSynthController::click() const
{
    return knob(Xml::xmlKeyClick());
}

void KickSynthController::setClick(int value)
{
    setKnob(Xml::xmlKeyClick(), value);
}

int KickSynthController::position() const
{
    return knob(Xml::xmlKeyPosition());
}

void KickSynthController::setPosition(int value)
{
    setKnob(Xml::xmlKeyPosition(), value);
}

int KickSynthController::frontHead() const
{
    return knob(Xml::xmlKeyFrontHead());
}

void KickSynthController::setFrontHead(int value)
{
    setKnob(Xml::xmlKeyFrontHead(), value);
}

int KickSynthController::frontTune() const
{
    return knob(Xml::xmlKeyFrontTune());
}

void KickSynthController::setFrontTune(int value)
{
    setKnob(Xml::xmlKeyFrontTune(), value);
}

int KickSynthController::shell() const
{
    return knob(Xml::xmlKeyShell());
}

void KickSynthController::setShell(int value)
{
    setKnob(Xml::xmlKeyShell(), value);
}

int KickSynthController::mic() const
{
    return knob(Xml::xmlKeyMicPosition());
}

void KickSynthController::setMic(int value)
{
    setKnob(Xml::xmlKeyMicPosition(), value);
}

int KickSynthController::drive() const
{
    return knob(Xml::xmlKeyDrive());
}

void KickSynthController::setDrive(int value)
{
    setKnob(Xml::xmlKeyDrive(), value);
}

int KickSynthController::ampAttack() const
{
    return knob(Xml::xmlKeyAmpAttack());
}

void KickSynthController::setAmpAttack(int value)
{
    setKnob(Xml::xmlKeyAmpAttack(), value);
}

int KickSynthController::ampHold() const
{
    return knob(Xml::xmlKeyAmpHold());
}

void KickSynthController::setAmpHold(int value)
{
    setKnob(Xml::xmlKeyAmpHold(), value);
}

int KickSynthController::ampDecay() const
{
    return knob(Xml::xmlKeyAmpDecay());
}

void KickSynthController::setAmpDecay(int value)
{
    setKnob(Xml::xmlKeyAmpDecay(), value);
}

int KickSynthController::ampSustain() const
{
    return knob(Xml::xmlKeyAmpSustain());
}

void KickSynthController::setAmpSustain(int value)
{
    setKnob(Xml::xmlKeyAmpSustain(), value);
}

int KickSynthController::ampRelease() const
{
    return knob(Xml::xmlKeyAmpRelease());
}

void KickSynthController::setAmpRelease(int value)
{
    setKnob(Xml::xmlKeyAmpRelease(), value);
}

int KickSynthController::ampCurve() const
{
    return knob(Xml::xmlKeyAmpCurve());
}

void KickSynthController::setAmpCurve(int value)
{
    setKnob(Xml::xmlKeyAmpCurve(), value);
}

int KickSynthController::velocitySensitivity() const
{
    return knob(Xml::xmlKeyVelocitySensitivity());
}

void KickSynthController::setVelocitySensitivity(int value)
{
    setKnob(Xml::xmlKeyVelocitySensitivity(), value);
}

int KickSynthController::lpfCutoff() const
{
    return knob(Xml::xmlKeyLpfCutoff());
}

void KickSynthController::setLpfCutoff(int value)
{
    setKnob(Xml::xmlKeyLpfCutoff(), value);
}

int KickSynthController::hpfCutoff() const
{
    return knob(Xml::xmlKeyHpfCutoff());
}

void KickSynthController::setHpfCutoff(int value)
{
    setKnob(Xml::xmlKeyHpfCutoff(), value);
}

bool KickSynthController::chromatic() const
{
    return m_device ? m_device->chromatic() : false;
}

void KickSynthController::setChromatic(bool value)
{
    if (m_device) {
        m_device->setChromatic(value);
    }
}

QVariantList KickSynthController::waveformData() const
{
    return m_waveformData;
}

double KickSynthController::waveformDuration() const
{
    return m_waveformDuration;
}

double KickSynthController::audibleLength() const
{
    return m_audibleLength;
}

double KickSynthController::waveformEnvelopeAttack() const
{
    return m_waveformEnvelope.attack;
}

double KickSynthController::waveformEnvelopeHold() const
{
    return m_waveformEnvelope.hold;
}

double KickSynthController::waveformEnvelopeDecay() const
{
    return m_waveformEnvelope.decay;
}

double KickSynthController::waveformEnvelopeSustain() const
{
    return m_waveformEnvelope.sustain;
}

double KickSynthController::waveformEnvelopeRelease() const
{
    return m_waveformEnvelope.release;
}

double KickSynthController::waveformEnvelopeCurve() const
{
    return m_waveformEnvelope.curve;
}

void KickSynthController::setWaveformRequest(int peakCount, bool visible)
{
    const bool changed = peakCount != m_waveformPeakCount || visible != m_waveformVisible;
    m_waveformPeakCount = peakCount;
    m_waveformVisible = visible;
    if (changed) {
        scheduleWaveform();
    }
}

void KickSynthController::scheduleWaveform(int delayMs)
{
    if (!m_waveformVisible || m_waveformPeakCount <= 0) {
        m_waveformTimer.stop();
        return;
    }
    m_waveformTimer.setInterval(delayMs);
    m_waveformTimer.start();
}

void KickSynthController::renderWaveform()
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

void KickSynthController::requestSettings()
{
    scheduleWaveform();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit knobsChanged();
}

} // namespace noteahead
