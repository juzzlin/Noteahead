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
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/devices/sampler_device.hpp"
#include "../../domain/effects/all_pass_filter.hpp"
#include "../../domain/effects/auto_ducker.hpp"
#include "../../domain/effects/auto_panner.hpp"
#include "../../domain/effects/chorus.hpp"
#include "../../domain/effects/clipper.hpp"
#include "../../domain/effects/compressor.hpp"
#include "../../domain/effects/delay.hpp"
#include "../../domain/effects/drive.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/eq_8_band_parametric.hpp"
#include "../../domain/effects/limiter.hpp"
#include "../../domain/effects/panner.hpp"
#include "../../domain/effects/reverb.hpp"
#include "../../domain/effects/saturator.hpp"
#include "../../domain/effects/stereo_enhancer.hpp"
#include "../../domain/effects/tube_stage.hpp"
#include "../../domain/effects/wave_designer.hpp"
#include "../../domain/utility/dbtp_meter.hpp"
#include "../../domain/utility/lufs_meter.hpp"
#include "../../domain/utility/rta.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"
#include "knob_controller.hpp"

#include <QDateTime>
#include <QFile>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

namespace noteahead {

static constexpr float dbtpFloor = -70.0f;

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
    emit rackEnabledChanged();
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
    emit rackEnabledChanged();
    m_revision++;
    emit revisionChanged();
    emit effectCountChanged();
}

int EffectRackController::targetSubIndex() const
{
    return m_targetSubIndex;
}

void EffectRackController::setTargetSubIndex(int index)
{
    m_targetSubIndex = index;
    emit targetSubIndexChanged();
    emit rackEnabledChanged();
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
        if (const auto device = m_deviceService->device(m_targetDeviceName.toStdString()); device) {
            if (m_targetSubIndex >= 0) {
                if (const auto sampler = std::dynamic_pointer_cast<SamplerDevice>(device)) {
                    return std::ref(sampler->sampleEffectRack(static_cast<uint8_t>(m_targetSubIndex)));
                }
                if (const auto drum = std::dynamic_pointer_cast<DrumSynthDevice>(device)) {
                    return std::ref(drum->voiceEffectRack(m_targetSubIndex));
                }
                return std::nullopt;
            }
            return std::ref(device->insertEffectRack());
        }
    }
    return std::nullopt;
}

float EffectRackController::parameterValue(quint32 effectIndex, const QString & paramName) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            if (const auto parameter = effect->parameter(paramName.toStdString()); parameter) {
                return parameter->get().value();
            }
        }
    }
    return 0.0f;
}

void EffectRackController::setParameterValue(quint32 effectIndex, const QString & paramName, float value)
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
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
    if (const auto effect = EffectFactory::createEffect(typeId.toStdString()); effect) {
        if (const auto rack = currentRack(); rack) {
            rack->get().setEffect(static_cast<size_t>(slotIndex), std::move(effect));
            m_editorService->setIsModified(true);
            m_revision++;
            emit revisionChanged();
        }
    }
}

