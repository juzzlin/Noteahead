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
#include "../../domain/dsp/lfo.hpp"
#include "../../domain/effects/all_pass_filter.hpp"
#include "../../domain/effects/analog_fuzz.hpp"
#include "../../domain/effects/auto_ducker.hpp"
#include "../../domain/effects/auto_filter.hpp"
#include "../../domain/effects/auto_panner.hpp"
#include "../../domain/effects/bass_grinder.hpp"
#include "../../domain/effects/chorus.hpp"
#include "../../domain/effects/clipper.hpp"
#include "../../domain/effects/compressor.hpp"
#include "../../domain/effects/delay.hpp"
#include "../../domain/effects/dimension.hpp"
#include "../../domain/effects/drive.hpp"
#include "../../domain/effects/early_reflections.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/eq_8_band_parametric.hpp"
#include "../../domain/effects/limiter.hpp"
#include "../../domain/effects/monitor.hpp"
#include "../../domain/effects/multiband_compressor.hpp"
#include "../../domain/effects/panner.hpp"
#include "../../domain/effects/phaser.hpp"
#include "../../domain/effects/reverb.hpp"
#include "../../domain/effects/saturator.hpp"
#include "../../domain/effects/stereo_enhancer.hpp"
#include "../../domain/effects/stereo_exciter.hpp"
#include "../../domain/effects/stereo_widener.hpp"
#include "../../domain/effects/tube_stage.hpp"
#include "../../domain/effects/wave_designer.hpp"
#include "../../domain/utility/dbtp_meter.hpp"
#include "../../domain/utility/lufs_meter.hpp"
#include "../../domain/utility/rta.hpp"
#include "../../domain/utility/stereo_field_meter.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"
#include "knob_controller.hpp"

#include <QDateTime>
#include <QFile>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

