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
#include "../../domain/tracker/parameter_container.hpp"

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
    Q_PROPERTY(QString tubeStageType READ tubeStageType CONSTANT)
    Q_PROPERTY(QString waveDesignerType READ waveDesignerType CONSTANT)
    Q_PROPERTY(QString stereoEnhancerType READ stereoEnhancerType CONSTANT)
    Q_PROPERTY(QString stereoExciterType READ stereoExciterType CONSTANT)
    Q_PROPERTY(QString driveType READ driveType CONSTANT)
    Q_PROPERTY(QString analogFuzzType READ analogFuzzType CONSTANT)
    Q_PROPERTY(QString bassGrinderType READ bassGrinderType CONSTANT)
    Q_PROPERTY(QString limiterType READ limiterType CONSTANT)
    Q_PROPERTY(QString monitorType READ monitorType CONSTANT)
    Q_PROPERTY(QString gainType READ gainType CONSTANT)
    Q_PROPERTY(QString compressorType READ compressorType CONSTANT)
    Q_PROPERTY(QString multibandCompressorType READ multibandCompressorType CONSTANT)
    Q_PROPERTY(QString stereoWidenerType READ stereoWidenerType CONSTANT)
    Q_PROPERTY(QString stereoFieldMeterType READ stereoFieldMeterType CONSTANT)
    Q_PROPERTY(QString delayType READ delayType CONSTANT)
    Q_PROPERTY(QString dimensionType READ dimensionType CONSTANT)
    Q_PROPERTY(QString earlyReflectionsType READ earlyReflectionsType CONSTANT)
    Q_PROPERTY(QString eq8BandParametricType READ eq8BandParametricType CONSTANT)
    Q_PROPERTY(QString vintagePassiveEqType READ vintagePassiveEqType CONSTANT)
    Q_PROPERTY(QString airBandEqType READ airBandEqType CONSTANT)
    Q_PROPERTY(QString simpleEqType READ simpleEqType CONSTANT)
    Q_PROPERTY(QString pannerType READ pannerType CONSTANT)
    Q_PROPERTY(QString autoPannerType READ autoPannerType CONSTANT)
    Q_PROPERTY(QString autoDuckerType READ autoDuckerType CONSTANT)
    Q_PROPERTY(QString autoFilterType READ autoFilterType CONSTANT)
    Q_PROPERTY(QString phaserType READ phaserType CONSTANT)
    Q_PROPERTY(QString reverbType READ reverbType CONSTANT)
    Q_PROPERTY(QString endlessType READ endlessType CONSTANT)

