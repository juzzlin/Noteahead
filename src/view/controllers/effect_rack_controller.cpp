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
#include "../../domain/effects/auto_panner.hpp"
#include "../../domain/effects/chorus.hpp"
#include "../../domain/effects/clipper.hpp"
#include "../../domain/effects/compressor.hpp"
#include "../../domain/effects/delay.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/eq_8_band_parametric.hpp"
#include "../../domain/effects/limiter.hpp"
#include "../../domain/effects/panner.hpp"
#include "../../domain/effects/reverb.hpp"
#include "../../domain/effects/saturator.hpp"
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

int EffectRackController::targetSubIndex() const
{
    return m_targetSubIndex;
}

void EffectRackController::setTargetSubIndex(int index)
{
    m_targetSubIndex = index;
    emit targetSubIndexChanged();
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
    addEffect("Auto Panner", Constants::RackEffectType::autoPanner().toStdString());
    addEffect("Endless Reverb", Constants::RackEffectType::endless().toStdString());
    addEffect("Chorus", Chorus::typeIdString());
    addEffect("Clipper", Constants::RackEffectType::clipper().toStdString());
    addEffect("Compressor", Constants::RackEffectType::compressor().toStdString());
    addEffect("dBTP Meter", DbTpMeter::typeIdString());
    addEffect("Delay", Constants::RackEffectType::delay().toStdString());
    addEffect("EQ 8-Band Parametric", Constants::RackEffectType::eq8BandParametric().toStdString());
    addEffect("Limiter", Constants::RackEffectType::limiter().toStdString());
    addEffect("LUFS Meter", LufsMeter::typeIdString());
    addEffect("Panner", Constants::RackEffectType::panner().toStdString());
    addEffect("Reverb", Constants::RackEffectType::reverb().toStdString());
    addEffect("RTA", Constants::RackEffectType::rta().toStdString());
    addEffect("Saturator", Constants::RackEffectType::saturator().toStdString());

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
            } else if (type == Constants::RackEffectType::saturator()) {
                const auto drive = effect->parameter(Constants::NahdXml::xmlKeyDrive().toStdString());
                const auto mix = effect->parameter(Constants::NahdXml::xmlKeyMix().toStdString());
                if (drive && mix) {
                    return QString { "(drive=%1dB, mix=%2%)" }
                      .arg(drive->get().value() * 24.0f, 0, 'f', 1)
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
