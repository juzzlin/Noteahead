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

#include "piano_synth_acoustic_controller.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/piano_synth_acoustic_device.hpp"
#include "../../domain/devices/piano_synth_acoustic_presets.hpp"

#include <cmath>

namespace noteahead {

namespace {
namespace Xml = Constants::NahdXml;
} // namespace

PianoSynthAcousticController::PianoSynthAcousticController(std::shared_ptr<PianoSynthAcousticDevice> device, QObject * parent)
  : DeviceController { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

PianoSynthAcousticController::~PianoSynthAcousticController() = default;

DeviceController::DeviceS PianoSynthAcousticController::device() const
{
    return m_device;
}

bool PianoSynthAcousticController::setDevice(DeviceController::DeviceS device)
{
    if (const auto piano = std::dynamic_pointer_cast<PianoSynthAcousticDevice>(device)) {
        setDevice(piano);
        return true;
    }
    return false;
}

void PianoSynthAcousticController::setDevice(std::shared_ptr<PianoSynthAcousticDevice> device)
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

QStringList PianoSynthAcousticController::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : PianoSynthAcousticPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void PianoSynthAcousticController::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int PianoSynthAcousticController::knob(const QString & key) const
{
    return m_device ? static_cast<int>(std::round(m_device->knob(key) * Constants::uiInternalScaling())) : 0;
}

void PianoSynthAcousticController::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->setKnob(key, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthAcousticController::hardness() const
{
    return knob(Xml::xmlKeyHardness());
}

void PianoSynthAcousticController::setHardness(int value)
{
    setKnob(Xml::xmlKeyHardness(), value);
}

int PianoSynthAcousticController::knock() const
{
    return knob(Xml::xmlKeyBody());
}

void PianoSynthAcousticController::setKnock(int value)
{
    setKnob(Xml::xmlKeyBody(), value);
}

int PianoSynthAcousticController::decay() const
{
    return knob(Xml::xmlKeyDecay());
}

void PianoSynthAcousticController::setDecay(int value)
{
    setKnob(Xml::xmlKeyDecay(), value);
}

int PianoSynthAcousticController::brightness() const
{
    return knob(Xml::xmlKeyBrightness());
}

void PianoSynthAcousticController::setBrightness(int value)
{
    setKnob(Xml::xmlKeyBrightness(), value);
}

int PianoSynthAcousticController::inharmonicity() const
{
    return knob(Xml::xmlKeyInharmonicity());
}

void PianoSynthAcousticController::setInharmonicity(int value)
{
    setKnob(Xml::xmlKeyInharmonicity(), value);
}

int PianoSynthAcousticController::detune() const
{
    return knob(Xml::xmlKeyStringDetune());
}

void PianoSynthAcousticController::setDetune(int value)
{
    setKnob(Xml::xmlKeyStringDetune(), value);
}

int PianoSynthAcousticController::release() const
{
    return knob(Xml::xmlKeyReleaseTime());
}

void PianoSynthAcousticController::setRelease(int value)
{
    setKnob(Xml::xmlKeyReleaseTime(), value);
}

int PianoSynthAcousticController::resonance() const
{
    return knob(Xml::xmlKeyResonance());
}

void PianoSynthAcousticController::setResonance(int value)
{
    setKnob(Xml::xmlKeyResonance(), value);
}

int PianoSynthAcousticController::panSpread() const
{
    return knob(Xml::xmlKeyPanSpread());
}

void PianoSynthAcousticController::setPanSpread(int value)
{
    setKnob(Xml::xmlKeyPanSpread(), value);
}

int PianoSynthAcousticController::stretch() const
{
    return knob(Xml::xmlKeyStretch());
}

void PianoSynthAcousticController::setStretch(int value)
{
    setKnob(Xml::xmlKeyStretch(), value);
}

int PianoSynthAcousticController::velocitySensitivity() const
{
    return knob(Xml::xmlKeyVelocitySensitivity());
}

void PianoSynthAcousticController::setVelocitySensitivity(int value)
{
    setKnob(Xml::xmlKeyVelocitySensitivity(), value);
}

int PianoSynthAcousticController::lpfCutoff() const
{
    return knob(Xml::xmlKeyLpfCutoff());
}

void PianoSynthAcousticController::setLpfCutoff(int value)
{
    setKnob(Xml::xmlKeyLpfCutoff(), value);
}

int PianoSynthAcousticController::hpfCutoff() const
{
    return knob(Xml::xmlKeyHpfCutoff());
}

void PianoSynthAcousticController::setHpfCutoff(int value)
{
    setKnob(Xml::xmlKeyHpfCutoff(), value);
}

void PianoSynthAcousticController::requestSettings()
{
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit knobsChanged();
}

} // namespace noteahead