void EffectRackController::clearEffect(int slotIndex)
{
    if (const auto rack = currentRack(); rack) {
        rack->get().setEffect(static_cast<size_t>(slotIndex), nullptr);
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

void EffectRackController::moveEffectUp(int index)
{
    if (index <= 0) {
        return;
    }
    if (const auto rack = currentRack(); rack) {
        rack->get().swapEffects(static_cast<size_t>(index), static_cast<size_t>(index - 1));
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

void EffectRackController::moveEffectDown(int index)
{
    if (const auto rack = currentRack(); rack) {
        if (index >= static_cast<int>(rack->get().effectCount()) - 1) {
            return;
        }
        rack->get().swapEffects(static_cast<size_t>(index), static_cast<size_t>(index + 1));
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

void EffectRackController::moveEffectToTop(int index)
{
    if (index <= 0) {
        return;
    }
    if (const auto rack = currentRack(); rack) {
        rack->get().moveEffect(static_cast<size_t>(index), 0);
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

void EffectRackController::moveEffectToBottom(int index)
{
    if (const auto rack = currentRack(); rack) {
        const auto lastIndex = static_cast<int>(rack->get().effectCount()) - 1;
        if (index < 0 || index >= lastIndex) {
            return;
        }
        rack->get().moveEffect(static_cast<size_t>(index), static_cast<size_t>(lastIndex));
        m_editorService->setIsModified(true);
        m_revision++;
        emit revisionChanged();
    }
}

QVariantList EffectRackController::availableEffects() const
{
    QVariantList list;

    const auto addEffect = [&](const QString & name, const std::string & typeId) {
        QVariantMap map;
        map["name"] = name;
        map["typeId"] = QString::fromStdString(typeId);
        list.append(map);
    };

    addEffect("All-Pass Filter", AllPassFilter::typeIdString());
    addEffect("Auto Ducker", Constants::RackEffectType::autoDucker().toStdString());
    addEffect("Auto Panner", Constants::RackEffectType::autoPanner().toStdString());
    addEffect("Endless Reverb", Constants::RackEffectType::endless().toStdString());
    addEffect("Chorus", Chorus::typeIdString());
    addEffect("Clipper", Constants::RackEffectType::clipper().toStdString());
    addEffect("Compressor", Constants::RackEffectType::compressor().toStdString());
    addEffect("dBTP Meter", DbTpMeter::typeIdString());
    addEffect("Delay", Constants::RackEffectType::delay().toStdString());
    addEffect("Drive", Constants::RackEffectType::drive().toStdString());
    addEffect("EQ 8-Band Parametric", Constants::RackEffectType::eq8BandParametric().toStdString());
    addEffect("Vintage Passive EQ", Constants::RackEffectType::vintagePassiveEq().toStdString());
    addEffect("Air Band EQ", Constants::RackEffectType::airBandEq().toStdString());
    addEffect("Simple EQ", Constants::RackEffectType::simpleEq().toStdString());
    addEffect("Limiter", Constants::RackEffectType::limiter().toStdString());
    addEffect("LUFS Meter", LufsMeter::typeIdString());
    addEffect("Panner", Constants::RackEffectType::panner().toStdString());
    addEffect("Reverb", Constants::RackEffectType::reverb().toStdString());
    addEffect("RTA", Constants::RackEffectType::rta().toStdString());
    addEffect("Saturator", Constants::RackEffectType::saturator().toStdString());
    addEffect("Tube Stage", Constants::RackEffectType::tubeStage().toStdString());
    addEffect("Wave Designer", Constants::RackEffectType::waveDesigner().toStdString());
    addEffect("Stereo Enhancer", Constants::RackEffectType::stereoEnhancer().toStdString());

    return list;
}

void EffectRackController::copyEffect(int sourceSlot, int targetSlot)
{
    if (const auto rack = currentRack(); rack) {
        if (rack->get().copyEffect(static_cast<size_t>(sourceSlot), static_cast<size_t>(targetSlot))) {
            m_editorService->setIsModified(true);
            m_revision++;
            emit revisionChanged();
        }
    }
}

QVariantList EffectRackController::populatedEffects() const
{
    QVariantList list;
    if (const auto rack = currentRack(); rack) {
        const auto count = rack->get().effectCount();
        for (size_t i = 0; i < count; i++) {
            if (const auto effect = rack->get().effect(i)) {
                QVariantMap map;
                map["slotIndex"] = static_cast<int>(i);
                map["name"] = effectDisplayName(QString::fromStdString(effect->type()));
                list.append(map);
            }
        }
    }
    return list;
}

bool EffectRackController::isEffectEnabled(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex))) {
            return effect->enabled();
        }
    }
    return false;
}

void EffectRackController::setIsEffectEnabled(quint32 effectIndex, bool enabled)
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            if (effect->enabled() != enabled) {
                effect->setEnabled(enabled);
                m_editorService->setIsModified(true);
                m_revision++;
                emit revisionChanged();
            }
        }
    }
}

bool EffectRackController::rackEnabled() const
{
    if (const auto rack = currentRack(); rack) {
        return rack->get().enabled();
    }
    return true;
}

void EffectRackController::setRackEnabled(bool enabled)
{
    if (const auto rack = currentRack(); rack) {
        if (rack->get().enabled() != enabled) {
            rack->get().setEnabled(enabled);
            m_editorService->setIsModified(true);
            emit rackEnabledChanged();
            m_revision++;
            emit revisionChanged();
        }
    }
}

QStringList EffectRackController::parameterNames(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            QStringList names;
            for (const auto & name : effect->parameterNames()) {
                names.append(QString::fromStdString(name));
            }
            return names;
        }
    }
    return {};
}

QString EffectRackController::effectType(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            return QString::fromStdString(effect->type());
        }
    }
    return "";
}

QString EffectRackController::effectDisplayName(const QString & typeId) const
{
    for (const auto & item : availableEffects()) {
        const auto map = item.toMap();
        if (map["typeId"].toString() == typeId) {
            return map["name"].toString();
        }
    }
    return typeId;
}

