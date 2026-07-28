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

#ifndef EFFECT_RACK_CONTROLLER_HPP
#define EFFECT_RACK_CONTROLLER_HPP

#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"

#include <QObject>
#include <QUrl>

#include <functional>
#include <memory>
#include <optional>

namespace noteahead {

class ProjectReader;
class ProjectWriter;

class EffectRackController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int effectCount READ effectCount NOTIFY effectCountChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(QString targetDeviceName READ targetDeviceName WRITE setTargetDeviceName NOTIFY targetDeviceNameChanged)
    Q_PROPERTY(bool isInsertRack READ isInsertRack WRITE setIsInsertRack NOTIFY isInsertRackChanged)
    Q_PROPERTY(int targetSubIndex READ targetSubIndex WRITE setTargetSubIndex NOTIFY targetSubIndexChanged)
    Q_PROPERTY(bool rackEnabled READ rackEnabled WRITE setRackEnabled NOTIFY rackEnabledChanged)

    Q_PROPERTY(QString allPassFilterType READ allPassFilterType CONSTANT)
    Q_PROPERTY(QString lufsMeterType READ lufsMeterType CONSTANT)
    Q_PROPERTY(QString dbtpMeterType READ dbtpMeterType CONSTANT)
    Q_PROPERTY(QString rtaType READ rtaType CONSTANT)
    Q_PROPERTY(QString chorusType READ chorusType CONSTANT)
    Q_PROPERTY(QString clipperType READ clipperType CONSTANT)
    Q_PROPERTY(QString saturatorType READ saturatorType CONSTANT)
    Q_PROPERTY(QString driveType READ driveType CONSTANT)
    Q_PROPERTY(QString limiterType READ limiterType CONSTANT)
    Q_PROPERTY(QString compressorType READ compressorType CONSTANT)
    Q_PROPERTY(QString delayType READ delayType CONSTANT)
    Q_PROPERTY(QString eq8BandParametricType READ eq8BandParametricType CONSTANT)
    Q_PROPERTY(QString vintagePassiveEqType READ vintagePassiveEqType CONSTANT)
    Q_PROPERTY(QString airBandEqType READ airBandEqType CONSTANT)
    Q_PROPERTY(QString simpleEqType READ simpleEqType CONSTANT)
    Q_PROPERTY(QString pannerType READ pannerType CONSTANT)
    Q_PROPERTY(QString autoPannerType READ autoPannerType CONSTANT)
    Q_PROPERTY(QString reverbType READ reverbType CONSTANT)
    Q_PROPERTY(QString endlessType READ endlessType CONSTANT)

public:
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using EditorServiceS = std::shared_ptr<EditorService>;
    explicit EffectRackController(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent = nullptr);

    int effectCount() const;
    int revision() const;

    QString targetDeviceName() const;
    void setTargetDeviceName(const QString & name);

    bool isInsertRack() const;
    void setIsInsertRack(bool isInsert);

    // -1 targets the whole-device rack; >= 0 targets a Sampler pad (note) or Drum Synth voice (index).
    int targetSubIndex() const;
    void setTargetSubIndex(int index);

    bool rackEnabled() const;
    void setRackEnabled(bool enabled);

    QString allPassFilterType() const;
    QString autoPannerType() const;
    QString dbtpMeterType() const;
    QString lufsMeterType() const;
    QString rtaType() const;
    QString clipperType() const;
    QString saturatorType() const;
    QString driveType() const;
    QString limiterType() const;
    QString compressorType() const;
    QString delayType() const;
    QString chorusType() const;
    QString eq8BandParametricType() const;
    QString vintagePassiveEqType() const;
    QString airBandEqType() const;
    QString simpleEqType() const;
    QString pannerType() const;
    QString reverbType() const;
    QString endlessType() const;

    Q_INVOKABLE QString effectParametersSummary(quint32 effectIndex) const;
    Q_INVOKABLE QString effectDisplayName(const QString & typeId) const;
    Q_INVOKABLE float parameterValue(quint32 effectIndex, const QString & paramName) const;
    Q_INVOKABLE void setParameterValue(quint32 effectIndex, const QString & paramName, float value);

    Q_INVOKABLE void setEffect(int slotIndex, const QString & typeId);
    Q_INVOKABLE void clearEffect(int slotIndex);
    Q_INVOKABLE void moveEffectUp(int index);
    Q_INVOKABLE void moveEffectDown(int index);

    Q_INVOKABLE void exportEffectSettings(int index, const QUrl & fileUrl);
    Q_INVOKABLE void importEffectSettings(int index, const QUrl & fileUrl);
    Q_INVOKABLE void confirmImportEffectSettings(int index, const QUrl & fileUrl);

