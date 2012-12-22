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
#include "automation_refresh_throttle.hpp"

#include "../plugins/plugin_catalog.hpp"

#include <QObject>

#include <QUrl>
#include <utility>
#include <vector>

#include <functional>
#include <memory>
#include <optional>

namespace noteahead {

class PresetService;
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
    Q_PROPERTY(QString crossfeedType READ crossfeedType CONSTANT)
    Q_PROPERTY(QString referenceType READ referenceType CONSTANT)
    Q_PROPERTY(QString gainType READ gainType CONSTANT)
    Q_PROPERTY(QString compressorType READ compressorType CONSTANT)
    Q_PROPERTY(QString multibandCompressorType READ multibandCompressorType CONSTANT)
    Q_PROPERTY(QString stereoWidenerType READ stereoWidenerType CONSTANT)
    Q_PROPERTY(QString stereoFieldMeterType READ stereoFieldMeterType CONSTANT)
    Q_PROPERTY(QString delayType READ delayType CONSTANT)
    Q_PROPERTY(QString dimensionType READ dimensionType CONSTANT)
    Q_PROPERTY(QString compressor2AType READ compressor2AType CONSTANT)
    Q_PROPERTY(QString doublerType READ doublerType CONSTANT)
    Q_PROPERTY(QString earlyReflectionsType READ earlyReflectionsType CONSTANT)
    Q_PROPERTY(QString eq8BandParametricType READ eq8BandParametricType CONSTANT)
    Q_PROPERTY(QString vintagePassiveEqType READ vintagePassiveEqType CONSTANT)
    Q_PROPERTY(QString airBandEqType READ airBandEqType CONSTANT)
    Q_PROPERTY(QString eq32BandGraphicalType READ eq32BandGraphicalType CONSTANT)
    Q_PROPERTY(QString phaseScopeType READ phaseScopeType CONSTANT)
    Q_PROPERTY(QString dynamicEqType READ dynamicEqType CONSTANT)
    Q_PROPERTY(QString ambienceType READ ambienceType CONSTANT)
    Q_PROPERTY(QString reverb224Type READ reverb224Type CONSTANT)
    Q_PROPERTY(QString simpleEqType READ simpleEqType CONSTANT)
    Q_PROPERTY(QString pannerType READ pannerType CONSTANT)
    Q_PROPERTY(QString autoPannerType READ autoPannerType CONSTANT)
    Q_PROPERTY(QString autoDuckerType READ autoDuckerType CONSTANT)
    Q_PROPERTY(QString autoFilterType READ autoFilterType CONSTANT)
    Q_PROPERTY(QString phaserType READ phaserType CONSTANT)
    Q_PROPERTY(QString reverbType READ reverbType CONSTANT)
    Q_PROPERTY(QString endlessType READ endlessType CONSTANT)
    Q_PROPERTY(QString gateType READ gateType CONSTANT)
    Q_PROPERTY(QString bitCrusherType READ bitCrusherType CONSTANT)
    Q_PROPERTY(QString flangerType READ flangerType CONSTANT)
    Q_PROPERTY(QString tremoloType READ tremoloType CONSTANT)

public:
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using EditorServiceS = std::shared_ptr<EditorService>;
    explicit EffectRackController(DeviceServiceS deviceService, EditorServiceS editorService, QObject * parent = nullptr);

    using PresetServiceS = std::shared_ptr<PresetService>;
    void setPresetService(PresetServiceS presetService);

    int effectCount() const;

    //! How many effects the given send bus runs after its own. Drives the count the master rack's
    //! send rows show, so it addresses a bus directly rather than going through the current target.
    Q_INVOKABLE int sendChainEffectCount(int busIndex) const;

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
    QString crossfeedType() const;
    QString referenceType() const;
    QString gainType() const;
    QString compressorType() const;
    QString multibandCompressorType() const;
    QString stereoWidenerType() const;
    QString stereoFieldMeterType() const;
    QString delayType() const;
    QString dimensionType() const;
    QString compressor2AType() const;
    QString doublerType() const;
    QString earlyReflectionsType() const;
    QString chorusType() const;
    QString eq8BandParametricType() const;
    QString vintagePassiveEqType() const;
    QString airBandEqType() const;
    QString eq32BandGraphicalType() const;
    QString phaseScopeType() const;
    QString dynamicEqType() const;
    QString ambienceType() const;
    QString reverb224Type() const;
    QString simpleEqType() const;
    QString pannerType() const;
    QString reverbType() const;
    QString endlessType() const;
    QString gateType() const;
    QString bitCrusherType() const;
    QString flangerType() const;
    QString tremoloType() const;