QString EffectRackController::effectParametersSummary(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            const auto type = QString::fromStdString(effect->type());
            if (type == Constants::RackEffectType::allPassFilter()) {
                const auto freq = effect->parameter(Constants::NahdXml::xmlKeyFrequency().toStdString());
                const auto stages = effect->parameter(Constants::NahdXml::xmlKeyStages().toStdString());
                if (freq && stages) {
                    return QString { "(freq=%1Hz, stages=%2)" }
                      .arg(freq->get().xmlValue())
                      .arg(stages->get().xmlValue());
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
            } else if (type == Constants::RackEffectType::chorus()) {
                const auto rate = effect->parameter(Constants::NahdXml::xmlKeyRate().toStdString());
                const auto mix = effect->parameter(Constants::NahdXml::xmlKeyMix().toStdString());
                if (rate && mix) {
                    const float rateHz = static_cast<float>(ParameterMapper::mapExponential(rate->get().value(), 0.1, 10.0));
                    return QString { "(rate=%1Hz, mix=%2%)" }
                      .arg(rateHz, 0, 'f', 1)
                      .arg(static_cast<int>(std::round(mix->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::clipper()) {
                if (const auto threshold = effect->parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()); threshold) {
                    return QString { "(thr=%1dB)" }.arg(threshold->get().xmlValue() / 100.0f, 0, 'f', 1);
                }
            } else if (type == Constants::RackEffectType::limiter()) {
                const auto ceiling = effect->parameter(Constants::NahdXml::xmlKeyCeiling().toStdString());
                const auto boost = effect->parameter(Constants::NahdXml::xmlKeyBoost().toStdString());
                if (ceiling && boost) {
                    return QString { "(ceil=%1dB, boost=%2)" }
                      .arg(ceiling->get().xmlValue() / 100.0f, 0, 'f', 1)
                      .arg(boost->get().value() > 0.5f ? tr("on") : tr("off"));
                }
            } else if (type == Constants::RackEffectType::compressor()) {
                const auto attack { effect->parameter(Constants::NahdXml::xmlKeyAttack().toStdString()) };
                const auto ratio { effect->parameter(Constants::NahdXml::xmlKeyRatio().toStdString()) };
                if (attack && ratio) {
                    const float attackMs { static_cast<float>(ParameterMapper::mapExponential(attack->get().value(), 0.1, 500.0)) };
                    const float ratioValue { static_cast<float>(ratio->get().xmlValue()) / static_cast<float>(ratio->get().xmlScale()) };
                    QString scName { tr("None") };
                    if (const auto sourceIndex { effect->sidechainSourceDeviceIndex() }; sourceIndex) {
                        if (const auto sourceDevice { m_deviceService->device(*sourceIndex) }) {
                            scName = QString::fromStdString(sourceDevice->name());
                        }
                    }
                    return QString { "(attack=%1ms, ratio=%2:1, sidechain=%3)" }
                      .arg(attackMs, 0, 'f', 1)
                      .arg(ratioValue, 0, 'g', 3)
                      .arg(scName);
                }
            } else if (type == Constants::RackEffectType::autoDucker()) {
                const auto amount { effect->parameter(Constants::NahdXml::xmlKeyAmount().toStdString()) };
                const auto threshold { effect->parameter(Constants::NahdXml::xmlKeyThreshold().toStdString()) };
                if (amount && threshold) {
                    QString scName { tr("None") };
                    if (const auto sourceIndex { effect->sidechainSourceDeviceIndex() }; sourceIndex) {
                        if (const auto sourceDevice { m_deviceService->device(*sourceIndex) }) {
                            scName = QString::fromStdString(sourceDevice->name());
                        }
                    }
                    return QString { "(amount=%1dB, thr=%2dB, sidechain=%3)" }
                      .arg(amount->get().xmlValue() / 100.0f, 0, 'f', 1)
                      .arg(threshold->get().xmlValue() / 100.0f, 0, 'f', 1)
                      .arg(scName);
                }
            } else if (type == Constants::RackEffectType::delay()) {
                const auto sync = effect->parameter(Constants::NahdXml::xmlKeyDelaySync().toStdString());
                const auto feedback = effect->parameter(Constants::NahdXml::xmlKeyDelayFeedback().toStdString());
                if (sync && feedback) {
                    QString timeStr;
                    if (sync->get().value() > 0.5f) {
                        const auto division = effect->parameter(Constants::NahdXml::xmlKeyDelaySyncDivision().toStdString());
                        KnobController knobController;
                        timeStr = knobController.syncLabel(knobController.syncIndex(division->get().value() * Constants::uiInternalScaling()));
                    } else {
                        const auto time = effect->parameter(Constants::NahdXml::xmlKeyDelayTime().toStdString());
                        timeStr = QString { "%1ms" }.arg(static_cast<int>(std::round(time->get().value() * 10000.0f)));
                    }
                    return QString { "(time=%1, fb=%2%)" }
                      .arg(timeStr)
                      .arg(static_cast<int>(std::round(feedback->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::lufsMeter()) {
                if (const auto meter = std::dynamic_pointer_cast<LufsMeter>(effect)) {
                    const auto fmt = [](float v) { return v <= -70.0f ? QString("-∞") : QString("%1").arg(v, 0, 'f', 1); };
                    return QString { "(M=%1 S=%2 LUFS)" }.arg(fmt(meter->momentaryLufs())).arg(fmt(meter->shortTermLufs()));
                }
            } else if (type == Constants::RackEffectType::dbtpMeter()) {
                if (const auto meter = std::dynamic_pointer_cast<DbTpMeter>(effect)) {
                    const auto fmt = [](float v) { return v <= -70.0f ? QString("-∞") : QString("%1").arg(v, 0, 'f', 1); };
                    return QString { "(L=%1 R=%2 dBTP)" }.arg(fmt(meter->truePeakHoldL())).arg(fmt(meter->truePeakHoldR()));
                }
            } else if (type == Constants::RackEffectType::eq8BandParametric()) {
                return "(Parametric)";
            } else if (type == Constants::RackEffectType::vintagePassiveEq()) {
                return "(Passive)";
            } else if (type == Constants::RackEffectType::airBandEq()) {
                return "(Air)";
            } else if (type == Constants::RackEffectType::simpleEq()) {
                if (const auto amount = effect->parameter(Constants::NahdXml::xmlKeyAmount().toStdString()); amount) {
                    return QString { "(%1%)" }.arg(static_cast<int>(std::round(amount->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::panner()) {
                const auto pan = effect->parameter(Constants::NahdXml::xmlKeyPan().toStdString());
                const auto width = effect->parameter(Constants::NahdXml::xmlKeyWidth().toStdString());
                if (pan && width) {
                    return QString { "(pan=%1%, width=%2%)" }
                      .arg(static_cast<int>(std::round(pan->get().value() * 100.0f)))
                      .arg(static_cast<int>(std::round(width->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::reverb()) {
                const auto preDelay = effect->parameter(Constants::NahdXml::xmlKeyPreDelay().toStdString());
                const auto decay = effect->parameter(Constants::NahdXml::xmlKeyDecay().toStdString());
                if (preDelay && decay) {
                    return QString { "(pre=%1ms, decay=%2ms)" }
                      .arg(preDelay->get().xmlValue() / preDelay->get().xmlScale())
                      .arg(decay->get().xmlValue() / decay->get().xmlScale());
                }
            } else if (type == Constants::RackEffectType::tubeStage()) {
                const auto drive = effect->parameter(Constants::NahdXml::xmlKeyDrive().toStdString());
                const auto bias = effect->parameter(Constants::NahdXml::xmlKeyBias().toStdString());
                if (drive && bias) {
                    return QString { "(drive=%1dB, bias=%2%)" }
                      .arg(drive->get().value() * 36.0f, 0, 'f', 1)
                      .arg(static_cast<int>(std::round(bias->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::waveDesigner()) {
                const auto attack = effect->parameter(Constants::NahdXml::xmlKeyAttack().toStdString());
                const auto sustain = effect->parameter(Constants::NahdXml::xmlKeySustain().toStdString());
                if (attack && sustain) {
                    return QString { "(attack=%1, sustain=%2)" }
                      .arg((attack->get().value() - 0.5f) * 2.0f, 0, 'f', 2)
                      .arg((sustain->get().value() - 0.5f) * 2.0f, 0, 'f', 2);
                }
            } else if (type == Constants::RackEffectType::stereoEnhancer()) {
                const auto bass = effect->parameter(Constants::NahdXml::xmlKeyBassGain().toStdString());
                const auto high = effect->parameter(Constants::NahdXml::xmlKeyHighGain().toStdString());
                const auto spread = effect->parameter(Constants::NahdXml::xmlKeySpread().toStdString());
                if (bass && high && spread) {
                    return QString { "(bass=%1%, hi=%2%, spread=%3%)" }
                      .arg(static_cast<int>(std::round(bass->get().value() * 100.0f)))
                      .arg(static_cast<int>(std::round(high->get().value() * 100.0f)))
                      .arg(static_cast<int>(std::round(spread->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::saturator()) {
                const auto drive = effect->parameter(Constants::NahdXml::xmlKeyDrive().toStdString());
                const auto mix = effect->parameter(Constants::NahdXml::xmlKeyMix().toStdString());
                if (drive && mix) {
                    return QString { "(drive=%1dB, mix=%2%)" }
                      .arg(drive->get().value() * 24.0f, 0, 'f', 1)
                      .arg(static_cast<int>(std::round(mix->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::drive()) {
                const auto drive = effect->parameter(Constants::NahdXml::xmlKeyDrive().toStdString());
                const auto mix = effect->parameter(Constants::NahdXml::xmlKeyMix().toStdString());
                if (drive && mix) {
                    return QString { "(drive=%1%, mix=%2%)" }
                      .arg(static_cast<int>(std::round(drive->get().value() * 100.0f)))
                      .arg(static_cast<int>(std::round(mix->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::endless()) {
                const auto size = effect->parameter(Constants::NahdXml::xmlKeySize().toStdString());
                const auto freeze = effect->parameter(Constants::NahdXml::xmlKeyFreeze().toStdString());
                if (size && freeze) {
                    return QString { "(size=%1%, freeze=%2)" }
                      .arg(static_cast<int>(std::round(size->get().value() * 100.0f)))
                      .arg(freeze->get().value() > 0.5f ? tr("on") : tr("off"));
                }
            }
        }
    }
    return {};
}

QString EffectRackController::reverbSizeKey() const
{
    return Constants::NahdXml::xmlKeySize();
}

QString EffectRackController::reverbDecayKey() const
{
    return Constants::NahdXml::xmlKeyDecay();
}

QString EffectRackController::reverbDampingKey() const
{
    return Constants::NahdXml::xmlKeyDamping();
}

QString EffectRackController::reverbPreDelayKey() const
{
    return Constants::NahdXml::xmlKeyPreDelay();
}

QString EffectRackController::reverbWidthKey() const
{
    return Constants::NahdXml::xmlKeyWidth();
}

QString EffectRackController::reverbLpfCutoffKey() const
{
    return Constants::NahdXml::xmlKeyLpfCutoff();
}

QString EffectRackController::reverbHpfCutoffKey() const
{
    return Constants::NahdXml::xmlKeyHpfCutoff();
}

QString EffectRackController::reverbMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

QString EffectRackController::reverbGatedKey() const
{
    return Constants::NahdXml::xmlKeyGated();
}

QString EffectRackController::reverbGateThresholdKey() const
{
    return Constants::NahdXml::xmlKeyThreshold();
}

QString EffectRackController::reverbGateAttackKey() const
{
    return Constants::NahdXml::xmlKeyAttack();
}

QString EffectRackController::reverbGateHoldKey() const
{
    return Constants::NahdXml::xmlKeyHold();
}

QString EffectRackController::reverbGateReleaseKey() const
{
    return Constants::NahdXml::xmlKeyRelease();
}

QString EffectRackController::chorusRateKey() const
{
    return Constants::NahdXml::xmlKeyRate();
}

QString EffectRackController::chorusDepthKey() const
{
    return Constants::NahdXml::xmlKeyDepth();
}

QString EffectRackController::chorusDelayKey() const
{
    return Constants::NahdXml::xmlKeyDelay();
}

QString EffectRackController::chorusMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

QString EffectRackController::chorusWidthKey() const
{
    return Constants::NahdXml::xmlKeyWidth();
}

QString EffectRackController::chorusLpfKey() const
{
    return Constants::NahdXml::xmlKeyLpfCutoff();
}

QString EffectRackController::chorusHpfKey() const
{
    return Constants::NahdXml::xmlKeyHpfCutoff();
}

QString EffectRackController::compressorThresholdKey() const
{
    return Constants::NahdXml::xmlKeyThreshold();
}

QString EffectRackController::compressorRatioKey() const
{
    return Constants::NahdXml::xmlKeyRatio();
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
    return Constants::NahdXml::xmlKeyKnee();
}

QString EffectRackController::compressorMakeupKey() const
{
    return Constants::NahdXml::xmlKeyMakeup();
}

QString EffectRackController::compressorLookaheadKey() const
{
    return Constants::NahdXml::xmlKeyLookahead();
}

QString EffectRackController::compressorSideChainSourceDeviceKey() const
{
    return Constants::NahdXml::xmlKeySideChainSourceDevice();
}

QString EffectRackController::compressorSideChainLpfKey() const
{
    return Constants::NahdXml::xmlKeySideChainLpf();
}

QString EffectRackController::autoDuckerThresholdKey() const
{
    return Constants::NahdXml::xmlKeyThreshold();
}

QString EffectRackController::autoDuckerAmountKey() const
{
    return Constants::NahdXml::xmlKeyAmount();
}

QString EffectRackController::autoDuckerKneeKey() const
{
    return Constants::NahdXml::xmlKeyKnee();
}

QString EffectRackController::autoDuckerAttackKey() const
{
    return Constants::NahdXml::xmlKeyAttack();
}

QString EffectRackController::autoDuckerReleaseKey() const
{
    return Constants::NahdXml::xmlKeyRelease();
}

QString EffectRackController::autoDuckerHoldKey() const
{
    return Constants::NahdXml::xmlKeyHold();
}

QString EffectRackController::autoDuckerSideChainSourceDeviceKey() const
{
    return Constants::NahdXml::xmlKeySideChainSourceDevice();
}

QString EffectRackController::autoDuckerSideChainLpfKey() const
{
    return Constants::NahdXml::xmlKeySideChainLpf();
}

QString EffectRackController::allPassFilterFrequencyKey() const
{
    return Constants::NahdXml::xmlKeyFrequency();
}

QString EffectRackController::allPassFilterQKey() const
{
    return Constants::NahdXml::xmlKeyQ();
}

QString EffectRackController::allPassFilterStagesKey() const
{
    return Constants::NahdXml::xmlKeyStages();
}

QString EffectRackController::clipperModeKey() const
{
    return Constants::NahdXml::xmlKeyMode();
}

QString EffectRackController::clipperThresholdKey() const
{
    return Constants::NahdXml::xmlKeyThreshold();
}

QString EffectRackController::clipperGainKey() const
{
    return Constants::NahdXml::xmlKeyGain();
}

QString EffectRackController::limiterThresholdKey() const
{
    return Constants::NahdXml::xmlKeyThreshold();
}

QString EffectRackController::limiterCeilingKey() const
{
    return Constants::NahdXml::xmlKeyCeiling();
}

QString EffectRackController::limiterReleaseKey() const
{
    return Constants::NahdXml::xmlKeyRelease();
}

QString EffectRackController::limiterLookaheadKey() const
{
    return Constants::NahdXml::xmlKeyLookahead();
}

QString EffectRackController::limiterBoostKey() const
{
    return Constants::NahdXml::xmlKeyBoost();
}

QString EffectRackController::saturatorModeKey() const
{
    return Constants::NahdXml::xmlKeyMode();
}

QString EffectRackController::saturatorDriveKey() const
{
    return Constants::NahdXml::xmlKeyDrive();
}

QString EffectRackController::saturatorToneKey() const
{
    return Constants::NahdXml::xmlKeyTone();
}

QString EffectRackController::saturatorMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

QString EffectRackController::saturatorGainKey() const
{
    return Constants::NahdXml::xmlKeyGain();
}

QString EffectRackController::tubeStageModeKey() const
{
    return Constants::NahdXml::xmlKeyMode();
}

QString EffectRackController::tubeStageDriveKey() const
{
    return Constants::NahdXml::xmlKeyDrive();
}

QString EffectRackController::tubeStageBiasKey() const
{
    return Constants::NahdXml::xmlKeyBias();
}

QString EffectRackController::tubeStageToneKey() const
{
    return Constants::NahdXml::xmlKeyTone();
}

QString EffectRackController::tubeStageMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

QString EffectRackController::tubeStageGainKey() const
{
    return Constants::NahdXml::xmlKeyGain();
}

QString EffectRackController::driveModeKey() const
{
    return Constants::NahdXml::xmlKeyMode();
}

QString EffectRackController::driveAmountKey() const
{
    return Constants::NahdXml::xmlKeyDrive();
}

QString EffectRackController::driveMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

QString EffectRackController::driveGainKey() const
{
    return Constants::NahdXml::xmlKeyGain();
}

QString EffectRackController::eq8BandParametricTypeKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandType(bandIndex);
}

QString EffectRackController::eq8BandParametricFreqKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandFreq(bandIndex);
}

QString EffectRackController::eq8BandParametricGainKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandGain(bandIndex);
}

QString EffectRackController::eq8BandParametricQKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandQ(bandIndex);
}

QString EffectRackController::eq8BandParametricStereoModeKey() const
{
    return Constants::NahdXml::xmlKeyStereoMode();
}

QString EffectRackController::vintagePassiveEqLowFreqKey() const
{
    return Constants::NahdXml::xmlKeyLowFreq();
}

QString EffectRackController::vintagePassiveEqLowBoostKey() const
{
    return Constants::NahdXml::xmlKeyLowBoost();
}

QString EffectRackController::vintagePassiveEqLowAttenKey() const
{
    return Constants::NahdXml::xmlKeyLowAtten();
}

QString EffectRackController::vintagePassiveEqHighBoostFreqKey() const
{
    return Constants::NahdXml::xmlKeyHighBoostFreq();
}

QString EffectRackController::vintagePassiveEqHighBoostKey() const
{
    return Constants::NahdXml::xmlKeyHighBoost();
}

QString EffectRackController::vintagePassiveEqBandwidthKey() const
{
    return Constants::NahdXml::xmlKeyBandwidth();
}

QString EffectRackController::vintagePassiveEqHighAttenFreqKey() const
{
    return Constants::NahdXml::xmlKeyHighAttenFreq();
}

QString EffectRackController::vintagePassiveEqHighAttenKey() const
{
    return Constants::NahdXml::xmlKeyHighAtten();
}

QString EffectRackController::simpleEqAmountKey() const
{
    return Constants::NahdXml::xmlKeyAmount();
}

QString EffectRackController::airBandEqBandGainKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandGain(bandIndex);
}

QString EffectRackController::airBandEqAirFreqKey() const
{
    return Constants::NahdXml::xmlKeyAirFreq();
}

QString EffectRackController::airBandEqAirGainKey() const
{
    return Constants::NahdXml::xmlKeyAirGain();
}

QString EffectRackController::airBandEqOutputGainKey() const
{
    return Constants::NahdXml::xmlKeyGain();
}

QString EffectRackController::allPassFilterType() const
{
    return Constants::RackEffectType::allPassFilter();
}

QString EffectRackController::lufsMeterType() const
{
    return Constants::RackEffectType::lufsMeter();
}

QString EffectRackController::dbtpMeterType() const
{
    return Constants::RackEffectType::dbtpMeter();
}

QString EffectRackController::rtaType() const
{
    return Constants::RackEffectType::rta();
}

QString EffectRackController::clipperType() const
{
    return Constants::RackEffectType::clipper();
}

QString EffectRackController::saturatorType() const
{
    return Constants::RackEffectType::saturator();
}

QString EffectRackController::waveDesignerType() const
{
    return Constants::RackEffectType::waveDesigner();
}

QString EffectRackController::waveDesignerAttackKey() const
{
    return Constants::NahdXml::xmlKeyAttack();
}

QString EffectRackController::waveDesignerSustainKey() const
{
    return Constants::NahdXml::xmlKeySustain();
}

QString EffectRackController::waveDesignerGainKey() const
{
    return Constants::NahdXml::xmlKeyGain();
}

QString EffectRackController::waveDesignerMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

QString EffectRackController::stereoEnhancerType() const
{
    return Constants::RackEffectType::stereoEnhancer();
}

QString EffectRackController::stereoEnhancerBassGainKey() const
{
    return Constants::NahdXml::xmlKeyBassGain();
}

QString EffectRackController::stereoEnhancerBassFreqKey() const
{
    return Constants::NahdXml::xmlKeyBassFreq();
}

QString EffectRackController::stereoEnhancerMidGainKey() const
{
    return Constants::NahdXml::xmlKeyMidGain();
}

QString EffectRackController::stereoEnhancerMidQKey() const
{
    return Constants::NahdXml::xmlKeyMidQ();
}

QString EffectRackController::stereoEnhancerHighGainKey() const
{
    return Constants::NahdXml::xmlKeyHighGain();
}

QString EffectRackController::stereoEnhancerHighFreqKey() const
{
    return Constants::NahdXml::xmlKeyHighFreq();
}

QString EffectRackController::stereoEnhancerGainKey() const
{
    return Constants::NahdXml::xmlKeyGain();
}

QString EffectRackController::stereoEnhancerSpreadKey() const
{
    return Constants::NahdXml::xmlKeySpread();
}

QString EffectRackController::stereoEnhancerMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

QString EffectRackController::tubeStageType() const
{
    return Constants::RackEffectType::tubeStage();
}

QString EffectRackController::driveType() const
{
    return Constants::RackEffectType::drive();
}

QString EffectRackController::limiterType() const
{
    return Constants::RackEffectType::limiter();
}

QString EffectRackController::chorusType() const
{
    return Constants::RackEffectType::chorus();
}

QString EffectRackController::compressorType() const
{
    return Constants::RackEffectType::compressor();
}

QString EffectRackController::autoDuckerType() const
{
    return Constants::RackEffectType::autoDucker();
}

QString EffectRackController::delayType() const
{
    return Constants::RackEffectType::delay();
}

QString EffectRackController::eq8BandParametricType() const
{
    return Constants::RackEffectType::eq8BandParametric();
}

QString EffectRackController::vintagePassiveEqType() const
{
    return Constants::RackEffectType::vintagePassiveEq();
}

QString EffectRackController::airBandEqType() const
{
    return Constants::RackEffectType::airBandEq();
}

QString EffectRackController::simpleEqType() const
{
    return Constants::RackEffectType::simpleEq();
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

QString EffectRackController::endlessType() const
{
    return Constants::RackEffectType::endless();
}

QString EffectRackController::endlessSizeKey() const
{
    return Constants::NahdXml::xmlKeySize();
}

QString EffectRackController::endlessFeedbackKey() const
{
    return Constants::NahdXml::xmlKeyDecay();
}

QString EffectRackController::endlessDampingKey() const
{
    return Constants::NahdXml::xmlKeyDamping();
}

QString EffectRackController::endlessPreDelayKey() const
{
    return Constants::NahdXml::xmlKeyPreDelay();
}

QString EffectRackController::endlessModDepthKey() const
{
    return Constants::NahdXml::xmlKeyDepth();
}

QString EffectRackController::endlessModRateKey() const
{
    return Constants::NahdXml::xmlKeyRate();
}

QString EffectRackController::endlessWidthKey() const
{
    return Constants::NahdXml::xmlKeyWidth();
}

QString EffectRackController::endlessLpfCutoffKey() const
{
    return Constants::NahdXml::xmlKeyLpfCutoff();
}

QString EffectRackController::endlessHpfCutoffKey() const
{
    return Constants::NahdXml::xmlKeyHpfCutoff();
}

QString EffectRackController::endlessMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

QString EffectRackController::endlessFreezeKey() const
{
    return Constants::NahdXml::xmlKeyFreeze();
}

float EffectRackController::compressorReductionDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto compressor = std::dynamic_pointer_cast<Compressor>(effect)) {
                return compressor->reductionDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::clipperReductionDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto clipper = std::dynamic_pointer_cast<Clipper>(effect); clipper) {
                return clipper->reductionDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::limiterReductionDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto limiter = std::dynamic_pointer_cast<Limiter>(effect); limiter) {
                return limiter->reductionDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::autoDuckerGainDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto autoDucker = std::dynamic_pointer_cast<AutoDucker>(effect); autoDucker) {
                return autoDucker->gainDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::saturatorSaturationDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto saturator = std::dynamic_pointer_cast<Saturator>(effect); saturator) {
                return saturator->saturationDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::waveDesignerShapingDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto waveDesigner = std::dynamic_pointer_cast<WaveDesigner>(effect); waveDesigner) {
                return waveDesigner->shapingDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::tubeStageSaturationDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto tubeStage = std::dynamic_pointer_cast<TubeStage>(effect); tubeStage) {
                return tubeStage->saturationDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::lufsMeterMomentary(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<LufsMeter>(effect)) {
                return meter->momentaryLufs();
            }
        }
    }
    return -70.0f;
}

float EffectRackController::lufsMeterShortTerm(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<LufsMeter>(effect)) {
                return meter->shortTermLufs();
            }
        }
    }
    return -70.0f;
}

float EffectRackController::dbtpMeterTruePeakL(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<DbTpMeter>(effect)) {
                return meter->truePeakL();
            }
        }
    }
    return dbtpFloor;
}

float EffectRackController::dbtpMeterTruePeakR(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<DbTpMeter>(effect)) {
                return meter->truePeakR();
            }
        }
    }
    return dbtpFloor;
}

float EffectRackController::dbtpMeterTruePeakHoldL(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<DbTpMeter>(effect)) {
                return meter->truePeakHoldL();
            }
        }
    }
    return dbtpFloor;
}

float EffectRackController::dbtpMeterTruePeakHoldR(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<DbTpMeter>(effect)) {
                return meter->truePeakHoldR();
            }
        }
    }
    return dbtpFloor;
}

QVariantList EffectRackController::rtaBandMagnitudes(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto rta = std::dynamic_pointer_cast<Rta>(effect)) {
                QVariantList list;
                for (const float v : rta->bandMagnitudesDb()) {
                    list.append(v);
                }
                return list;
            }
        }
    }
    return {};
}

QVariantList EffectRackController::rtaBandLogPositions(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto rta = std::dynamic_pointer_cast<Rta>(effect)) {
                QVariantList list;
                for (const auto & [xLo, xHi] : rta->bandLogPositions()) {
                    list.append(xLo);
                    list.append(xHi);
                }
                return list;
            }
        }
    }
    return {};
}

QString EffectRackController::rtaBandCountKey() const
{
    return Constants::NahdXml::xmlKeyBandCount();
}

QString EffectRackController::rtaDbRangeKey() const
{
    return Constants::NahdXml::xmlKeyDbRange();
}

QString EffectRackController::rtaShowPinkNoiseKey() const
{
    return Constants::NahdXml::xmlKeyShowPinkNoise();
}

QString EffectRackController::rtaPinkNoiseLevelKey() const
{
    return Constants::NahdXml::xmlKeyPinkNoiseLevel();
}

QString EffectRackController::rtaSpeedKey() const
{
    return Constants::NahdXml::xmlKeySpeed();
}

QString EffectRackController::rtaFftRateKey() const
{
    return Constants::NahdXml::xmlKeyFftRate();
}

void EffectRackController::rtaSetActive(quint32 effectIndex, bool active)
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto rta = std::dynamic_pointer_cast<Rta>(effect)) {
                rta->setAnalysisEnabled(active);
            }
        }
    }
}

