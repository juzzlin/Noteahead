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

#include "electric_bass_controller.hpp"

#include "../../application/service/electric_bass_preview.hpp"
#include "../../common/constants.hpp"
#include "../../domain/devices/electric_bass_device.hpp"
#include "../../domain/devices/electric_bass_presets.hpp"

#include <cmath>

namespace noteahead {

namespace {
namespace Xml = Constants::NahdXml;
} // namespace

ElectricBassController::ElectricBassController(std::shared_ptr<ElectricBassDevice> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    m_waveformTimer.setSingleShot(true);
    connect(&m_waveformTimer, &QTimer::timeout, this, &ElectricBassController::renderWaveform);
    connectDeviceSignals();
}

ElectricBassController::~ElectricBassController() = default;

DeviceController::DeviceS ElectricBassController::device() const
{
    return m_device;
}

bool ElectricBassController::setDevice(DeviceController::DeviceS device)
{
    if (const auto bass = std::dynamic_pointer_cast<ElectricBassDevice>(device)) {
        setDevice(bass);
        return true;
    }
    return false;
}

void ElectricBassController::setDevice(std::shared_ptr<ElectricBassDevice> device)
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

QStringList ElectricBassController::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : ElectricBassPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void ElectricBassController::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int ElectricBassController::knob(const QString & key) const
{
    return m_device ? static_cast<int>(std::round(m_device->knob(key) * Constants::uiInternalScaling())) : 0;
}

void ElectricBassController::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->setKnob(key, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int ElectricBassController::position() const
{
    return knob(Xml::xmlKeyPosition());
}

void ElectricBassController::setPosition(int value)
{
    setKnob(Xml::xmlKeyPosition(), value);
}

int ElectricBassController::finger() const
{
    return knob(Xml::xmlKeyHardness());
}

void ElectricBassController::setFinger(int value)
{
    setKnob(Xml::xmlKeyHardness(), value);
}

int ElectricBassController::sustain() const
{
    return knob(Xml::xmlKeySustain());
}

void ElectricBassController::setSustain(int value)
{
    setKnob(Xml::xmlKeySustain(), value);
}

int ElectricBassController::brightness() const
{
    return knob(Xml::xmlKeyBrightness());
}

void ElectricBassController::setBrightness(int value)
{
    setKnob(Xml::xmlKeyBrightness(), value);
}

int ElectricBassController::release() const
{
    return knob(Xml::xmlKeyRelease());
}

void ElectricBassController::setRelease(int value)
{
    setKnob(Xml::xmlKeyRelease(), value);
}

int ElectricBassController::neck() const
{
    return knob(Xml::xmlKeyNeckPickup());
}

void ElectricBassController::setNeck(int value)
{
    setKnob(Xml::xmlKeyNeckPickup(), value);
}

int ElectricBassController::bridge() const
{
    return knob(Xml::xmlKeyBridgePickup());
}

void ElectricBassController::setBridge(int value)
{
    setKnob(Xml::xmlKeyBridgePickup(), value);
}

int ElectricBassController::tone() const
{
    return knob(Xml::xmlKeyTone());
}

void ElectricBassController::setTone(int value)
{
    setKnob(Xml::xmlKeyTone(), value);
}

int ElectricBassController::velocitySensitivity() const
{
    return knob(Xml::xmlKeyVelocitySensitivity());
}

void ElectricBassController::setVelocitySensitivity(int value)
{
    setKnob(Xml::xmlKeyVelocitySensitivity(), value);
}

int ElectricBassController::pitchBendRange() const
{
    return m_device ? m_device->pitchBendRange() : 2;
}

void ElectricBassController::setPitchBendRange(int value)
{
    if (m_device) {
        m_device->setPitchBendRange(value);
    }
}

int ElectricBassController::lpfCutoff() const
{
    return knob(Xml::xmlKeyLpfCutoff());
}

void ElectricBassController::setLpfCutoff(int value)
{
    setKnob(Xml::xmlKeyLpfCutoff(), value);
}

int ElectricBassController::hpfCutoff() const
{
    return knob(Xml::xmlKeyHpfCutoff());
}

void ElectricBassController::setHpfCutoff(int value)
{
    setKnob(Xml::xmlKeyHpfCutoff(), value);
}

QVariantList ElectricBassController::waveformData() const
{
    return m_waveformData;
}

double ElectricBassController::waveformDuration() const
{
    return m_waveformDuration;
}

void ElectricBassController::setWaveformRequest(int peakCount, bool visible)
{
    const bool changed = peakCount != m_waveformPeakCount || visible != m_waveformVisible;
    m_waveformPeakCount = peakCount;
    m_waveformVisible = visible;
    if (changed) {
        scheduleWaveform();
    }
}

void ElectricBassController::scheduleWaveform(int delayMs)
{
    if (!m_waveformVisible || m_waveformPeakCount <= 0) {
        m_waveformTimer.stop();
        return;
    }
    m_waveformTimer.setInterval(delayMs);
    m_waveformTimer.start();
}

void ElectricBassController::renderWaveform()
{
    m_waveformTimer.stop();
    if (!m_device || !m_waveformVisible || m_waveformPeakCount <= 0) {
        return;
    }

    const auto preview = ElectricBassPreview::render(*m_device, m_waveformPeakCount, Constants::defaultSampleRate());
    m_waveformData = preview.peaks;
    m_waveformDuration = preview.durationSeconds;
    emit waveformChanged();
}

void ElectricBassController::requestSettings()
{
    scheduleWaveform();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit knobsChanged();
}

} // namespace noteahead
