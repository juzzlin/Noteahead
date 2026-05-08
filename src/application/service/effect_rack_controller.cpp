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

#include "effect_rack_controller.hpp"
#include "../../domain/dsp/reverb_effect.hpp"

#include <QStringList>

namespace noteahead {

EffectRackController::EffectRackController(std::shared_ptr<DeviceService> deviceService, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
{
}

int EffectRackController::effectCount() const
{
    return static_cast<int>(m_deviceService->effectRack().effectCount());
}

int EffectRackController::revision() const
{
    return m_revision;
}

float EffectRackController::parameterValue(int effectIndex, const QString & paramName) const
{
    if (auto effect = m_deviceService->effectRack().effect(static_cast<size_t>(effectIndex))) {
        if (auto p = effect->parameter(paramName.toStdString()); p) {
            return p->get().value();
        }
    }
    return 0.0f;
}

void EffectRackController::setParameterValue(int effectIndex, const QString & paramName, float value)
{
    if (auto effect = m_deviceService->effectRack().effect(static_cast<size_t>(effectIndex))) {
        if (auto p = effect->parameter(paramName.toStdString()); p) {
            if (p->get().value() != value) {
                p->get().setValue(value);
                
                if (auto reverb = std::dynamic_pointer_cast<ReverbEffect>(effect)) {
                    if (paramName == "reverbSize") reverb->setSize(value);
                    else if (paramName == "reverbDecay") reverb->setDecay(value);
                    else if (paramName == "reverbDamping") reverb->setDamping(value);
                    else if (paramName == "reverbPreDelay") reverb->setPreDelay(value);
                    else if (paramName == "reverbMix") reverb->setMix(value);
                    else if (paramName == "reverbWidth") reverb->setWidth(value);
                }
                
                m_revision++;
                emit revisionChanged();
                emit parameterChanged(effectIndex, paramName);
            }
        }
    }
}

QString EffectRackController::effectType(int effectIndex) const
{
    if (auto effect = m_deviceService->effectRack().effect(static_cast<size_t>(effectIndex))) {
        return QString::fromStdString(effect->type());
    }
    return "";
}

QStringList EffectRackController::reverbPresets() const
{
    return { "Hall", "Large Room", "Small Room", "Plate", "Cathedral", "Basement", "Tunnel", "Spring" };
}

void EffectRackController::applyReverbPreset(int effectIndex, int presetIndex)
{
    if (auto effect = m_deviceService->effectRack().effect(static_cast<size_t>(effectIndex))) {
        if (auto reverb = std::dynamic_pointer_cast<ReverbEffect>(effect)) {
            reverb->applyPreset(static_cast<ReverbEffect::Preset>(presetIndex));
            m_revision++;
            emit revisionChanged();
            emit parameterChanged(effectIndex, ""); // All changed
        }
    }
}

float EffectRackController::deviceSend(const QString & deviceName, int effectIndex) const
{
    if (auto dev = m_deviceService->device(deviceName.toStdString()); dev) {
        return dev->reverbSend(static_cast<size_t>(effectIndex));
    }
    return 0.0f;
}

void EffectRackController::setDeviceSend(const QString & deviceName, int effectIndex, float send)
{
    if (auto dev = m_deviceService->device(deviceName.toStdString()); dev) {
        dev->setReverbSend(static_cast<size_t>(effectIndex), send);
    }
}

} // namespace noteahead
