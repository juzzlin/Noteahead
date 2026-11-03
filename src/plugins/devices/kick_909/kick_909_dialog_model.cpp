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

#include "../../../application/service/drum_hit_preview.hpp"

#include <cmath>

namespace noteahead {

namespace {
namespace Xml = Constants::NahdXml;
} // namespace

Kick909DialogModel::Kick909DialogModel(std::shared_ptr<Kick909Device> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    m_waveform = new DrumHitWaveformModel {
        {
          .render = [this](int peakCount) -> DrumHitPreview::Preview {
              if (!m_device) {
                  return {};
              }
              const auto createCopy = [this] { return std::make_unique<Kick909Device>(m_device->name()); };
              const auto bypassEnvelope = [](Device & copy) {
                  auto && kick = static_cast<Kick909Device &>(copy);
                  kick.setKnob(Xml::xmlKeyAmpAttack(), 0.0f);
                  kick.setKnob(Xml::xmlKeyAmpSustain(), 1.0f);
                  kick.setKnob(Xml::xmlKeyAmpRelease(), 0.0f);
              };
              return DrumHitPreview::render(*m_device, createCopy, bypassEnvelope, Kick909Device::RootNote, peakCount, Constants::defaultSampleRate());
          },
          .envelope = [this]() -> DrumHitWaveformModel::Envelope {
              if (!m_device) {
                  return {};
              }
              return {
                  .attack = m_device->ampAttackSeconds(),
                  .hold = m_device->ampHoldSeconds(),
                  .decay = m_device->ampDecaySeconds(),
                  .sustain = static_cast<double>(m_device->knob(Xml::xmlKeyAmpSustain())),
                  .release = m_device->ampReleaseSeconds(),
                  .curve = static_cast<double>(m_device->knob(Xml::xmlKeyAmpCurve())),
              };
          },
          .elapsedSeconds = [this]() -> std::optional<double> {
              return m_device ? m_device->elapsedSeconds() : std::nullopt;
          },
        },
        this
    };
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

QObject * Kick909DialogModel::waveform() const
{
    return m_waveform;
}

void Kick909DialogModel::requestSettings()
{
    m_waveform->schedule();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit knobsChanged();
}

} // namespace noteahead