QStringList EffectRackController::reverbPresets() const
{
    QStringList presets;
    for (const auto & name : Reverb::presetNames()) {
        presets.append(QString::fromStdString(name));
    }
    return presets;
}

void EffectRackController::applyReverbPreset(quint32 effectIndex, quint32 presetIndex)
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto reverb = std::dynamic_pointer_cast<Reverb>(effect); reverb) {
                const auto presetNames = Reverb::presetNames();
                if (presetIndex < presetNames.size()) {
                    reverb->applyPreset(Reverb::stringToPreset(presetNames[static_cast<size_t>(presetIndex)]));
                    m_editorService->setIsModified(true);
                    m_revision++;
                    emit revisionChanged();
                    emit parameterChanged(effectIndex, ""); // Notify all parameters changed
                }
            }
        }
    }
}

void EffectRackController::exportEffectSettings(int index, const QUrl & fileUrl)
{
    auto filePath = fileUrl.toLocalFile();
    if (!filePath.endsWith(Constants::effectRackSettingsExtension())) {
        filePath += Constants::effectRackSettingsExtension();
    }
    QFile file { filePath };
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    NahdXmlWriter writer { file };
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1);

    if (const auto rack = currentRack(); rack) {
        rack->get().exportEffectSettings(static_cast<size_t>(index), writer);
    }
}

