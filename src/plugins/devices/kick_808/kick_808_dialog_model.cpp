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

#include "kick_808_dialog_model.hpp"

#include "../../../common/constants.hpp"
#include "kick_808_device.hpp"
#include "kick_808_presets.hpp"

#include "../../../application/service/drum_hit_preview.hpp"

#include <cmath>

namespace noteahead {

Kick808DialogModel::Kick808DialogModel(std::shared_ptr<Kick808Device> device, QObject * parent)
  : DeviceDialogModel { parent }
  , m_device { std::move(device) }
{
    namespace Xml = Constants::NahdXml;
    m_waveform = new DrumHitWaveformModel {
        {
          .render = [this](int peakCount) -> DrumHitPreview::Preview {
              if (!m_device) {
                  return {};
              }
              const auto createCopy = [this] { return std::make_unique<Kick808Device>(m_device->name()); };
              const auto bypassEnvelope = [](Device & copy) {
                  auto && kick = static_cast<Kick808Device &>(copy);
                  kick.setKnob(Xml::xmlKeyAmpAttack(), 0.0f);
                  kick.setKnob(Xml::xmlKeyAmpSustain(), 1.0f);
                  kick.setKnob(Xml::xmlKeyAmpRelease(), 0.0f);
              };
              // At the reference note, where the kick sounds at its Tuning with Key Track on or off
              const auto note = static_cast<uint8_t>(Kick808Engine::ReferenceNote);
              return DrumHitPreview::render(*m_device, createCopy, bypassEnvelope, note, peakCount, Constants::defaultSampleRate());
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

Kick808DialogModel::~Kick808DialogModel() = default;

DeviceDialogModel::DeviceS Kick808DialogModel::device() const
{
    return m_device;
}

bool Kick808DialogModel::setDevice(DeviceS device)
{
    if (const auto kick = std::dynamic_pointer_cast<Kick808Device>(device)) {
        setDevice(kick);
        return true;
    }
    return false;
}

int Kick808DialogModel::tune() const
{
    return m_device ? static_cast<int>(std::round(m_device->tune() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setTune(int value)
{
    if (m_device) {
        m_device->setTune(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::tone() const
{
    return m_device ? static_cast<int>(std::round(m_device->tone() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setTone(int value)
{
    if (m_device) {
        m_device->setTone(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::decay() const
{
    return m_device ? static_cast<int>(std::round(m_device->decay() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setDecay(int value)
{
    if (m_device) {
        m_device->setDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::pitchDepth() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchDepth() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setPitchDepth(int value)
{
    if (m_device) {
        m_device->setPitchDepth(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::pitchDecay() const
{
    return m_device ? static_cast<int>(std::round(m_device->pitchDecay() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setPitchDecay(int value)
{
    if (m_device) {
        m_device->setPitchDecay(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::drive() const
{
    return m_device ? static_cast<int>(std::round(m_device->drive() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setDrive(int value)
{
    if (m_device) {
        m_device->setDrive(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::glide() const
{
    return m_device ? static_cast<int>(std::round(m_device->glide() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setGlide(int value)
{
    if (m_device) {
        m_device->setGlide(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

bool Kick808DialogModel::keyTrack() const
{
    return m_device ? m_device->keyTrack() : false;
}

void Kick808DialogModel::setKeyTrack(bool value)
{
    if (m_device) {
        m_device->setKeyTrack(value);
    }
}

int Kick808DialogModel::lpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->lpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setLpfCutoff(int value)
{
    if (m_device) {
        m_device->setLpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::hpfCutoff() const
{
    return m_device ? static_cast<int>(std::round(m_device->hpfCutoff() * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setHpfCutoff(int value)
{
    if (m_device) {
        m_device->setHpfCutoff(static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::knob(const QString & key) const
{
    return m_device ? static_cast<int>(std::round(m_device->knob(key) * Constants::uiInternalScaling())) : 0;
}

void Kick808DialogModel::setKnob(const QString & key, int value)
{
    if (m_device) {
        m_device->setKnob(key, static_cast<float>(value) / Constants::uiInternalScaling());
    }
}

int Kick808DialogModel::ampAttack() const
{
    return knob(Constants::NahdXml::xmlKeyAmpAttack());
}

void Kick808DialogModel::setAmpAttack(int value)
{
    setKnob(Constants::NahdXml::xmlKeyAmpAttack(), value);
}

int Kick808DialogModel::ampHold() const
{
    return knob(Constants::NahdXml::xmlKeyAmpHold());
}

void Kick808DialogModel::setAmpHold(int value)
{
    setKnob(Constants::NahdXml::xmlKeyAmpHold(), value);
}

int Kick808DialogModel::ampDecay() const
{
    return knob(Constants::NahdXml::xmlKeyAmpDecay());
}

void Kick808DialogModel::setAmpDecay(int value)
{
    setKnob(Constants::NahdXml::xmlKeyAmpDecay(), value);
}

int Kick808DialogModel::ampSustain() const
{
    return knob(Constants::NahdXml::xmlKeyAmpSustain());
}

void Kick808DialogModel::setAmpSustain(int value)
{
    setKnob(Constants::NahdXml::xmlKeyAmpSustain(), value);
}

int Kick808DialogModel::ampRelease() const
{
    return knob(Constants::NahdXml::xmlKeyAmpRelease());
}

void Kick808DialogModel::setAmpRelease(int value)
{
    setKnob(Constants::NahdXml::xmlKeyAmpRelease(), value);
}

int Kick808DialogModel::ampCurve() const
{
    return knob(Constants::NahdXml::xmlKeyAmpCurve());
}

void Kick808DialogModel::setAmpCurve(int value)
{
    setKnob(Constants::NahdXml::xmlKeyAmpCurve(), value);
}

QObject * Kick808DialogModel::waveform() const
{
    return m_waveform;
}

QStringList Kick808DialogModel::factoryPresetNames() const
{
    QStringList names;
    for (const auto & preset : Kick808Presets::presets()) {
        names.append(QString::fromStdString(preset.name));
    }
    return names;
}

void Kick808DialogModel::loadFactoryPreset(int index)
{
    if (m_device) {
        m_device->loadPreset(index);
        requestSettings();
    }
}

void Kick808DialogModel::requestSettings()
{
    m_waveform->schedule();
    emit ampChanged();
    emit tuneChanged();
    emit toneChanged();
    emit decayChanged();
    emit pitchDepthChanged();
    emit pitchDecayChanged();
    emit driveChanged();
    emit glideChanged();
    emit keyTrackChanged();
    emit lpfCutoffChanged();
    emit hpfCutoffChanged();
    emit volumeChanged();
    emit gainChanged();
    emit panChanged();
    emit sampleRateChanged();
}

void Kick808DialogModel::setDevice(std::shared_ptr<Kick808Device> device)
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

} // namespace noteahead
