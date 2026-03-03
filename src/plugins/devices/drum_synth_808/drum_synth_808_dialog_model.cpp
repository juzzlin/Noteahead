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

#include "drum_synth_808_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "drum_synth_808_device.hpp"

#include <QCoreApplication>
#include <QVariantMap>

#include <cmath>

namespace noteahead {

DrumSynth808DialogModel::DrumSynth808DialogModel(std::shared_ptr<DeviceService> deviceService, QObject * parent)
  : DrumPadDialogModel { std::move(deviceService), parent }
{
    connect(this, &DrumPadDialogModel::selectedVoiceChanged, this, &DrumSynth808DialogModel::voiceKnobsChanged);
    connect(this, &DrumPadDialogModel::selectedVoiceChanged, this, [this] {
        m_knobRevision++;
        emit knobsChanged();
    });
}

bool DrumSynth808DialogModel::acceptsDevice(const DeviceS & device) const
{
    return std::dynamic_pointer_cast<DrumSynth808Device>(device) != nullptr;
}

QVariantList DrumSynth808DialogModel::voiceKnobs() const
{
    QVariantList knobs;
    for (auto && knob : DrumSynth808Device::voiceKnobs(m_selectedVoice)) {
        knobs.push_back(QVariantMap {
          { "key", QString::fromStdString(knob.key) },
          // A switch is named after the drum it switches to, which stays in English
          { "label", knob.isSwitch ? QString { knob.label } : QCoreApplication::translate("ParameterDisplay", knob.label) },
          { "isSwitch", knob.isSwitch },
        });
    }
    return knobs;
}

int DrumSynth808DialogModel::knobRevision() const
{
    return m_knobRevision;
}

int DrumSynth808DialogModel::knob(const QString & key) const
{
    if (!m_device) {
        return 0;
    }
    const auto parameter = m_device->parameter(currentVoicePrefix() + key.toStdString());
    return parameter ? static_cast<int>(std::round(parameter->get().shownValue() * Constants::uiInternalScaling())) : 0;
}

void DrumSynth808DialogModel::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, key.toStdString(), static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool DrumSynth808DialogModel::voiceSwitched(int index) const
{
    if (!m_device) {
        return false;
    }
    const auto parameter = m_device->parameter(DrumSynthV2::voiceId(index) + "_" + Constants::NahdXml::xmlKeyVoiceSwitch().toStdString());
    return parameter && parameter->get().value() >= 0.5f;
}

void DrumSynth808DialogModel::setVoiceSwitched(bool switched)
{
    if (m_device) {
        m_device->updateVoiceParameter(m_selectedVoice, Constants::NahdXml::xmlKeyVoiceSwitch().toStdString(), switched ? 1.0f : 0.0f);
    }
}

QStringList DrumSynth808DialogModel::factoryPresetNames() const
{
    QStringList names;
    for (auto && name : DrumSynth808Device::kitNames()) {
        names.push_back(QString::fromStdString(name));
    }
    return names;
}

void DrumSynth808DialogModel::loadFactoryPreset(int index)
{
    if (const auto device = std::dynamic_pointer_cast<DrumSynth808Device>(m_device); device && index >= 0 && index < static_cast<int>(DrumSynth808Device::kitNames().size())) {
        device->loadKit(static_cast<DrumSynth808Device::Kit>(index));
    }
}

void DrumSynth808DialogModel::requestSettings()
{
    DrumPadDialogModel::requestSettings();
    m_knobRevision++;
    emit knobsChanged();
}

void DrumSynth808DialogModel::retranslate()
{
    DrumPadDialogModel::retranslate();
    emit voiceKnobsChanged();
}

} // namespace noteahead