public:
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using EditorServiceS = std::shared_ptr<EditorService>;
    explicit EffectRackController(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent = nullptr);

    int effectCount() const;
    int revision() const;

    //! Bumps the revision so the dialogs re-read their summaries after a language change. The
    //! parameter summaries and the master rack labels are built with tr() at call time, so nothing
    //! else is needed to pick the new language up.
    void retranslate();

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
    QString autoDuckerType() const;
    QString autoFilterType() const;
    QString phaserType() const;
    QString autoPannerType() const;
    QString dbtpMeterType() const;
    QString lufsMeterType() const;
    QString rtaType() const;
    QString clipperType() const;
    QString saturatorType() const;
    QString tubeStageType() const;
    QString waveDesignerType() const;
    QString stereoEnhancerType() const;
    QString stereoExciterType() const;
    QString driveType() const;
    QString analogFuzzType() const;
    QString bassGrinderType() const;
    QString limiterType() const;
    QString monitorType() const;
    QString gainType() const;
    QString compressorType() const;
    QString multibandCompressorType() const;
    QString stereoWidenerType() const;
    QString stereoFieldMeterType() const;
    QString delayType() const;
    QString dimensionType() const;
    QString earlyReflectionsType() const;
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

    //! Remembers an effect's settings as its dialog opens, so that the dialog's Cancel button can
    //! put them back. Only one effect dialog is ever open, so one snapshot is enough.
    Q_INVOKABLE void snapshotEffect(int effectIndex);
    Q_INVOKABLE void revertEffect(int effectIndex);

    Q_INVOKABLE void setEffect(int slotIndex, const QString & typeId);
    Q_INVOKABLE void clearEffect(int slotIndex);
    Q_INVOKABLE void moveEffectUp(int index);
    Q_INVOKABLE void moveEffectDown(int index);
    Q_INVOKABLE void moveEffectToTop(int index);
    Q_INVOKABLE void moveEffectToBottom(int index);

    Q_INVOKABLE void exportEffectSettings(int index, const QUrl & fileUrl);
    Q_INVOKABLE void importEffectSettings(int index, const QUrl & fileUrl);
    Q_INVOKABLE void confirmImportEffectSettings(int index, const QUrl & fileUrl);

    Q_INVOKABLE void copyEffect(int sourceSlot, int targetSlot);
    Q_INVOKABLE QVariantList populatedEffects() const;

    //! Every rack that could be copied into the one currently targeted, the target itself left out.
    //!
    //! An entry carries "deviceName" and "isInsertRack", which is how copyRackFrom() addresses a
    //! rack: an empty device name is the master, where the flag picks the insert or the send rack.
    //! Sampler pad and Drum Synth voice racks are not offered -- a device has dozens of them, and
    //! the whole-device rack is what one wants to reuse.
    Q_INVOKABLE QVariantList availableRackSources() const;
    //! Replace the targeted rack with clones of another rack's effects. Returns false when either
    //! rack is missing or the two are the same rack.
    Q_INVOKABLE bool copyRackFrom(const QString & sourceDeviceName, bool sourceIsInsertRack);

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
    Q_INVOKABLE QString compressorModeKey() const;
    Q_INVOKABLE QString compressorThresholdKey() const;
    Q_INVOKABLE QString compressorRatioKey() const;
    Q_INVOKABLE QString compressorAttackKey() const;
    Q_INVOKABLE QString compressorReleaseKey() const;
    Q_INVOKABLE QString compressorKneeKey() const;
    Q_INVOKABLE QString compressorMakeupKey() const;
    Q_INVOKABLE QString compressorLookaheadKey() const;
    Q_INVOKABLE QString compressorSideChainSourceDeviceKey() const;
    Q_INVOKABLE QString compressorSideChainLpfKey() const;

    Q_INVOKABLE QString autoFilterFilterTypeKey() const;
    Q_INVOKABLE QString autoFilterFilterSlopeKey() const;
    Q_INVOKABLE QString autoFilterCutoffKey() const;
    Q_INVOKABLE QString autoFilterResonanceKey() const;
    Q_INVOKABLE QString autoFilterLfoWaveformKey() const;
    Q_INVOKABLE QString autoFilterLfoModeKey() const;
    Q_INVOKABLE QString autoFilterLfoRateKey() const;
    Q_INVOKABLE QString autoFilterLfoIntensityKey() const;
    Q_INVOKABLE QString autoFilterLfo2WaveformKey() const;
    Q_INVOKABLE QString autoFilterLfo2ModeKey() const;
    Q_INVOKABLE QString autoFilterLfo2RateKey() const;
    Q_INVOKABLE QString autoFilterLfo2IntensityKey() const;
    Q_INVOKABLE QString autoFilterStereoPhaseKey() const;
    Q_INVOKABLE QString autoFilterEnvModKey() const;
    Q_INVOKABLE QString autoFilterEnvAttackKey() const;
    Q_INVOKABLE QString autoFilterEnvReleaseKey() const;
    Q_INVOKABLE QString autoFilterGainKey() const;
    Q_INVOKABLE QString autoFilterMixKey() const;
    Q_INVOKABLE QString phaserStagesKey() const;
    Q_INVOKABLE QString phaserFrequencyKey() const;
    Q_INVOKABLE QString phaserDepthKey() const;
    Q_INVOKABLE QString phaserFeedbackKey() const;
    Q_INVOKABLE QString phaserLfoWaveformKey() const;
    Q_INVOKABLE QString phaserLfoModeKey() const;
    Q_INVOKABLE QString phaserLfoRateKey() const;
    Q_INVOKABLE QString phaserRateDividerKey() const;
    Q_INVOKABLE QString phaserStereoPhaseKey() const;
    Q_INVOKABLE QString phaserGainKey() const;
    Q_INVOKABLE QString phaserMixKey() const;
    Q_INVOKABLE int phaserMaxStages() const;
    Q_INVOKABLE int phaserMaxRateDivider() const;

    //! Shared by every effect whose LFO follows the Synth's shapes and modes.
    Q_INVOKABLE QStringList lfoWaveformNames() const;
    Q_INVOKABLE QStringList lfoModeNames() const;

    Q_INVOKABLE QString multibandCompressorCrossoverFreqKey(quint32 crossoverIndex) const;
    Q_INVOKABLE QString multibandCompressorThresholdKey(quint32 bandIndex) const;
    Q_INVOKABLE QString multibandCompressorRatioKey(quint32 bandIndex) const;
    Q_INVOKABLE QString multibandCompressorKneeKey(quint32 bandIndex) const;
    Q_INVOKABLE QString multibandCompressorAttackKey(quint32 bandIndex) const;
    Q_INVOKABLE QString multibandCompressorReleaseKey(quint32 bandIndex) const;
    Q_INVOKABLE QString multibandCompressorMakeupKey(quint32 bandIndex) const;
    Q_INVOKABLE QString multibandCompressorBypassKey(quint32 bandIndex) const;
    Q_INVOKABLE QString multibandCompressorSoloKey(quint32 bandIndex) const;
    Q_INVOKABLE QString multibandCompressorModeKey() const;
    Q_INVOKABLE QString multibandCompressorGainKey() const;
    Q_INVOKABLE QString multibandCompressorSideChainSourceDeviceKey() const;

    Q_INVOKABLE QString stereoWidenerCrossoverFreqKey(quint32 crossoverIndex) const;
    Q_INVOKABLE QString stereoWidenerWidthKey(quint32 bandIndex) const;
    Q_INVOKABLE QString stereoWidenerSoloKey(quint32 bandIndex) const;
    Q_INVOKABLE QString stereoWidenerMonoBassKey() const;
    Q_INVOKABLE QString stereoWidenerMonoFreqKey() const;
    Q_INVOKABLE QString stereoWidenerGainKey() const;

    Q_INVOKABLE QString earlyReflectionsSizeKey() const;
    Q_INVOKABLE QString earlyReflectionsPreDelayKey() const;
    Q_INVOKABLE QString earlyReflectionsDampingKey() const;
    Q_INVOKABLE QString earlyReflectionsDiffusionKey() const;
    Q_INVOKABLE QString earlyReflectionsWidthKey() const;
    Q_INVOKABLE QString earlyReflectionsLowCutKey() const;
    Q_INVOKABLE QString earlyReflectionsMixKey() const;
    Q_INVOKABLE QString earlyReflectionsSoloKey() const;

    Q_INVOKABLE QString dimensionDetuneKey() const;
    Q_INVOKABLE QString dimensionAmountKey() const;
    Q_INVOKABLE QString dimensionLowCutKey() const;
    Q_INVOKABLE QString dimensionSoloKey() const;

    Q_INVOKABLE QString stereoFieldMeterSpeedKey() const;
    Q_INVOKABLE QString stereoFieldMeterZoomKey() const;
    Q_INVOKABLE QString stereoFieldMeterShowGuidesKey() const;
    //! Recent sample pairs for the goniometer, flat: [l0, r0, l1, r1, ...].
    Q_INVOKABLE QVariantList stereoFieldMeterPoints(quint32 effectIndex, int maxPoints) const;
    //! Correlation, per-band correlation, mid and side levels and balance, taken together.
    Q_INVOKABLE QVariantMap stereoFieldMeterReading(quint32 effectIndex) const;
    Q_INVOKABLE void stereoFieldMeterSetActive(quint32 effectIndex, bool active);

    Q_INVOKABLE QString autoDuckerThresholdKey() const;
    Q_INVOKABLE QString autoDuckerAmountKey() const;
    Q_INVOKABLE QString autoDuckerKneeKey() const;
    Q_INVOKABLE QString autoDuckerAttackKey() const;
    Q_INVOKABLE QString autoDuckerReleaseKey() const;
    Q_INVOKABLE QString autoDuckerHoldKey() const;
    Q_INVOKABLE QString autoDuckerSideChainSourceDeviceKey() const;
    Q_INVOKABLE QString autoDuckerSideChainLpfKey() const;

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

    Q_INVOKABLE QString monitorModeKey() const;

    Q_INVOKABLE QString gainGainKey() const;
    //! Whether the Gain in this slot has seen a full-scale sample since it was last cleared.
    Q_INVOKABLE bool gainClipped(quint32 effectIndex) const;
    Q_INVOKABLE void clearGainClip(quint32 effectIndex);

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

    Q_INVOKABLE QString tubeStageModeKey() const;
    Q_INVOKABLE QString tubeStageDriveKey() const;
    Q_INVOKABLE QString tubeStageBiasKey() const;
    Q_INVOKABLE QString tubeStageToneKey() const;
    Q_INVOKABLE QString tubeStageMixKey() const;
    Q_INVOKABLE QString tubeStageGainKey() const;

    Q_INVOKABLE QString analogFuzzDriveKey() const;
    Q_INVOKABLE QString analogFuzzFuzzKey() const;
    Q_INVOKABLE QString analogFuzzBiasKey() const;
    Q_INVOKABLE QString analogFuzzCutoffKey() const;
    Q_INVOKABLE QString analogFuzzResonanceKey() const;
    Q_INVOKABLE QString analogFuzzMixKey() const;
    Q_INVOKABLE QString analogFuzzGainKey() const;

    Q_INVOKABLE QString bassGrinderDriveKey() const;
    Q_INVOKABLE QString bassGrinderBlendKey() const;
    Q_INVOKABLE QString bassGrinderSplitFreqKey() const;
    Q_INVOKABLE QString bassGrinderColorKey() const;
    Q_INVOKABLE QString bassGrinderBassGainKey() const;
    Q_INVOKABLE QString bassGrinderMidGainKey() const;
    Q_INVOKABLE QString bassGrinderMidFreqKey() const;
    Q_INVOKABLE QString bassGrinderHighGainKey() const;
    Q_INVOKABLE QString bassGrinderMixKey() const;
    Q_INVOKABLE QString bassGrinderGainKey() const;

    Q_INVOKABLE QString waveDesignerAttackKey() const;
    Q_INVOKABLE QString waveDesignerSustainKey() const;
    Q_INVOKABLE QString waveDesignerGainKey() const;
    Q_INVOKABLE QString waveDesignerMixKey() const;

    Q_INVOKABLE QString stereoEnhancerBassGainKey() const;
    Q_INVOKABLE QString stereoEnhancerBassFreqKey() const;
    Q_INVOKABLE QString stereoEnhancerMidGainKey() const;
    Q_INVOKABLE QString stereoEnhancerMidQKey() const;
    Q_INVOKABLE QString stereoEnhancerHighGainKey() const;
    Q_INVOKABLE QString stereoEnhancerHighFreqKey() const;
    Q_INVOKABLE QString stereoEnhancerGainKey() const;
    Q_INVOKABLE QString stereoEnhancerSpreadKey() const;
    Q_INVOKABLE QString stereoEnhancerMixKey() const;
    Q_INVOKABLE QString stereoEnhancerSoloKey() const;

    Q_INVOKABLE QString stereoExciterTuneKey() const;
    Q_INVOKABLE QString stereoExciterPeakKey() const;
    Q_INVOKABLE QString stereoExciterZeroFillKey() const;
    Q_INVOKABLE QString stereoExciterTimbreKey() const;
    Q_INVOKABLE QString stereoExciterHarmonicsKey() const;
    Q_INVOKABLE QString stereoExciterMixKey() const;
    Q_INVOKABLE QString stereoExciterSoloKey() const;

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
    Q_INVOKABLE float multibandCompressorBandReductionDb(quint32 effectIndex, quint32 bandIndex) const;
    Q_INVOKABLE float stereoWidenerBandCorrelation(quint32 effectIndex, quint32 bandIndex) const;
    Q_INVOKABLE float autoDuckerGainDb(quint32 effectIndex) const;
    Q_INVOKABLE float clipperReductionDb(quint32 effectIndex) const;
    Q_INVOKABLE float limiterReductionDb(quint32 effectIndex) const;
    Q_INVOKABLE float saturatorSaturationDb(quint32 effectIndex) const;
    Q_INVOKABLE float tubeStageSaturationDb(quint32 effectIndex) const;
    Q_INVOKABLE float analogFuzzSaturationDb(quint32 effectIndex) const;
    Q_INVOKABLE float bassGrinderSaturationDb(quint32 effectIndex) const;
    Q_INVOKABLE float waveDesignerShapingDb(quint32 effectIndex) const;
    Q_INVOKABLE float stereoExciterHarmonicsDb(quint32 effectIndex) const;
    Q_INVOKABLE float lufsMeterMomentary(quint32 effectIndex) const;
    Q_INVOKABLE float lufsMeterShortTerm(quint32 effectIndex) const;
    Q_INVOKABLE float lufsMeterIntegrated(quint32 effectIndex) const;
    Q_INVOKABLE void resetLufsMeter(quint32 effectIndex);
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

    Q_INVOKABLE QStringList compressorPresets() const;
    Q_INVOKABLE void applyCompressorPreset(quint32 effectIndex, quint32 presetIndex);

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

    //! An effect's settings as they were when its dialog opened. Nothing when no dialog is open or
    //! when the slot the snapshot was taken from no longer holds the same effect.
    struct EffectSnapshot
    {
        int effectIndex = -1;
        ParameterContainer::ParameterSnapshot parameters;
        bool enabled = true;
    };

    EffectTypeInfo peekEffectTypeInfo(const QUrl & fileUrl) const;
    using EffectRackOpt = std::optional<std::reference_wrapper<EffectRack>>;
    //! The rack an address points at. An empty device name is the master, where isInsertRack picks
    //! the insert or the send rack; subIndex >= 0 is a Sampler pad or a Drum Synth voice.
    EffectRackOpt rackAt(const QString & deviceName, bool isInsertRack, int subIndex) const;
    EffectRackOpt currentRack() const;

    DeviceServiceS m_deviceService;
    EditorServiceS m_editorService;
    QString m_targetDeviceName;
    int m_revision = 0;
    bool m_isInsertRack = false;
    int m_targetSubIndex = -1;
    std::optional<EffectSnapshot> m_snapshot;
};

} // namespace noteahead

#endif // EFFECT_RACK_CONTROLLER_HPP
