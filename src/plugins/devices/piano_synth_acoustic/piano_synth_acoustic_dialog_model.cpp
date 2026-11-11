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

#include "piano_synth_acoustic_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "piano_synth_acoustic_device.hpp"
#include "piano_synth_acoustic_presets.hpp"

#include <cmath>

namespace noteahead {

namespace {
namespace Xml = Constants::NahdXml;
} // namespace

PianoSynthAcousticDialogModel::PianoSynthAcousticDialogModel(std::shared_ptr<PianoSynthAcousticDevice> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    connectDeviceSignals();
}

PianoSynthAcousticDialogModel::~PianoSynthAcousticDialogModel() = default;

DeviceDialogModel::DeviceS PianoSynthAcousticDialogModel::device() const
{
    return m_device;
}

bool PianoSynthAcousticDialogModel::setDevice(DeviceDialogModel::DeviceS device)
{
    if (const auto piano = std::dynamic_pointer_cast<PianoSynthAcousticDevice>(device)) {
        setDevice(piano);
        return true;
    }
    return false;
}

void PianoSynthAcousticDialogModel::setDevice(std::shared_ptr<PianoSynthAcousticDevice> device)
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

QStringList PianoSynthAcousticDialogModel::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : PianoSynthAcousticPresets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void PianoSynthAcousticDialogModel::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

int PianoSynthAcousticDialogModel::knob(const QString & key) const
{
    return m_device ? static_cast<int>(std::round(m_device->knob(key) * Constants::uiInternalScaling())) : 0;
}

void PianoSynthAcousticDialogModel::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->setKnob(key, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int PianoSynthAcousticDialogModel::hardness() const
{
    return knob(Xml::xmlKeyHardness());
}

void PianoSynthAcousticDialogModel::setHardness(int value)
{
    setKnob(Xml::xmlKeyHardness(), value);
}

int PianoSynthAcousticDialogModel::knock() const
{
    return knob(Xml::xmlKeyBody());
}

void PianoSynthAcousticDialogModel::setKnock(int value)
{
    setKnob(Xml::xmlKeyBody(), value);
}

int PianoSynthAcousticDialogModel::decay() const
{
    return knob(Xml::xmlKeyDecay());
}

void PianoSynthAcousticDialogModel::setDecay(int value)
{
    setKnob(Xml::xmlKeyDecay(), value);
}

int PianoSynthAcousticDialogModel::brightness() const
{
    return knob(Xml::xmlKeyBrightness());
}

void PianoSynthAcousticDialogModel::setBrightness(int value)
{
    setKnob(Xml::xmlKeyBrightness(), value);
}

int PianoSynthAcousticDialogModel::inharmonicity() const
{
    return knob(Xml::xmlKeyInharmonicity());
}

void PianoSynthAcousticDialogModel::setInharmonicity(int value)
{
    setKnob(Xml::xmlKeyInharmonicity(), value);
}

int PianoSynthAcousticDialogModel::detune() const
{
    return knob(Xml::xmlKeyStringDetune());
}

void PianoSynthAcousticDialogModel::setDetune(int value)
{
    setKnob(Xml::xmlKeyStringDetune(), value);
}

int PianoSynthAcousticDialogModel::release() const
{
    return knob(Xml::xmlKeyReleaseTime());
}

void PianoSynthAcousticDialogModel::setRelease(int value)
{
    setKnob(Xml::xmlKeyReleaseTime(), value);
}

int PianoSynthAcousticDialogModel::resonance() const
{
    return knob(Xml::xmlKeyResonance());
}

void PianoSynthAcousticDialogModel::setResonance(int value)
{
    setKnob(Xml::xmlKeyResonance(), value);
}

int PianoSynthAcousticDialogModel::panSpread() const
{
    return knob(Xml::xmlKeyPanSpread());
}

void PianoSynthAcousticDialogModel::setPanSpread(int value)
{
    setKnob(Xml::xmlKeyPanSpread(), value);
}

int PianoSynthAcousticDialogModel::stretch() const
{
    return knob(Xml::xmlKeyStretch());
}

void PianoSynthAcousticDialogModel::setStretch(int value)
{
    setKnob(Xml::xmlKeyStretch(), value);
}

int PianoSynthAcousticDialogModel::velocitySensitivity() const
{
    return knob(Xml::xmlKeyVelocitySensitivity());
}

void PianoSynthAcousticDialogModel::setVelocitySensitivity(int value)
{
    setKnob(Xml::xmlKeyVelocitySensitivity(), value);
}

int PianoSynthAcousticDialogModel::lpfCutoff() const
{
    return knob(Xml::xmlKeyLpfCutoff());
}

void PianoSynthAcousticDialogModel::setLpfCutoff(int value)
{
    setKnob(Xml::xmlKeyLpfCutoff(), value);
}

int PianoSynthAcousticDialogModel::hpfCutoff() const
{
    return knob(Xml::xmlKeyHpfCutoff());
}

void PianoSynthAcousticDialogModel::setHpfCutoff(int value)
{
    setKnob(Xml::xmlKeyHpfCutoff(), value);
}

void PianoSynthAcousticDialogModel::requestSettings()
{
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit knobsChanged();
}

} // namespace noteahead