    Q_INVOKABLE void copyEffect(int sourceSlot, int targetSlot);
    Q_INVOKABLE QVariantList populatedEffects() const;

    Q_INVOKABLE void exportSettings(const QUrl & fileUrl);
    bool exportSettings(ProjectWriter & writer) const;

    Q_INVOKABLE void importSettings(const QUrl & fileUrl);
    bool importSettings(ProjectReader & reader);
    Q_INVOKABLE QVariantList availableEffects() const;

    Q_INVOKABLE bool isEffectEnabled(quint32 effectIndex) const;
    Q_INVOKABLE void setIsEffectEnabled(quint32 effectIndex, bool enabled);

    Q_INVOKABLE QStringList parameterNames(quint32 effectIndex) const;
    Q_INVOKABLE QString effectType(quint32 effectIndex) const;

    Q_INVOKABLE QString reverbSizeKey() const;
    Q_INVOKABLE QString reverbDecayKey() const;
    Q_INVOKABLE QString reverbDampingKey() const;
    Q_INVOKABLE QString reverbPreDelayKey() const;
    Q_INVOKABLE QString reverbWidthKey() const;
    Q_INVOKABLE QString reverbLpfCutoffKey() const;
    Q_INVOKABLE QString reverbHpfCutoffKey() const;
    Q_INVOKABLE QString reverbMixKey() const;
    Q_INVOKABLE QString reverbGatedKey() const;
    Q_INVOKABLE QString reverbGateThresholdKey() const;
    Q_INVOKABLE QString reverbGateAttackKey() const;
    Q_INVOKABLE QString reverbGateHoldKey() const;
    Q_INVOKABLE QString reverbGateReleaseKey() const;

    Q_INVOKABLE QString chorusRateKey() const;
    Q_INVOKABLE QString chorusDepthKey() const;
    Q_INVOKABLE QString chorusDelayKey() const;
    Q_INVOKABLE QString chorusMixKey() const;
    Q_INVOKABLE QString chorusWidthKey() const;
    Q_INVOKABLE QString chorusLpfKey() const;
    Q_INVOKABLE QString chorusHpfKey() const;
    Q_INVOKABLE QString compressorThresholdKey() const;
    Q_INVOKABLE QString compressorRatioKey() const;
    Q_INVOKABLE QString compressorAttackKey() const;
    Q_INVOKABLE QString compressorReleaseKey() const;
    Q_INVOKABLE QString compressorKneeKey() const;
    Q_INVOKABLE QString compressorMakeupKey() const;
    Q_INVOKABLE QString compressorLookaheadKey() const;
    Q_INVOKABLE QString compressorSideChainSourceDeviceKey() const;
    Q_INVOKABLE QString compressorSideChainLpfKey() const;

    Q_INVOKABLE QString allPassFilterFrequencyKey() const;
    Q_INVOKABLE QString allPassFilterQKey() const;
    Q_INVOKABLE QString allPassFilterStagesKey() const;

    Q_INVOKABLE QString clipperModeKey() const;
    Q_INVOKABLE QString clipperThresholdKey() const;
    Q_INVOKABLE QString clipperGainKey() const;

    Q_INVOKABLE QString limiterThresholdKey() const;
    Q_INVOKABLE QString limiterCeilingKey() const;
    Q_INVOKABLE QString limiterReleaseKey() const;
    Q_INVOKABLE QString limiterLookaheadKey() const;
    Q_INVOKABLE QString limiterBoostKey() const;

    Q_INVOKABLE QString endlessSizeKey() const;
    Q_INVOKABLE QString endlessFeedbackKey() const;
    Q_INVOKABLE QString endlessDampingKey() const;
    Q_INVOKABLE QString endlessPreDelayKey() const;
    Q_INVOKABLE QString endlessModDepthKey() const;
    Q_INVOKABLE QString endlessModRateKey() const;
    Q_INVOKABLE QString endlessWidthKey() const;
    Q_INVOKABLE QString endlessLpfCutoffKey() const;
    Q_INVOKABLE QString endlessHpfCutoffKey() const;
    Q_INVOKABLE QString endlessMixKey() const;
    Q_INVOKABLE QString endlessFreezeKey() const;

    Q_INVOKABLE QString saturatorModeKey() const;
    Q_INVOKABLE QString saturatorDriveKey() const;
    Q_INVOKABLE QString saturatorToneKey() const;
    Q_INVOKABLE QString saturatorMixKey() const;
    Q_INVOKABLE QString saturatorGainKey() const;

    Q_INVOKABLE QString driveModeKey() const;
    Q_INVOKABLE QString driveAmountKey() const;
    Q_INVOKABLE QString driveMixKey() const;
    Q_INVOKABLE QString driveGainKey() const;

