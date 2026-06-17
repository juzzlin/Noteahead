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
#include "../../domain/dsp/chorus_effect.hpp"
#include "../../domain/dsp/clipper_effect.hpp"
#include "../../domain/dsp/compressor_effect.hpp"
#include "../../domain/dsp/eq_8_band_parametric_effect.hpp"
#include "../../domain/dsp/reverb_effect.hpp"
#include "../../domain/effects/auto_panner_effect.hpp"
#include "../../domain/effects/delay_effect.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/panner_effect.hpp"
#include "../../infra/xml/nahd_xml_reader.hpp"
#include "../../infra/xml/nahd_xml_writer.hpp"
#include "knob_controller.hpp"

#include <QDateTime>
#include <QFile>
#include <QStringList>
#include <QVariant>
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
        if (const auto device = m_deviceService->device(m_targetDeviceName.toStdString()); device) {
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

    addEffect("Auto Panner", AutoPannerEffect::typeIdString());
    addEffect("Chorus", ChorusEffect::typeIdString());
    addEffect("Clipper", ClipperEffect::typeIdString());
    addEffect("Compressor", CompressorEffect::typeIdString());
    addEffect("Delay", DelayEffect::typeIdString());
    addEffect("EQ 8-Band Parametric", Eq8BandParametricEffect::typeIdString());
    addEffect("Panner", PannerEffect::typeIdString());
    addEffect("Reverb", ReverbEffect::typeIdString());

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

QString EffectRackController::effectParametersSummary(quint32 effectIndex) const
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            const auto type = QString::fromStdString(effect->type());
            if (type == Constants::RackEffectType::autoPanner()) {
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
            } else if (type == Constants::RackEffectType::compressor()) {
                const auto attack = effect->parameter(Constants::NahdXml::xmlKeyAttack().toStdString());
                const auto ratio = effect->parameter(Constants::NahdXml::xmlKeyRatio().toStdString());
                if (attack && ratio) {
                    const float attackMs = static_cast<float>(ParameterMapper::mapExponential(attack->get().value(), 0.1, 500.0));
                    const int ratioValue = ratio->get().xmlValue() / ratio->get().xmlScale();
                    return QString { "(attack=%1ms, ratio=%2:1)" }
                      .arg(attackMs, 0, 'f', 1)
                      .arg(ratioValue);
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

QString EffectRackController::clipperType() const
{
    return Constants::RackEffectType::clipper();
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

float EffectRackController::compressorReductionDb(quint32 effectIndex) const
{
    if (const auto rack = currentRack()) {
        if (const auto effect = rack->get().effect(effectIndex)) {
            if (const auto compressor = std::dynamic_pointer_cast<CompressorEffect>(effect)) {
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
            if (const auto clipper = std::dynamic_pointer_cast<ClipperEffect>(effect); clipper) {
                return clipper->reductionDb();
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

void EffectRackController::applyReverbPreset(quint32 effectIndex, quint32 presetIndex)
{
    if (const auto rack = currentRack(); rack) {
        if (const auto effect = rack->get().effect(effectIndex); effect) {
            if (const auto reverb = std::dynamic_pointer_cast<ReverbEffect>(effect); reverb) {
                const auto presetNames = ReverbEffect::presetNames();
                if (presetIndex < presetNames.size()) {
                    reverb->applyPreset(ReverbEffect::stringToPreset(presetNames[static_cast<size_t>(presetIndex)]));
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