void EffectRackController::importEffectSettings(int index, const QUrl & fileUrl)
{
    const auto fileInfo = peekEffectTypeInfo(fileUrl);
    const auto rack = currentRack();
    if (!rack) {
        return;
    }

    const auto currentEff = rack->get().effect(static_cast<size_t>(index));
    const auto currentType = currentEff ? QString::fromStdString(currentEff->type()) : QString {};
    const auto currentTypeId = currentEff ? QString::fromStdString(currentEff->typeId()) : QString {};
    const bool typeMismatch = currentEff && !fileInfo.typeId.isEmpty() && currentTypeId != fileInfo.typeId;

    emit importEffectSettingsConfirmationRequested(index, fileUrl, currentType, fileInfo.typeName, typeMismatch);
}

void EffectRackController::confirmImportEffectSettings(int index, const QUrl & fileUrl)
{
    QFile file { fileUrl.toLocalFile() };
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    NahdXmlReader reader { file };
    if (const auto rack = currentRack(); rack) {
        if (rack->get().importEffectSettings(static_cast<size_t>(index), reader)) {
            m_editorService->setIsModified(true);
            m_revision++;
            emit revisionChanged();
        }
    }
}

EffectRackController::EffectTypeInfo EffectRackController::peekEffectTypeInfo(const QUrl & fileUrl) const
{
    QFile file { fileUrl.toLocalFile() };
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    NahdXmlReader reader { file };
    while (!reader.atEnd() && !reader.hasError()) {
        if (reader.readNext() == ProjectReader::TokenType::StartElement) {
            if (reader.name() == Constants::NahdXml::xmlKeyEffect()) {
                return {
                    reader.attribute(Constants::NahdXml::xmlKeyTypeId()).toString(),
                    reader.attribute(Constants::NahdXml::xmlKeyType()).toString()
                };
            }
        }
    }
    return {};
}