namespace noteahead {

static constexpr float dbtpFloor = -70.0f;

namespace {

//! One meter reading for the rack listing, padded so that a value crossing -10 does not shove
//! everything after it along the row. An ordinary space would not do it: the row is set in a
//! proportional font, where a space is much narrower than a digit, so a constant character count
//! still comes out a different width. U+2007 FIGURE SPACE is defined to be exactly as wide as a
//! digit, which lines the fields up without the row having to become monospaced.
QString formatMeterReading(float value)
{
    static constexpr int fieldWidth = 5; // "-70.0", the widest reading either meter can produce
    static const QChar figureSpace { 0x2007 };
    return value <= dbtpFloor ? QString { "-∞" }.rightJustified(fieldWidth, figureSpace) : QString { "%1" }.arg(value, fieldWidth, 'f', 1, figureSpace);
}

//! A cutoff for the rack listing, in the unit that keeps it to four characters.
QString formatFrequency(double hz)
{
    return hz >= 1000.0 ? QString { "%1kHz" }.arg(hz / 1000.0, 0, 'f', 1) : QString { "%1Hz" }.arg(hz, 0, 'f', 0);
}

} // namespace

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
    addEffect("Analog Fuzz", Constants::RackEffectType::analogFuzz().toStdString());
    addEffect("Auto Ducker", Constants::RackEffectType::autoDucker().toStdString());
    addEffect("Auto Filter", Constants::RackEffectType::autoFilter().toStdString());
    addEffect("Auto Panner", Constants::RackEffectType::autoPanner().toStdString());
    addEffect("Bass Grinder", Constants::RackEffectType::bassGrinder().toStdString());
    addEffect("Endless Reverb", Constants::RackEffectType::endless().toStdString());
    addEffect("Chorus", Chorus::typeIdString());
    addEffect("Clipper", Constants::RackEffectType::clipper().toStdString());
    addEffect("Compressor", Constants::RackEffectType::compressor().toStdString());
    addEffect("dBTP Meter", DbTpMeter::typeIdString());
    addEffect("Delay", Constants::RackEffectType::delay().toStdString());
    addEffect("Dimension", Constants::RackEffectType::dimension().toStdString());
    addEffect("Early Reflections", Constants::RackEffectType::earlyReflections().toStdString());
    addEffect("Drive", Constants::RackEffectType::drive().toStdString());
    addEffect("EQ 8-Band Parametric", Constants::RackEffectType::eq8BandParametric().toStdString());
    addEffect("Vintage Passive EQ", Constants::RackEffectType::vintagePassiveEq().toStdString());
    addEffect("Air Band EQ", Constants::RackEffectType::airBandEq().toStdString());
    addEffect("Simple EQ", Constants::RackEffectType::simpleEq().toStdString());
    addEffect("Limiter", Constants::RackEffectType::limiter().toStdString());
    addEffect("Monitor", Constants::RackEffectType::monitor().toStdString());
    addEffect("Multiband Compressor", Constants::RackEffectType::multibandCompressor().toStdString());
    addEffect("LUFS Meter", LufsMeter::typeIdString());
    addEffect("Panner", Constants::RackEffectType::panner().toStdString());
    addEffect("Phaser", Constants::RackEffectType::phaser().toStdString());
    addEffect("Reverb", Constants::RackEffectType::reverb().toStdString());
    addEffect("RTA", Constants::RackEffectType::rta().toStdString());
    addEffect("Saturator", Constants::RackEffectType::saturator().toStdString());
    addEffect("Tube Stage", Constants::RackEffectType::tubeStage().toStdString());
    addEffect("Wave Designer", Constants::RackEffectType::waveDesigner().toStdString());
    addEffect("Stereo Enhancer", Constants::RackEffectType::stereoEnhancer().toStdString());
    addEffect("Stereo Exciter", Constants::RackEffectType::stereoExciter().toStdString());
    addEffect("Stereo Widener", Constants::RackEffectType::stereoWidener().toStdString());
    addEffect("Stereo Field Meter", Constants::RackEffectType::stereoFieldMeter().toStdString());

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
            } else if (type == Constants::RackEffectType::autoFilter()) {
                const auto filterType = effect->parameter(Constants::NahdXml::xmlKeyFilterType().toStdString());
                const auto cutoff = effect->parameter(Constants::NahdXml::xmlKeyCutoff().toStdString());
                const auto mode = effect->parameter(Constants::NahdXml::xmlKeyLfoMode().toStdString());
                const auto rate = effect->parameter(Constants::NahdXml::xmlKeyLfoRate().toStdString());
                const auto intensity = effect->parameter(Constants::NahdXml::xmlKeyLfoIntensity().toStdString());
                if (filterType && cutoff && mode && rate && intensity) {
                    const QStringList typeNames { "LPF", "HPF", "BPF", "Notch" };
                    QString rateStr;
                    if (static_cast<Lfo::Mode>(mode->get().xmlValue()) == Lfo::Mode::BPM) {
                        KnobController knobController;
                        rateStr = knobController.syncLabel(knobController.syncIndex(rate->get().value() * Constants::uiInternalScaling()));
                    } else {
                        rateStr = QString { "%1Hz" }.arg(ParameterMapper::mapLfoFrequency(rate->get().value(), 0.05, 20.0), 0, 'f', 2);
                    }
                    // The intensity readout carries the same cubic taper as the knob it came from.
                    const auto intensityPercent = ParameterMapper::mapCubicCentered((intensity->get().value() - 0.5f) * 2.0f, -100.0, 100.0);
                    return QString { "(%1, %2, rate=%3, int=%4%)" }
                      .arg(typeNames.value(std::clamp(filterType->get().xmlValue(), 0, 3)))
                      .arg(formatFrequency(ParameterMapper::mapExponential(cutoff->get().value(), 20.0, 20000.0)))
                      .arg(rateStr)
                      .arg(static_cast<int>(std::round(intensityPercent)));
                }
            } else if (type == Constants::RackEffectType::phaser()) {
                const auto stages = effect->parameter(Constants::NahdXml::xmlKeyStages().toStdString());
                const auto mode = effect->parameter(Constants::NahdXml::xmlKeyLfoMode().toStdString());
                const auto rate = effect->parameter(Constants::NahdXml::xmlKeyLfoRate().toStdString());
                const auto feedback = effect->parameter(Constants::NahdXml::xmlKeyFeedback().toStdString());
                const auto divider = effect->parameter(Constants::NahdXml::xmlKeyRateDivider().toStdString());
                if (stages && mode && rate && feedback && divider) {
                    const auto rateDivider = std::max(1, divider->get().xmlValue());
                    QString rateStr;
                    if (static_cast<Lfo::Mode>(mode->get().xmlValue()) == Lfo::Mode::BPM) {
                        KnobController knobController;
                        // A division cannot be divided into a division that has a name, so the
                        // divider is reported as itself.
                        rateStr = knobController.syncLabel(knobController.syncIndex(rate->get().value() * Constants::uiInternalScaling()));
                        if (rateDivider > 1) {
                            rateStr += QString { "/%1" }.arg(rateDivider);
                        }
                    } else {
                        rateStr = QString { "%1Hz" }.arg(ParameterMapper::mapLfoFrequency(rate->get().value(), 0.05, 20.0) / rateDivider, 0, 'f', 2);
                    }
                    const auto feedbackPercent = ParameterMapper::mapCubicCentered((feedback->get().value() - 0.5f) * 2.0f, -100.0, 100.0);
                    return QString { "(%1 stages, rate=%2, fb=%3%)" }
                      .arg(stages->get().xmlValue())
                      .arg(rateStr)
                      .arg(static_cast<int>(std::round(feedbackPercent)));
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
            } else if (type == Constants::RackEffectType::monitor()) {
                // Named in full and in capitals: a monitor left folded is the one setting in the rack
                // that quietly misrepresents everything downstream of it, so it has to be readable
                // without opening anything.
                if (const auto mode = effect->parameter(Constants::NahdXml::xmlKeyMode().toStdString()); mode) {
                    switch (static_cast<Monitor::Mode>(mode->get().xmlValue())) {
                    case Monitor::Mode::Stereo:
                        return QString { "(%1)" }.arg(tr("STEREO"));
                    case Monitor::Mode::Mono:
                        return QString { "(%1)" }.arg(tr("MONO"));
                    case Monitor::Mode::Left:
                        return QString { "(%1)" }.arg(tr("LEFT"));
                    case Monitor::Mode::Right:
                        return QString { "(%1)" }.arg(tr("RIGHT"));
                    case Monitor::Mode::Side:
                        return QString { "(%1)" }.arg(tr("SIDE"));
                    }
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
                    QString modeName { tr("Peak") };
                    if (const auto mode { effect->parameter(Constants::NahdXml::xmlKeyMode().toStdString()) }; mode) {
                        if (mode->get().value() > 0.5f) {
                            modeName = tr("RMS");
                        }
                    }
                    return QString { "(%1, attack=%2ms, ratio=%3:1, sidechain=%4)" }
                      .arg(modeName)
                      .arg(attackMs, 0, 'f', 1)
                      .arg(ratioValue, 0, 'g', 3)
                      .arg(scName);
                }
            } else if (type == Constants::RackEffectType::multibandCompressor()) {
                const auto lowerCrossover { effect->parameter(Constants::NahdXml::xmlKeyCrossoverFreq(0).toStdString()) };
                const auto upperCrossover { effect->parameter(Constants::NahdXml::xmlKeyCrossoverFreq(1).toStdString()) };
                if (lowerCrossover && upperCrossover) {
                    QString scName { tr("None") };
                    if (const auto sourceIndex { effect->sidechainSourceDeviceIndex() }; sourceIndex) {
                        if (const auto sourceDevice { m_deviceService->device(*sourceIndex) }) {
                            scName = QString::fromStdString(sourceDevice->name());
                        }
                    }
                    QString modeName { tr("Peak") };
                    if (const auto mode { effect->parameter(Constants::NahdXml::xmlKeyMode().toStdString()) }; mode) {
                        if (mode->get().value() > 0.5f) {
                            modeName = tr("RMS");
                        }
                    }
                    return QString { "(%1, %2/%3Hz, sidechain=%4)" }
                      .arg(modeName)
                      .arg(static_cast<int>(std::round(ParameterMapper::mapLogFrequency(lowerCrossover->get().value(), 20.0, 20000.0))))
                      .arg(static_cast<int>(std::round(ParameterMapper::mapLogFrequency(upperCrossover->get().value(), 20.0, 20000.0))))
                      .arg(scName);
                }
            } else if (type == Constants::RackEffectType::earlyReflections()) {
                const auto size { effect->parameter(Constants::NahdXml::xmlKeySize().toStdString()) };
                const auto preDelay { effect->parameter(Constants::NahdXml::xmlKeyPreDelay().toStdString()) };
                const auto mix { effect->parameter(Constants::NahdXml::xmlKeyMix().toStdString()) };
                if (size && preDelay && mix) {
                    return QString { "(size=%1%, pre=%2ms, mix=%3%)" }
                      .arg(static_cast<int>(std::round(size->get().value() * 100.0f)))
                      .arg(static_cast<int>(std::round(preDelay->get().value() * 100.0f)))
                      .arg(static_cast<int>(std::round(mix->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::dimension()) {
                const auto detune { effect->parameter(Constants::NahdXml::xmlKeyDetune().toStdString()) };
                const auto amount { effect->parameter(Constants::NahdXml::xmlKeyAmount().toStdString()) };
                if (detune && amount) {
                    return QString { "(detune=%1c, amount=%2%)" }
                      .arg(detune->get().value() * 25.0f, 0, 'f', 1)
                      .arg(static_cast<int>(std::round(amount->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::stereoFieldMeter()) {
                if (const auto meter { std::dynamic_pointer_cast<StereoFieldMeter>(effect) }; meter) {
                    const auto reading = meter->reading();
                    return QString { "(corr=%1, side=%2dB)" }
                      .arg(reading.correlation, 0, 'f', 2)
                      .arg(reading.sideDb, 0, 'f', 1);
                }
            } else if (type == Constants::RackEffectType::stereoWidener()) {
                const auto lowWidth { effect->parameter(Constants::NahdXml::xmlKeyBandWidth(0).toStdString()) };
                const auto midWidth { effect->parameter(Constants::NahdXml::xmlKeyBandWidth(1).toStdString()) };
                const auto highWidth { effect->parameter(Constants::NahdXml::xmlKeyBandWidth(2).toStdString()) };
                if (lowWidth && midWidth && highWidth) {
                    QString monoName { tr("off") };
                    if (const auto monoBass { effect->parameter(Constants::NahdXml::xmlKeyMonoBass().toStdString()) }; monoBass && monoBass->get().value() > 0.5f) {
                        if (const auto monoFreq { effect->parameter(Constants::NahdXml::xmlKeyMonoFreq().toStdString()) }; monoFreq) {
                            monoName = QString { "<%1Hz" }.arg(static_cast<int>(std::round(ParameterMapper::mapLogFrequency(monoFreq->get().value(), 20.0, 300.0))));
                        }
                    }
                    const auto widthPercent = [](const auto & parameter) {
                        return static_cast<int>(std::round(parameter->get().value() * 200.0f));
                    };
                    return QString { "(lo=%1%, mid=%2%, hi=%3%, mono=%4)" }
                      .arg(widthPercent(lowWidth))
                      .arg(widthPercent(midWidth))
                      .arg(widthPercent(highWidth))
                      .arg(monoName);
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
                    return QString { "(M=%1 S=%2 I=%3 LUFS)" }
                      .arg(formatMeterReading(meter->momentaryLufs()))
                      .arg(formatMeterReading(meter->shortTermLufs()))
                      .arg(formatMeterReading(meter->integratedLufs()));
                }
            } else if (type == Constants::RackEffectType::dbtpMeter()) {
                if (const auto meter = std::dynamic_pointer_cast<DbTpMeter>(effect)) {
                    return QString { "(L=%1 R=%2 dBTP)" }
                      .arg(formatMeterReading(meter->truePeakHoldL()))
                      .arg(formatMeterReading(meter->truePeakHoldR()));
                }
            } else if (type == Constants::RackEffectType::eq8BandParametric()) {
                QString modeName { tr("Mid + Side") };
                if (const auto stereoMode { effect->parameter(Constants::NahdXml::xmlKeyStereoMode().toStdString()) }; stereoMode) {
                    switch (static_cast<Eq8BandParametric::StereoMode>(std::clamp(static_cast<int>(std::round(stereoMode->get().value())), 0, 2))) {
                    case Eq8BandParametric::StereoMode::Mid:
                        modeName = tr("Mid");
                        break;
                    case Eq8BandParametric::StereoMode::Side:
                        modeName = tr("Side");
                        break;
                    case Eq8BandParametric::StereoMode::MidSide:
                        break;
                    }
                }
                return QString { "(Parametric, %1)" }.arg(modeName);
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
            } else if (type == Constants::RackEffectType::bassGrinder()) {
                const auto drive = effect->parameter(Constants::NahdXml::xmlKeyDrive().toStdString());
                const auto blend = effect->parameter(Constants::NahdXml::xmlKeyBlend().toStdString());
                if (drive && blend) {
                    return QString { "(drive=%1dB, blend=%2%)" }
                      .arg(drive->get().value() * 40.0f, 0, 'f', 1)
                      .arg(static_cast<int>(std::round(blend->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::analogFuzz()) {
                const auto drive = effect->parameter(Constants::NahdXml::xmlKeyDrive().toStdString());
                const auto fuzz = effect->parameter(Constants::NahdXml::xmlKeyFuzz().toStdString());
                if (drive && fuzz) {
                    return QString { "(drive=%1dB, fuzz=%2%)" }
                      .arg(drive->get().value() * 42.0f, 0, 'f', 1)
                      .arg(static_cast<int>(std::round(fuzz->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::tubeStage()) {
                const auto drive = effect->parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString());
                const auto bias = effect->parameter(Constants::NahdXml::xmlKeyBias().toStdString());
                if (drive && bias) {
                    return QString { "(drive=%1dB, bias=%2%)" }
                      .arg(drive->get().value() * 48.0f, 0, 'f', 1)
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
            } else if (type == Constants::RackEffectType::stereoExciter()) {
                const auto tune = effect->parameter(Constants::NahdXml::xmlKeyTune().toStdString());
                const auto harmonics = effect->parameter(Constants::NahdXml::xmlKeyHarmonics().toStdString());
                if (tune && harmonics) {
                    return QString { "(tune=%1Hz, harmonics=%2%)" }
                      .arg(static_cast<int>(std::round(ParameterMapper::mapLogFrequency(tune->get().value(), 700.0, 8000.0))))
                      .arg(static_cast<int>(std::round(harmonics->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::saturator()) {
                const auto drive = effect->parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString());
                const auto mix = effect->parameter(Constants::NahdXml::xmlKeyMix().toStdString());
                if (drive && mix) {
                    return QString { "(drive=%1dB, mix=%2%)" }
                      .arg(drive->get().value() * 40.0f, 0, 'f', 1)
                      .arg(static_cast<int>(std::round(mix->get().value() * 100.0f)));
                }
            } else if (type == Constants::RackEffectType::drive()) {
                const auto drive = effect->parameter(Constants::NahdXml::xmlKeyDriveDb().toStdString());
                const auto mix = effect->parameter(Constants::NahdXml::xmlKeyMix().toStdString());
                if (drive && mix) {
                    return QString { "(drive=%1dB, mix=%2%)" }
                      .arg(drive->get().value() * 40.0f, 0, 'f', 1)
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

QString EffectRackController::compressorModeKey() const
{
    return Constants::NahdXml::xmlKeyMode();
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

QString EffectRackController::monitorModeKey() const
{
    return Constants::NahdXml::xmlKeyMode();
}

QString EffectRackController::saturatorModeKey() const
{
    return Constants::NahdXml::xmlKeyMode();
}

QString EffectRackController::saturatorDriveKey() const
{
    return Constants::NahdXml::xmlKeyDriveDb();
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

QString EffectRackController::analogFuzzDriveKey() const
{
    return Constants::NahdXml::xmlKeyDrive();
}

QString EffectRackController::analogFuzzFuzzKey() const
{
    return Constants::NahdXml::xmlKeyFuzz();
}

QString EffectRackController::analogFuzzBiasKey() const
{
    return Constants::NahdXml::xmlKeyBias();
}

QString EffectRackController::analogFuzzCutoffKey() const
{
    return Constants::NahdXml::xmlKeyCutoff();
}

QString EffectRackController::analogFuzzResonanceKey() const
{
    return Constants::NahdXml::xmlKeyResonance();
}

QString EffectRackController::analogFuzzMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

QString EffectRackController::analogFuzzGainKey() const
{
    return Constants::NahdXml::xmlKeyGain();
}

QString EffectRackController::tubeStageModeKey() const
{
    return Constants::NahdXml::xmlKeyMode();
}

QString EffectRackController::tubeStageDriveKey() const
{
    return Constants::NahdXml::xmlKeyDriveDb();
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

QString EffectRackController::bassGrinderDriveKey() const
{
    return Constants::NahdXml::xmlKeyDrive();
}

QString EffectRackController::bassGrinderBlendKey() const
{
    return Constants::NahdXml::xmlKeyBlend();
}

QString EffectRackController::bassGrinderSplitFreqKey() const
{
    return Constants::NahdXml::xmlKeySplitFreq();
}

QString EffectRackController::bassGrinderColorKey() const
{
    return Constants::NahdXml::xmlKeyColor();
}

QString EffectRackController::bassGrinderBassGainKey() const
{
    return Constants::NahdXml::xmlKeyBassGain();
}

QString EffectRackController::bassGrinderMidGainKey() const
{
    return Constants::NahdXml::xmlKeyMidGain();
}

QString EffectRackController::bassGrinderMidFreqKey() const
{
    return Constants::NahdXml::xmlKeyMidFreq();
}

QString EffectRackController::bassGrinderHighGainKey() const
{
    return Constants::NahdXml::xmlKeyHighGain();
}

QString EffectRackController::bassGrinderMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

QString EffectRackController::bassGrinderGainKey() const
{
    return Constants::NahdXml::xmlKeyGain();
}

QString EffectRackController::driveModeKey() const
{
    return Constants::NahdXml::xmlKeyMode();
}

QString EffectRackController::driveAmountKey() const
{
    return Constants::NahdXml::xmlKeyDriveDb();
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

QString EffectRackController::autoFilterFilterTypeKey() const
{
    return Constants::NahdXml::xmlKeyFilterType();
}

QString EffectRackController::autoFilterFilterSlopeKey() const
{
    return Constants::NahdXml::xmlKeyFilterSlope();
}

QString EffectRackController::autoFilterCutoffKey() const
{
    return Constants::NahdXml::xmlKeyCutoff();
}

QString EffectRackController::autoFilterResonanceKey() const
{
    return Constants::NahdXml::xmlKeyResonance();
}

QString EffectRackController::autoFilterLfoWaveformKey() const
{
    return Constants::NahdXml::xmlKeyLfoWaveform();
}

QString EffectRackController::autoFilterLfoModeKey() const
{
    return Constants::NahdXml::xmlKeyLfoMode();
}

QString EffectRackController::autoFilterLfoRateKey() const
{
    return Constants::NahdXml::xmlKeyLfoRate();
}

QString EffectRackController::autoFilterLfoIntensityKey() const
{
    return Constants::NahdXml::xmlKeyLfoIntensity();
}

QString EffectRackController::autoFilterLfo2WaveformKey() const
{
    return Constants::NahdXml::xmlKeyLfo2Waveform();
}

QString EffectRackController::autoFilterLfo2ModeKey() const
{
    return Constants::NahdXml::xmlKeyLfo2Mode();
}

QString EffectRackController::autoFilterLfo2RateKey() const
{
    return Constants::NahdXml::xmlKeyLfo2Rate();
}

QString EffectRackController::autoFilterLfo2IntensityKey() const
{
    return Constants::NahdXml::xmlKeyLfo2Intensity();
}

QString EffectRackController::autoFilterStereoPhaseKey() const
{
    return Constants::NahdXml::xmlKeyStereoPhase();
}

QString EffectRackController::autoFilterEnvModKey() const
{
    return Constants::NahdXml::xmlKeyEnvMod();
}

QString EffectRackController::autoFilterEnvAttackKey() const
{
    return Constants::NahdXml::xmlKeyAttack();
}

QString EffectRackController::autoFilterEnvReleaseKey() const
{
    return Constants::NahdXml::xmlKeyRelease();
}

QString EffectRackController::autoFilterGainKey() const
{
    return Constants::NahdXml::xmlKeyGain();
}

QString EffectRackController::autoFilterMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

QString EffectRackController::phaserStagesKey() const
{
    return Constants::NahdXml::xmlKeyStages();
}

QString EffectRackController::phaserFrequencyKey() const
{
    return Constants::NahdXml::xmlKeyFrequency();
}

QString EffectRackController::phaserDepthKey() const
{
    return Constants::NahdXml::xmlKeyDepth();
}

QString EffectRackController::phaserFeedbackKey() const
{
    return Constants::NahdXml::xmlKeyFeedback();
}

QString EffectRackController::phaserLfoWaveformKey() const
{
    return Constants::NahdXml::xmlKeyLfoWaveform();
}

QString EffectRackController::phaserLfoModeKey() const
{
    return Constants::NahdXml::xmlKeyLfoMode();
}

QString EffectRackController::phaserLfoRateKey() const
{
    return Constants::NahdXml::xmlKeyLfoRate();
}

QString EffectRackController::phaserRateDividerKey() const
{
    return Constants::NahdXml::xmlKeyRateDivider();
}

QString EffectRackController::phaserStereoPhaseKey() const
{
    return Constants::NahdXml::xmlKeyStereoPhase();
}

QString EffectRackController::phaserGainKey() const
{
    return Constants::NahdXml::xmlKeyGain();
}

QString EffectRackController::phaserMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

int EffectRackController::phaserMaxStages() const
{
    return Phaser::maxStages();
}

int EffectRackController::phaserMaxRateDivider() const
{
    return Phaser::maxRateDivider();
}

QStringList EffectRackController::lfoWaveformNames() const
{
    QStringList names;
    for (auto && name : Lfo::waveformNames()) {
        names.append(QString::fromStdString(name));
    }
    return names;
}

QStringList EffectRackController::lfoModeNames() const
{
    // Ordered by Lfo::Mode, which the parameter stores as its ordinal.
    return { tr("Normal"), tr("Sync"), tr("One-Shot") };
}

QString EffectRackController::multibandCompressorCrossoverFreqKey(quint32 crossoverIndex) const
{
    return Constants::NahdXml::xmlKeyCrossoverFreq(crossoverIndex);
}

QString EffectRackController::multibandCompressorThresholdKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandThreshold(bandIndex);
}

QString EffectRackController::multibandCompressorRatioKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandRatio(bandIndex);
}

QString EffectRackController::multibandCompressorKneeKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandKnee(bandIndex);
}

QString EffectRackController::multibandCompressorAttackKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandAttack(bandIndex);
}

QString EffectRackController::multibandCompressorReleaseKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandRelease(bandIndex);
}

QString EffectRackController::multibandCompressorMakeupKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandMakeup(bandIndex);
}

QString EffectRackController::multibandCompressorBypassKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandBypass(bandIndex);
}

QString EffectRackController::multibandCompressorSoloKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandSolo(bandIndex);
}

QString EffectRackController::multibandCompressorModeKey() const
{
    return Constants::NahdXml::xmlKeyMode();
}

QString EffectRackController::multibandCompressorGainKey() const
{
    return Constants::NahdXml::xmlKeyGain();
}

QString EffectRackController::multibandCompressorSideChainSourceDeviceKey() const
{
    return Constants::NahdXml::xmlKeySideChainSourceDevice();
}

QString EffectRackController::stereoWidenerCrossoverFreqKey(quint32 crossoverIndex) const
{
    return Constants::NahdXml::xmlKeyCrossoverFreq(crossoverIndex);
}

QString EffectRackController::stereoWidenerWidthKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandWidth(bandIndex);
}

QString EffectRackController::stereoWidenerSoloKey(quint32 bandIndex) const
{
    return Constants::NahdXml::xmlKeyBandSolo(bandIndex);
}

QString EffectRackController::stereoWidenerMonoBassKey() const
{
    return Constants::NahdXml::xmlKeyMonoBass();
}

QString EffectRackController::stereoWidenerMonoFreqKey() const
{
    return Constants::NahdXml::xmlKeyMonoFreq();
}

QString EffectRackController::stereoWidenerGainKey() const
{
    return Constants::NahdXml::xmlKeyGain();
}

QString EffectRackController::earlyReflectionsSizeKey() const
{
    return Constants::NahdXml::xmlKeySize();
}

QString EffectRackController::earlyReflectionsPreDelayKey() const
{
    return Constants::NahdXml::xmlKeyPreDelay();
}

QString EffectRackController::earlyReflectionsDampingKey() const
{
    return Constants::NahdXml::xmlKeyDamping();
}

QString EffectRackController::earlyReflectionsDiffusionKey() const
{
    return Constants::NahdXml::xmlKeyDiffusion();
}

QString EffectRackController::earlyReflectionsWidthKey() const
{
    return Constants::NahdXml::xmlKeyWidth();
}

QString EffectRackController::earlyReflectionsLowCutKey() const
{
    return Constants::NahdXml::xmlKeyHpfCutoff();
}

QString EffectRackController::earlyReflectionsMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

QString EffectRackController::earlyReflectionsSoloKey() const
{
    return Constants::NahdXml::xmlKeySolo();
}

QString EffectRackController::dimensionDetuneKey() const
{
    return Constants::NahdXml::xmlKeyDetune();
}

QString EffectRackController::dimensionAmountKey() const
{
    return Constants::NahdXml::xmlKeyAmount();
}

QString EffectRackController::dimensionLowCutKey() const
{
    return Constants::NahdXml::xmlKeyHpfCutoff();
}

QString EffectRackController::dimensionSoloKey() const
{
    return Constants::NahdXml::xmlKeySolo();
}

QString EffectRackController::stereoFieldMeterSpeedKey() const
{
    return Constants::NahdXml::xmlKeySpeed();
}

QString EffectRackController::stereoFieldMeterZoomKey() const
{
    return Constants::NahdXml::xmlKeyZoom();
}

QString EffectRackController::stereoFieldMeterShowGuidesKey() const
{
    return Constants::NahdXml::xmlKeyShowGuides();
}

QVariantList EffectRackController::stereoFieldMeterPoints(quint32 effectIndex, int maxPoints) const
{
    QVariantList list;
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto meter = std::dynamic_pointer_cast<StereoFieldMeter>(effect); meter) {
                const auto snapshot = meter->trace(static_cast<size_t>(std::max(maxPoints, 0)));
                const auto count = std::min(snapshot.left.size(), snapshot.right.size());
                list.reserve(static_cast<qsizetype>(count * 2));
                for (size_t i = 0; i < count; i++) {
                    list.append(snapshot.left[i]);
                    list.append(snapshot.right[i]);
                }
            }
        }
    }
    return list;
}

QVariantMap EffectRackController::stereoFieldMeterReading(quint32 effectIndex) const
{
    QVariantMap map;
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto meter = std::dynamic_pointer_cast<StereoFieldMeter>(effect); meter) {
                const auto reading = meter->reading();
                QVariantList bands;
                for (const float value : reading.bandCorrelation) {
                    bands.append(value);
                }
                map["correlation"] = reading.correlation;
                map["bandCorrelations"] = bands;
                map["midDb"] = reading.midDb;
                map["sideDb"] = reading.sideDb;
                map["balance"] = reading.balance;
            }
        }
    }
    return map;
}

void EffectRackController::stereoFieldMeterSetActive(quint32 effectIndex, bool active)
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto meter = std::dynamic_pointer_cast<StereoFieldMeter>(effect); meter) {
                meter->setAnalysisEnabled(active);
            }
        }
    }
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

QString EffectRackController::stereoEnhancerSoloKey() const
{
    return Constants::NahdXml::xmlKeySolo();
}

QString EffectRackController::stereoExciterType() const
{
    return Constants::RackEffectType::stereoExciter();
}

QString EffectRackController::stereoExciterTuneKey() const
{
    return Constants::NahdXml::xmlKeyTune();
}

QString EffectRackController::stereoExciterPeakKey() const
{
    return Constants::NahdXml::xmlKeyPeak();
}

QString EffectRackController::stereoExciterZeroFillKey() const
{
    return Constants::NahdXml::xmlKeyZeroFill();
}

QString EffectRackController::stereoExciterTimbreKey() const
{
    return Constants::NahdXml::xmlKeyTimbre();
}

QString EffectRackController::stereoExciterHarmonicsKey() const
{
    return Constants::NahdXml::xmlKeyHarmonics();
}

QString EffectRackController::stereoExciterMixKey() const
{
    return Constants::NahdXml::xmlKeyMix();
}

QString EffectRackController::stereoExciterSoloKey() const
{
    return Constants::NahdXml::xmlKeySolo();
}

QString EffectRackController::analogFuzzType() const
{
    return Constants::RackEffectType::analogFuzz();
}

QString EffectRackController::tubeStageType() const
{
    return Constants::RackEffectType::tubeStage();
}

QString EffectRackController::driveType() const
{
    return Constants::RackEffectType::drive();
}

QString EffectRackController::bassGrinderType() const
{
    return Constants::RackEffectType::bassGrinder();
}

QString EffectRackController::limiterType() const
{
    return Constants::RackEffectType::limiter();
}

QString EffectRackController::monitorType() const
{
    return Constants::RackEffectType::monitor();
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

QString EffectRackController::autoFilterType() const
{
    return Constants::RackEffectType::autoFilter();
}

QString EffectRackController::phaserType() const
{
    return Constants::RackEffectType::phaser();
}

QString EffectRackController::multibandCompressorType() const
{
    return Constants::RackEffectType::multibandCompressor();
}

QString EffectRackController::stereoWidenerType() const
{
    return Constants::RackEffectType::stereoWidener();
}

QString EffectRackController::stereoFieldMeterType() const
{
    return Constants::RackEffectType::stereoFieldMeter();
}

QString EffectRackController::dimensionType() const
{
    return Constants::RackEffectType::dimension();
}

QString EffectRackController::earlyReflectionsType() const
{
    return Constants::RackEffectType::earlyReflections();
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

float EffectRackController::multibandCompressorBandReductionDb(quint32 effectIndex, quint32 bandIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto compressor = std::dynamic_pointer_cast<MultibandCompressor>(effect); compressor) {
                return compressor->bandReductionDb(bandIndex);
            }
        }
    }

    return 0.0f;
}

float EffectRackController::stereoWidenerBandCorrelation(quint32 effectIndex, quint32 bandIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto width = std::dynamic_pointer_cast<StereoWidener>(effect); width) {
                return width->bandCorrelation(bandIndex);
            }
        }
    }

    // Nothing to read is reported as centred, which is where the meter rests.
    return 1.0f;
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

float EffectRackController::stereoExciterHarmonicsDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto exciter = std::dynamic_pointer_cast<StereoExciter>(effect); exciter) {
                return exciter->harmonicsDb();
            }
        }
    }

    return 0.0f;
}

float EffectRackController::analogFuzzSaturationDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto analogFuzz = std::dynamic_pointer_cast<AnalogFuzz>(effect); analogFuzz) {
                return analogFuzz->saturationDb();
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

float EffectRackController::bassGrinderSaturationDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto bassGrinder = std::dynamic_pointer_cast<BassGrinder>(effect); bassGrinder) {
                return bassGrinder->saturationDb();
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

float EffectRackController::lufsMeterIntegrated(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<LufsMeter>(effect)) {
                return meter->integratedLufs();
            }
        }
    }
    return -70.0f;
}

void EffectRackController::resetLufsMeter(quint32 effectIndex)
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto meter = std::dynamic_pointer_cast<LufsMeter>(effect)) {
                meter->requestReset();
            }
        }
    }
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

void EffectRackController::snapshotEffect(int effectIndex)
{
    m_snapshot.reset();
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            m_snapshot = EffectSnapshot { effectIndex, effect->parameterSnapshot(), effect->enabled() };
        }
    }
}

void EffectRackController::revertEffect(int effectIndex)
{
    if (!m_snapshot || m_snapshot->effectIndex != effectIndex) {
        return;
    }
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(static_cast<size_t>(effectIndex)); effect) {
            effect->restoreParameterSnapshot(m_snapshot->parameters);
            effect->setEnabled(m_snapshot->enabled);
            effect->sync();
            m_editorService->setIsModified(true);
            m_revision++;
            emit revisionChanged();
            emit parameterChanged(static_cast<quint32>(effectIndex), ""); // Notify all parameters changed
        }
    }
    m_snapshot.reset();
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