    Q_INVOKABLE QString effectParametersSummary(quint32 effectIndex) const;
    Q_INVOKABLE QString effectDisplayName(const QString & typeId) const;
    Q_INVOKABLE float parameterValue(quint32 effectIndex, const QString & paramName) const;
    //! The top of a discrete parameter's range, such as the most a Rate Divider divides by.
    Q_INVOKABLE int parameterMaximum(quint32 effectIndex, const QString & paramName) const;
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
    //! The dialog file of an effect type that registers itself, or empty for one that does not yet.
    Q_INVOKABLE QString effectDialogFile(const QString & effectType) const;

    Q_INVOKABLE bool isEffectEnabled(quint32 effectIndex) const;
    Q_INVOKABLE void setIsEffectEnabled(quint32 effectIndex, bool enabled);

    Q_INVOKABLE QStringList parameterNames(quint32 effectIndex) const;
    Q_INVOKABLE QString effectType(quint32 effectIndex) const;
    //! The id of the effect in @p effectIndex of the current rack, empty for none. See Effect::id().
    Q_INVOKABLE QString effectId(quint32 effectIndex) const;

    //! Shared by every effect whose LFO follows the Synth's shapes and modes.
    Q_INVOKABLE QStringList lfoWaveformNames() const;
    Q_INVOKABLE QStringList lfoModeNames() const;

    //! Recent sample pairs for the goniometer, flat: [l0, r0, l1, r1, ...].
    Q_INVOKABLE QVariantList stereoFieldMeterPoints(quint32 effectIndex, int maxPoints) const;
    //! Correlation, per-band correlation, mid and side levels and balance, taken together.
    Q_INVOKABLE QVariantMap stereoFieldMeterReading(quint32 effectIndex) const;
    Q_INVOKABLE void stereoFieldMeterSetActive(quint32 effectIndex, bool active);

    //! What auto gain applies at @p effectIndex, in dB: 0 while it is off, and for an effect
    //! without it.
    Q_INVOKABLE double eqAutoGainDb(quint32 effectIndex) const;

    Q_INVOKABLE float lufsMeterMomentary(quint32 effectIndex) const;
    Q_INVOKABLE float lufsMeterShortTerm(quint32 effectIndex) const;
    Q_INVOKABLE float lufsMeterIntegrated(quint32 effectIndex) const;
    Q_INVOKABLE void resetLufsMeter(quint32 effectIndex);
    Q_INVOKABLE float dbtpMeterTruePeakL(quint32 effectIndex) const;
    Q_INVOKABLE float dbtpMeterTruePeakR(quint32 effectIndex) const;
    Q_INVOKABLE float dbtpMeterTruePeakHoldL(quint32 effectIndex) const;
    Q_INVOKABLE float dbtpMeterTruePeakHoldR(quint32 effectIndex) const;

    //! Copies one frame of analysis straight into @p renderer, layout included when it has moved.
    //!
    //! One call rather than a list of levels and a list of positions fetched separately: the levels
    //! are wanted on every frame and the positions only when the band count or the sample rate
    //! changes, and neither needs to become a list of QVariants to cross a dozen pixels of C++.
    //!
    //! Declared as QObject rather than as the renderer it wants: moc records a parameter it cannot
    //! see the declaration of by the name as spelled here, and QML then looks that name up against
    //! the fully qualified one the class is registered under and finds nothing, which fails the
    //! call rather than the build. QObject is a type it always knows.
    Q_INVOKABLE void rtaUpdateRenderer(quint32 effectIndex, QObject * renderer) const;
    Q_INVOKABLE void rtaSetActive(quint32 effectIndex, bool active);

    //! Copies one frame of the spectrum an equalizer's @p renderer shows into it, layout included
    //! when it has moved. QObject for the same reason as rtaUpdateRenderer().
    Q_INVOKABLE void spectrumUpdateRenderer(quint32 effectIndex, QObject * renderer) const;
    //! Runs the taps the mode of @p renderer needs, or none when @p active is false.
    Q_INVOKABLE void spectrumSetActive(quint32 effectIndex, QObject * renderer, bool active);

    //! The effect's built-in patches followed by the user's own, numbered continuously and with a
    //! user preset marked, exactly as a device's preset dropdown is built. Empty when the effect
    //! ships no patches and the user has saved none, which is how a dialog knows to hide the row.
    Q_INVOKABLE QStringList effectPresetNames(quint32 effectIndex) const;
    //! Which entry of that list is showing. Reset to the first whenever a dialog opens, since an
    //! effect carries no memory of the preset it was last loaded from.
    Q_INVOKABLE int currentEffectPresetIndex(quint32 effectIndex) const;
    //! Loads the preset at @p presetIndex of effectPresetNames(), built-in or user alike.
    Q_INVOKABLE void loadEffectPreset(quint32 effectIndex, int presetIndex);
    //! Whether the showing entry is one of the user's own, i.e. theirs to delete.
    Q_INVOKABLE bool currentEffectPresetIsUserPreset(quint32 effectIndex) const;
    //! The showing user preset's name without the numbering, or empty when a built-in one is
    //! showing. What a delete confirmation has to quote back.
    Q_INVOKABLE QString currentEffectUserPresetName(quint32 effectIndex) const;
    //! Whether saving under @p presetName would replace a preset the user already has.
    Q_INVOKABLE bool effectUserPresetExists(quint32 effectIndex, const QString & presetName) const;
    //! Stores the effect's current parameters under @p presetName and shows it as selected.
    Q_INVOKABLE bool saveEffectUserPreset(quint32 effectIndex, const QString & presetName);
    //! Deletes the showing preset. A no-op unless the showing one is the user's own.
    Q_INVOKABLE bool deleteCurrentEffectUserPreset(quint32 effectIndex);