void EffectRackController::exportSettings(const QUrl & fileUrl)
{
    auto filePath = fileUrl.toLocalFile();
    if (!filePath.endsWith(Constants::effectRackSettingsExtension())) {
        filePath += Constants::effectRackSettingsExtension();
    }
    QFile file { filePath };
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    NahdXmlWriter writer { file };
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1);
    exportSettings(writer);
}

bool EffectRackController::exportSettings(ProjectWriter & writer) const
{
    const auto rack = currentRack();
    if (!rack) {
        return false;
    }
    writer.writeStartDocument();
    writer.writeStartElement(Constants::NahdXml::xmlKeySettings());
    writer.writeAttribute(Constants::NahdXml::xmlKeyFileFormatVersion(), Constants::fileFormatVersion());
    writer.writeAttribute(Constants::NahdXml::xmlKeyApplicationName(), Constants::applicationName());
    writer.writeAttribute(Constants::NahdXml::xmlKeyApplicationVersion(), Constants::applicationVersion());
    writer.writeAttribute(Constants::NahdXml::xmlKeyCreatedDate(), QDateTime::currentDateTime().toString(Qt::DateFormat::ISODateWithMs));
    writer.writeStartElement(Constants::NahdXml::xmlKeyInsertEffects());
    rack->get().serializeEffectsToXml(writer);
    writer.writeEndElement(); // InsertEffects
    writer.writeEndElement(); // Settings
    writer.writeEndDocument();
    return true;
}

void EffectRackController::importSettings(const QUrl & fileUrl)
{
    QFile file { fileUrl.toLocalFile() };
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    NahdXmlReader reader { file };
    if (importSettings(reader)) {
        m_revision++;
        emit revisionChanged();
    }
}

bool EffectRackController::importSettings(ProjectReader & reader)
{
    const auto rack = currentRack();
    if (!rack) {
        return false;
    }
    while (!reader.atEnd() && !reader.hasError()) {
        if (reader.readNext() == ProjectReader::TokenType::StartElement) {
            if (reader.name() == Constants::NahdXml::xmlKeyInsertEffects()) {
                rack->get().deserializeEffectsFromXml(reader);
                return !reader.hasError();
            }
        }
    }
    return false;
}

float EffectRackController::deviceSend(const QString & deviceName, quint32 effectIndex) const
{
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        return device->reverbSend(effectIndex);
    }
    return 0.0f;
}

void EffectRackController::setDeviceSend(const QString & deviceName, quint32 effectIndex, float send)
{
    if (const auto device = m_deviceService->device(deviceName.toStdString()); device) {
        device->setReverbSend(effectIndex, send);
    }
}

} // namespace noteahead
