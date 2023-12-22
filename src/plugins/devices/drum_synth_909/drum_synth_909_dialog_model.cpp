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

#include "drum_synth_909_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "drum_synth_909_device.hpp"

#include <QCoreApplication>
#include <QVariantMap>

#include <cmath>

namespace noteahead {

DrumSynth909DialogModel::DrumSynth909DialogModel(std::shared_ptr<DeviceService> deviceService, QObject * parent)
  : DrumPadDialogModel { std::move(deviceService), parent }
{
    connect(this, &DrumPadDialogModel::selectedVoiceChanged, this, &DrumSynth909DialogModel::voiceKnobsChanged);
    connect(this, &DrumPadDialogModel::selectedVoiceChanged, this, [this] {
        m_knobRevision++;
        emit knobsChanged();
    });
}

bool DrumSynth909DialogModel::acceptsDevice(const DeviceS & device) const
{
    return std::dynamic_pointer_cast<DrumSynth909Device>(device) != nullptr;
}

QVariantList DrumSynth909DialogModel::voiceKnobs() const
{
    QVariantList knobs;
    for (auto && knob : DrumSynth909Device::voiceKnobs(m_selectedVoice)) {
        knobs.push_back(QVariantMap {
          { "key", QString::fromStdString(knob.key) },
          { "label", QCoreApplication::translate("ParameterDisplay", knob.label) },
        });
    }
    return knobs;
}

int DrumSynth909DialogModel::knobRevision() const
{
    return m_knobRevision;
}

int DrumSynth909DialogModel::knob(const QString & key) const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + key.toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 0;
}

void DrumSynth909DialogModel::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, key.toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

QStringList DrumSynth909DialogModel::factoryPresetNames() const
{
    QStringList names;
    for (auto && name : DrumSynth909Device::kitNames()) {
        names.push_back(QString::fromStdString(name));
    }
    return names;
}

void DrumSynth909DialogModel::loadFactoryPreset(int index)
{
    if (const auto device = std::dynamic_pointer_cast<DrumSynth909Device>(m_device); device && index >= 0 && index < static_cast<int>(DrumSynth909Device::kitNames().size())) {
        device->loadKit(static_cast<DrumSynth909Device::Kit>(index));
    }
}

void DrumSynth909DialogModel::requestSettings()
{
    DrumPadDialogModel::requestSettings();
    m_knobRevision++;
    emit knobsChanged();
}

void DrumSynth909DialogModel::retranslate()
{
    DrumPadDialogModel::retranslate();
    emit voiceKnobsChanged();
}

} // namespace noteahead
