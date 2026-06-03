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

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../../domain/devices/auto_panner_effect.hpp"
#include "../../domain/devices/delay_effect.hpp"
#include "../../domain/devices/high_pass_filter_effect.hpp"
#include "../../domain/devices/low_pass_filter_effect.hpp"
#include "../../domain/devices/panner_effect.hpp"
#include "../../domain/devices/panning_effect.hpp"
#include "../../domain/devices/volume_effect.hpp"
#include "../../domain/dsp/clipper_effect.hpp"
#include "../../domain/dsp/compressor_effect.hpp"
#include "../../domain/dsp/eq_8_band_parametric_effect.hpp"
#include "../../domain/dsp/reverb_effect.hpp"
#include "knob_controller.hpp"

#include <QStringList>
#include <QVariantMap>

namespace noteahead {

EffectRackController::EffectRackController(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent)
  : QObject { parent }
  , m_deviceService { std::move(deviceService) }
  , m_editorService { std::move(editorService) }
{
    if (m_deviceService) {
        connect(m_deviceService.get(), &DeviceService::dataChanged, this, [this]() {
            m_revision++;
            emit revisionChanged();
            emit effectCountChanged();
        });
    }
}

int EffectRackController::effectCount() const
{
    if (const auto rack = currentRack()) {
        return static_cast<int>(rack->get().effectCount());
    }
    return 0;
}

int EffectRackController::revision() const
{
    return m_revision;
}

QString EffectRackController::targetDeviceName() const
{
    return m_targetDeviceName;
}

void EffectRackController::setTargetDeviceName(const QString & name)
{
    m_targetDeviceName = name;
    emit targetDeviceNameChanged();
    m_revision++;
    emit revisionChanged();
    emit effectCountChanged();
}

bool EffectRackController::isInsertRack() const
{
    return m_isInsertRack;
}

void EffectRackController::setIsInsertRack(bool isInsert)
{
    m_isInsertRack = isInsert;
    emit isInsertRackChanged();
    m_revision++;
    emit revisionChanged();
    emit effectCountChanged();
}

std::optional<std::reference_wrapper<EffectRack>> EffectRackController::currentRack() const
{
    if (m_targetDeviceName.isEmpty()) {
        if (m_isInsertRack) {
            return std::ref(m_deviceService->insertEffectRack());
        } else {
            return std::ref(m_deviceService->sendEffectRack());
        }
    } else {
        if (const auto device = m_deviceService->device(m_targetDeviceName.toStdString())) {
            return std::ref(device->insertEffectRack());
        }
    }
    return std::nullopt;
}

float EffectRackController::parameterValue(int effectIndex, const QString & paramName) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex))) {
            if (const auto parameter = effect->parameter(paramName.toStdString()); parameter) {
                return parameter->get().value();
            }
        }
    }
    return 0.0f;
}

void EffectRackController::setParameterValue(int effectIndex, const QString & paramName, float value)
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex))) {
            if (const auto parameter = effect->parameter(paramName.toStdString()); parameter) {
                if (parameter->get().update(value)) {
                    effect->sync();

                    m_editorService->setIsModified(true);
                    m_revision++;
                    emit revisionChanged();
                    emit parameterChanged(effectIndex, paramName);
                }
            }
        }
    }
}

void EffectRackController::setEffect(int slotIndex, const QString & typeId)
{
    const auto stdTypeId = typeId.toStdString();
    EffectRack::EffectS effect;
    if (stdTypeId == ReverbEffect::typeIdString()) {
        effect = std::make_shared<ReverbEffect>();
    } else if (stdTypeId == DelayEffect::typeIdString()) {
        effect = std::make_shared<DelayEffect>();
    } else if (stdTypeId == AutoPannerEffect::typeIdString()) {
        effect = std::make_shared<AutoPannerEffect>();
    } else if (stdTypeId == HighPassFilterEffect::typeIdString()) {
        effect = std::make_shared<HighPassFilterEffect>();
    } else if (stdTypeId == LowPassFilterEffect::typeIdString()) {
        effect = std::make_shared<LowPassFilterEffect>();
    } else if (stdTypeId == PannerEffect::typeIdString()) {
        effect = std::make_shared<PannerEffect>();
    } else if (stdTypeId == PanningEffect::typeIdString()) {
        effect = std::make_shared<PanningEffect>();
    } else if (stdTypeId == VolumeEffect::typeIdString()) {
        effect = std::make_shared<VolumeEffect>();
    } else if (stdTypeId == CompressorEffect::typeIdString()) {
        effect = std::make_shared<CompressorEffect>();
    } else if (stdTypeId == ClipperEffect::typeIdString()) {
        effect = std::make_shared<ClipperEffect>();
    } else if (stdTypeId == Eq8BandParametricEffect::typeIdString()) {
        effect = std::make_shared<Eq8BandParametricEffect>();
    }

    if (effect) {
        if (const auto rack = currentRack()) {
            rack->get().setEffect(static_cast<size_t>(slotIndex), std::move(effect));
            m_editorService->setIsModified(true);
            m_revision++;
            emit revisionChanged();
        }
    }
}