    Q_INVOKABLE QString eq8BandParametricTypeKey(quint32 bandIndex) const;
    Q_INVOKABLE QString eq8BandParametricFreqKey(quint32 bandIndex) const;
    Q_INVOKABLE QString eq8BandParametricGainKey(quint32 bandIndex) const;
    Q_INVOKABLE QString eq8BandParametricQKey(quint32 bandIndex) const;
    Q_INVOKABLE QString eq8BandParametricStereoModeKey() const;

    Q_INVOKABLE QString vintagePassiveEqLowFreqKey() const;
    Q_INVOKABLE QString vintagePassiveEqLowBoostKey() const;
    Q_INVOKABLE QString vintagePassiveEqLowAttenKey() const;
    Q_INVOKABLE QString vintagePassiveEqHighBoostFreqKey() const;
    Q_INVOKABLE QString vintagePassiveEqHighBoostKey() const;
    Q_INVOKABLE QString vintagePassiveEqBandwidthKey() const;
    Q_INVOKABLE QString vintagePassiveEqHighAttenFreqKey() const;
    Q_INVOKABLE QString vintagePassiveEqHighAttenKey() const;

    Q_INVOKABLE QString airBandEqBandGainKey(quint32 bandIndex) const;
    Q_INVOKABLE QString airBandEqAirFreqKey() const;
    Q_INVOKABLE QString airBandEqAirGainKey() const;
    Q_INVOKABLE QString airBandEqOutputGainKey() const;

    Q_INVOKABLE QString simpleEqAmountKey() const;

    Q_INVOKABLE float compressorReductionDb(quint32 effectIndex) const;
    Q_INVOKABLE float clipperReductionDb(quint32 effectIndex) const;
    Q_INVOKABLE float limiterReductionDb(quint32 effectIndex) const;
    Q_INVOKABLE float saturatorSaturationDb(quint32 effectIndex) const;
    Q_INVOKABLE float lufsMeterMomentary(quint32 effectIndex) const;
    Q_INVOKABLE float lufsMeterShortTerm(quint32 effectIndex) const;
    Q_INVOKABLE float dbtpMeterTruePeakL(quint32 effectIndex) const;
    Q_INVOKABLE float dbtpMeterTruePeakR(quint32 effectIndex) const;
    Q_INVOKABLE float dbtpMeterTruePeakHoldL(quint32 effectIndex) const;
    Q_INVOKABLE float dbtpMeterTruePeakHoldR(quint32 effectIndex) const;

    Q_INVOKABLE QVariantList rtaBandMagnitudes(quint32 effectIndex) const;
    Q_INVOKABLE QVariantList rtaBandLogPositions(quint32 effectIndex) const;
    Q_INVOKABLE QString rtaBandCountKey() const;
    Q_INVOKABLE QString rtaDbRangeKey() const;
    Q_INVOKABLE QString rtaShowPinkNoiseKey() const;
    Q_INVOKABLE QString rtaPinkNoiseLevelKey() const;
    Q_INVOKABLE QString rtaSpeedKey() const;
    Q_INVOKABLE QString rtaFftRateKey() const;
    Q_INVOKABLE void rtaSetActive(quint32 effectIndex, bool active);

    Q_INVOKABLE QStringList reverbPresets() const;
    Q_INVOKABLE void applyReverbPreset(quint32 effectIndex, quint32 presetIndex);

    Q_INVOKABLE float deviceSend(const QString & deviceName, quint32 effectIndex) const;
    Q_INVOKABLE void setDeviceSend(const QString & deviceName, quint32 effectIndex, float send);

signals:
    void effectCountChanged();
    void revisionChanged();
    void targetDeviceNameChanged();
    void isInsertRackChanged();
    void targetSubIndexChanged();
    void rackEnabledChanged();
    void parameterChanged(quint32 effectIndex, const QString & paramName);
    void importEffectSettingsConfirmationRequested(int slotIndex, QUrl fileUrl, QString currentType, QString importedType, bool typeMismatch);

private:
    struct EffectTypeInfo
    {
        QString typeId;
        QString typeName;
    };

    EffectTypeInfo peekEffectTypeInfo(const QUrl & fileUrl) const;
    std::optional<std::reference_wrapper<EffectRack>> currentRack() const;

    DeviceServiceS m_deviceService;
    EditorServiceS m_editorService;
    QString m_targetDeviceName;
    int m_revision = 0;
    bool m_isInsertRack = false;
    int m_targetSubIndex = -1;
};

} // namespace noteahead

#endif // EFFECT_RACK_CONTROLLER_HPP
