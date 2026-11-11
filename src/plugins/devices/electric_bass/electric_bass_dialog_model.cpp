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

#include "electric_bass_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "electric_bass_device.hpp"
#include "electric_bass_presets.hpp"
#include "electric_bass_preview.hpp"

#include <cmath>

namespace noteahead {

namespace {
namespace Xml = Constants::NahdXml;
} // namespace

ElectricBassDialogModel::ElectricBassDialogModel(std::shared_ptr<ElectricBassDevice> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    m_waveformTimer.setSingleShot(true);
    connect(&m_waveformTimer, &QTimer::timeout, this, &ElectricBassDialogModel::renderWaveform);
    connectDeviceSignals();
}

ElectricBassDialogModel::~ElectricBassDialogModel() = default;

DeviceDialogModel::DeviceS ElectricBassDialogModel::device() const
{
    return m_device;
}

bool ElectricBassDialogModel::setDevice(DeviceDialogModel::DeviceS device)
{
    if (const auto bass = std::dynamic_pointer_cast<ElectricBassDevice>(device)) {
        setDevice(bass);
        return true;
    }
    return false;
}

void ElectricBassDialogModel::setDevice(std::shared_ptr<ElectricBassDevice> device)
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

QStringList ElectricBassDialogModel::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : ElectricBassPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void ElectricBassDialogModel::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int ElectricBassDialogModel::knob(const QString & key) const
{
    return m_device ? static_cast<int>(std::round(m_device->knob(key) * Constants::uiInternalScaling())) : 0;
}

void ElectricBassDialogModel::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->setKnob(key, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int ElectricBassDialogModel::position() const
{
    return knob(Xml::xmlKeyPosition());
}

void ElectricBassDialogModel::setPosition(int value)
{
    setKnob(Xml::xmlKeyPosition(), value);
}

int ElectricBassDialogModel::finger() const
{
    return knob(Xml::xmlKeyHardness());
}

void ElectricBassDialogModel::setFinger(int value)
{
    setKnob(Xml::xmlKeyHardness(), value);
}

int ElectricBassDialogModel::sustain() const
{
    return knob(Xml::xmlKeySustain());
}

void ElectricBassDialogModel::setSustain(int value)
{
    setKnob(Xml::xmlKeySustain(), value);
}

int ElectricBassDialogModel::brightness() const
{
    return knob(Xml::xmlKeyBrightness());
}

void ElectricBassDialogModel::setBrightness(int value)
{
    setKnob(Xml::xmlKeyBrightness(), value);
}

int ElectricBassDialogModel::release() const
{
    return knob(Xml::xmlKeyRelease());
}

void ElectricBassDialogModel::setRelease(int value)
{
    setKnob(Xml::xmlKeyRelease(), value);
}

int ElectricBassDialogModel::neck() const
{
    return knob(Xml::xmlKeyNeckPickup());
}

void ElectricBassDialogModel::setNeck(int value)
{
    setKnob(Xml::xmlKeyNeckPickup(), value);
}

int ElectricBassDialogModel::bridge() const
{
    return knob(Xml::xmlKeyBridgePickup());
}

void ElectricBassDialogModel::setBridge(int value)
{
    setKnob(Xml::xmlKeyBridgePickup(), value);
}

int ElectricBassDialogModel::tone() const
{
    return knob(Xml::xmlKeyTone());
}

void ElectricBassDialogModel::setTone(int value)
{
    setKnob(Xml::xmlKeyTone(), value);
}

int ElectricBassDialogModel::velocitySensitivity() const
{
    return knob(Xml::xmlKeyVelocitySensitivity());
}

void ElectricBassDialogModel::setVelocitySensitivity(int value)
{
    setKnob(Xml::xmlKeyVelocitySensitivity(), value);
}

int ElectricBassDialogModel::pitchBendRange() const
{
    return m_device ? m_device->pitchBendRange() : 2;
}

void ElectricBassDialogModel::setPitchBendRange(int value)
{
    if (m_device) {
        m_device->setPitchBendRange(value);
    }
}

int ElectricBassDialogModel::lpfCutoff() const
{
    return knob(Xml::xmlKeyLpfCutoff());
}

void ElectricBassDialogModel::setLpfCutoff(int value)
{
    setKnob(Xml::xmlKeyLpfCutoff(), value);
}

int ElectricBassDialogModel::hpfCutoff() const
{
    return knob(Xml::xmlKeyHpfCutoff());
}

void ElectricBassDialogModel::setHpfCutoff(int value)
{
    setKnob(Xml::xmlKeyHpfCutoff(), value);
}

QVariantList ElectricBassDialogModel::waveformData() const
{
    return m_waveformData;
}

double ElectricBassDialogModel::waveformDuration() const
{
    return m_waveformDuration;
}

void ElectricBassDialogModel::setWaveformRequest(int peakCount, bool visible)
{
    const bool changed = peakCount != m_waveformPeakCount || visible != m_waveformVisible;
    m_waveformPeakCount = peakCount;
    m_waveformVisible = visible;
    if (changed) {
        scheduleWaveform();
    }
}

void ElectricBassDialogModel::scheduleWaveform(int delayMs)
{
    if (!m_waveformVisible || m_waveformPeakCount <= 0) {
        m_waveformTimer.stop();
        return;
    }
    m_waveformTimer.setInterval(delayMs);
    m_waveformTimer.start();
}

void ElectricBassDialogModel::renderWaveform()
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

void ElectricBassDialogModel::requestSettings()
{
    scheduleWaveform();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit knobsChanged();
}

} // namespace noteahead