void EffectRackController::clearEffect(int slotIndex)
{
    if (const auto rack = currentRack()) {
        rack->get().setEffect(static_cast<size_t>(slotIndex), nullptr);
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

QVariantList EffectRackController::availableEffects() const
{
    QVariantList list;

    auto addEffect = [&](const QString & name, const std::string & typeId) {
        QVariantMap map;
        map["name"] = name;
        map["typeId"] = QString::fromStdString(typeId);
        list.append(map);
    };

    addEffect("Auto Panner", AutoPannerEffect::typeIdString());
    addEffect("Clipper", ClipperEffect::typeIdString());
    addEffect("Compressor", CompressorEffect::typeIdString());
    addEffect("EQ 8-Band Parametric", Eq8BandParametricEffect::typeIdString());
    addEffect("Panner", PannerEffect::typeIdString());
    addEffect("Reverb", ReverbEffect::typeIdString());

    return list;
}

bool EffectRackController::isEffectEnabled(int effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex))) {
            return effect->enabled();
        }
    }
    return false;
}

void EffectRackController::setIsEffectEnabled(int effectIndex, bool enabled)
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex))) {
            if (effect->enabled() != enabled) {
                effect->setEnabled(enabled);
                m_editorService->setIsModified(true);
                m_revision++;
                emit revisionChanged();
            }
        }
    }
}

QStringList EffectRackController::parameterNames(int effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex))) {
            QStringList names;
            for (const auto & name : effect->parameterNames()) {
                names.append(QString::fromStdString(name));
            }
            return names;
        }
    }
    return {};
}

QString EffectRackController::effectType(int effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex))) {
            return QString::fromStdString(effect->type());
        }
    }
    return "";
}