    //! Points the controller at the master send rack and hands back the type of the effect on
    //! @p busIndex, so a send row's title can open that effect's own dialog. Empty when the bus has
    //! no effect to open.
    Q_INVOKABLE QString openSendBusEffect(quint32 busIndex);

    //! What effectParametersSummary() gives for the current rack, for a master send bus instead.
    Q_INVOKABLE QString masterSendParametersSummary(quint32 busIndex) const;

    //! The effect at @p effectIndex in the rack now shown, or nothing when there is none there.
    //! What an EffectDialogModel reads its effect through.
    std::shared_ptr<Effect> effectAt(quint32 effectIndex) const;
    //! Sets several parameters of one effect at once, with one revision and one sync for all of them.
    void setParameterValues(quint32 effectIndex, const std::vector<std::pair<QString, float>> & values);

signals:
    void effectCountChanged();
    void revisionChanged();
    void targetDeviceNameChanged();
    void isInsertRackChanged();
    void targetSubIndexChanged();
    void rackEnabledChanged();
    void parameterChanged(quint32 effectIndex, const QString & paramName);
    //! The preset list or the selection of @p effectIndex changed.
    void effectPresetsChanged(quint32 effectIndex);
    void importEffectSettingsConfirmationRequested(int slotIndex, QUrl fileUrl, QString currentType, QString importedType, bool typeMismatch);

private:
    //! The parameter summary of one effect, whichever rack it came from.
    QString summaryFor(const EffectRack::EffectS & effect) const;
    PluginCatalog::SummaryContext summaryContext() const;

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

    //! Carries a send bus's chain of effects over with the effect itself.
    //!
    //! A send bus is its effect plus whatever the user hung after it, and copying only the effect
    //! left the target bus sounding nothing like the one it was copied from. A no-op on every other
    //! rack: only the master send rack's slots have chains behind them.
    void copySendChain(int sourceBus, int targetBus);

    //! Type id of the effect in @p effectIndex, or empty when the slot is empty. What the preset
    //! store is keyed by, so presets of one effect type are never offered for another.
    QString effectTypeId(quint32 effectIndex) const;
    QStringList effectUserPresetNames(quint32 effectIndex) const;
    int effectFactoryPresetCount(quint32 effectIndex) const;

    EffectTypeInfo peekEffectTypeInfo(const QUrl & fileUrl) const;
    using EffectRackOpt = std::optional<std::reference_wrapper<EffectRack>>;
    //! The rack an address points at. An empty device name is the master, where isInsertRack picks
    //! the insert or the send rack; subIndex >= 0 is a Sampler pad or a Drum Synth voice.
    EffectRackOpt rackAt(const QString & deviceName, bool isInsertRack, int subIndex) const;
    //! Whether the controller is on the master send rack itself, rather than on an insert rack or
    //! on the chain behind one of the send buses.
    bool isMasterSendRack() const;
    EffectRackOpt currentRack() const;

    DeviceServiceS m_deviceService;
    EditorServiceS m_editorService;
    QString m_targetDeviceName;
    int m_revision = 0;

    //! Without it the knobs sat still while automation played, then all jumped at once as soon as
    //! anything else refreshed the dialog. See AutomationRefreshThrottle.
    void handleParameterAutomation(quintptr target, qint64 delayMs);
    bool isInCurrentRack(quintptr target) const;
    AutomationRefreshThrottle m_automationRefresh;
    bool m_isInsertRack = false;
    int m_targetSubIndex = -1;
    std::optional<EffectSnapshot> m_snapshot;

    //! Which preset the open dialog is showing. One is enough for the same reason one snapshot is:
    //! only one effect dialog is ever open. Held per slot rather than on the effect because an
    //! effect edited after loading a preset is no longer that preset, and nothing should pretend it
    //! carries the name.
    int m_presetEffectIndex = -1;
    int m_presetIndex = 0;

    PresetServiceS m_presetService;
};

} // namespace noteahead

#endif // EFFECT_RACK_CONTROLLER_HPP