QString EffectRackController::effectParametersSummary(int effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex))) {
            const auto type = QString::fromStdString(effect->type());
            if (type == Constants::RackEffectType::reverb()) {
                auto preDelay = effect->parameter(Constants::NahdXml::xmlKeyReverbPreDelay().toStdString());
                auto decay = effect->parameter(Constants::NahdXml::xmlKeyReverbDecay().toStdString());
                if (preDelay && decay) {
                    return QString { "(pre=%1ms, decay=%2ms)" }
                      .arg(preDelay->get().xmlValue())
                      .arg(decay->get().xmlValue());
                }
            } else if (type == Constants::RackEffectType::compressor()) {
                auto attack = effect->parameter(Constants::NahdXml::xmlKeyAttack().toStdString());
                auto ratio = effect->parameter(Constants::NahdXml::xmlKeyCompressorRatio().toStdString());
                if (attack && ratio) {
                    const float attackMs = static_cast<float>(ParameterMapper::mapExponential(attack->get().value(), 0.1, 500.0));
                    const int ratioValue = ratio->get().xmlValue();
                    return QString { "(attack=%1ms, ratio=%2:1)" }
                      .arg(attackMs, 0, 'f', 1)
                      .arg(ratioValue);
                }
            } else if (type == Constants::RackEffectType::autoPanner()) {
                const auto sync = effect->parameter(Constants::NahdXml::xmlKeySync().toStdString());
                const auto intensity = effect->parameter(Constants::NahdXml::xmlKeyIntensity().toStdString());
                if (sync && intensity) {
                    QString rateStr;
                    if (sync->get().value() > 0.5f) {
                        const auto division = effect->parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString());
                        KnobController knobController;
                        rateStr = knobController.syncLabel(knobController.syncIndex(division->get().value() * Constants::uiInternalScaling()));
                    } else {
                        const auto rate = effect->parameter(Constants::NahdXml::xmlKeyRate().toStdString());
                        const float rateHz = static_cast<float>(ParameterMapper::mapExponential(rate->get().value(), 0.05, 20.0));
                        rateStr = QString { "%1Hz" }.arg(rateHz, 0, 'f', 2);
                    }
                    return QString { "(rate=%1, int=%2%)" }
                      .arg(rateStr)
                      .arg(static_cast<int>(std::round(intensity->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::panner()) {
                auto pan = effect->parameter(Constants::NahdXml::xmlKeyPan().toStdString());
                auto width = effect->parameter(Constants::NahdXml::xmlKeyReverbWidth().toStdString());
                if (pan && width) {
                    return QString { "(pan=%1%, width=%2%)" }
                      .arg(static_cast<int>(std::round(pan->get().value() * 100.0f)))
                      .arg(static_cast<int>(std::round(width->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::clipper()) {
                auto threshold = effect->parameter(Constants::NahdXml::xmlKeyClipperThreshold().toStdString());
                if (threshold) {
                    return QString { "(thr=%1dB)" }.arg(threshold->get().xmlValue() / 100.0f, 0, 'f', 1);
                }
            } else if (type == Constants::RackEffectType::eq8BandParametric()) {
                return "(Parametric)";
            }
        }
    }
    return {};
}

QString EffectRackController::reverbSizeKey() const
{
    return Constants::NahdXml::xmlKeyReverbSize();
}

QString EffectRackController::reverbDecayKey() const
{
    return Constants::NahdXml::xmlKeyReverbDecay();
}

QString EffectRackController::reverbDampingKey() const
{
    return Constants::NahdXml::xmlKeyReverbDamping();
}

QString EffectRackController::reverbPreDelayKey() const
{
    return Constants::NahdXml::xmlKeyReverbPreDelay();
}

QString EffectRackController::reverbWidthKey() const
{
    return Constants::NahdXml::xmlKeyReverbWidth();
}

QString EffectRackController::reverbLpfCutoffKey() const
{
    return Constants::NahdXml::xmlKeyReverbLpfCutoff();
}

QString EffectRackController::reverbHpfCutoffKey() const
{
    return Constants::NahdXml::xmlKeyReverbHpfCutoff();
}

QString EffectRackController::reverbMixKey() const
{
    return Constants::NahdXml::xmlKeyReverbMix();
}

QString EffectRackController::compressorThresholdKey() const
{
    return Constants::NahdXml::xmlKeyCompressorThreshold();
}

QString EffectRackController::compressorRatioKey() const
{
    return Constants::NahdXml::xmlKeyCompressorRatio();
}

QString EffectRackController::compressorAttackKey() const
{
    return Constants::NahdXml::xmlKeyAttack();
}

QString EffectRackController::compressorReleaseKey() const
{
    return Constants::NahdXml::xmlKeyRelease();
}

QString EffectRackController::compressorKneeKey() const
{
    return Constants::NahdXml::xmlKeyCompressorKnee();
}

QString EffectRackController::compressorMakeupKey() const
{
    return Constants::NahdXml::xmlKeyCompressorMakeup();
}

QString EffectRackController::compressorLookaheadKey() const
{
    return Constants::NahdXml::xmlKeyLookahead();
}

QString EffectRackController::clipperModeKey() const
{
    return Constants::NahdXml::xmlKeyClipperMode();
}

QString EffectRackController::clipperThresholdKey() const
{
    return Constants::NahdXml::xmlKeyClipperThreshold();
}

QString EffectRackController::clipperGainKey() const
{
    return Constants::NahdXml::xmlKeyClipperGain();
}

QString EffectRackController::eq8BandParametricTypeKey(int bandIndex) const
{
    return Constants::NahdXml::xmlKeyEq8BandParametricType(bandIndex);
}

QString EffectRackController::eq8BandParametricFreqKey(int bandIndex) const
{
    return Constants::NahdXml::xmlKeyEq8BandParametricFreq(bandIndex);
}

QString EffectRackController::eq8BandParametricGainKey(int bandIndex) const
{
    return Constants::NahdXml::xmlKeyEq8BandParametricGain(bandIndex);
}

QString EffectRackController::eq8BandParametricQKey(int bandIndex) const
{
    return Constants::NahdXml::xmlKeyEq8BandParametricQ(bandIndex);
}

QString EffectRackController::clipperType() const
{
    return Constants::RackEffectType::clipper();
}

QString EffectRackController::compressorType() const
{
    return Constants::RackEffectType::compressor();
}

QString EffectRackController::delayType() const
{
    return Constants::RackEffectType::delay();
}

QString EffectRackController::eq8BandParametricType() const
{
    return Constants::RackEffectType::eq8BandParametric();
}

QString EffectRackController::pannerType() const
{
    return Constants::RackEffectType::panner();
}

QString EffectRackController::autoPannerType() const
{
    return Constants::RackEffectType::autoPanner();
}

QString EffectRackController::reverbType() const
{
    return Constants::RackEffectType::reverb();
}

float EffectRackController::compressorReductionDb(int effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex))) {
            if (const auto compressor = std::dynamic_pointer_cast<CompressorEffect>(effect)) {
                return compressor->reductionDb();
            }
        }
    }
    return 0.0f;
}

QStringList EffectRackController::reverbPresets() const
{
    QStringList presets;
    for (const auto & name : ReverbEffect::presetNames()) {
        presets.append(QString::fromStdString(name));
    }
    return presets;
}

void EffectRackController::applyReverbPreset(int effectIndex, int presetIndex)
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex))) {
            if (const auto reverb = std::dynamic_pointer_cast<ReverbEffect>(effect)) {
                const auto presetNames = ReverbEffect::presetNames();
                if (presetIndex >= 0 && presetIndex < static_cast<int>(presetNames.size())) {
                    reverb->applyPreset(ReverbEffect::stringToPreset(presetNames[presetIndex]));
                    m_editorService->setIsModified(true);
                    m_revision++;
                    emit revisionChanged();
                    emit parameterChanged(effectIndex, ""); // Notify all parameters changed
                }
            }
        }
    }
}

float EffectRackController::deviceSend(const QString & deviceName, int effectIndex) const
{
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        return device->reverbSend(static_cast<size_t>(effectIndex));
    }
    return 0.0f;
}

void EffectRackController::setDeviceSend(const QString & deviceName, int effectIndex, float send)
{
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        device->setReverbSend(static_cast<size_t>(effectIndex), send);
    }
}

} // namespace noteahead
