#ifndef EFFECT_RACK_CONTROLLER_TEST_HPP
#define EFFECT_RACK_CONTROLLER_TEST_HPP

#include <QObject>

namespace noteahead {

class EffectRackControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void test_effectParametersSummary_reverb_shouldReturnFormattedSummary();
    void test_effectParametersSummary_endlessReverb_shouldReportPreDelay();
    void test_effectParametersSummary_compressor_shouldReturnFormattedSummary();
    void test_effectParametersSummary_autoPanner_shouldReturnFormattedSummary();
    void test_effectParametersSummary_autoFilter_shouldReturnFormattedSummary();
    void test_effectParametersSummary_phaser_shouldReturnFormattedSummary();
    void test_effectParametersSummary_panner_shouldReturnFormattedSummary();
    void test_effectParametersSummary_clipper_shouldReturnFormattedSummary();
    void test_effectParametersSummary_saturator_shouldReturnFormattedSummary();
    void test_effectParametersSummary_eq8BandParametric_shouldReturnFormattedSummary();
    void test_effectParametersSummary_emptySlot_shouldReturnEmptyString();
    void test_availableEffects_shouldBeSortedByName();
    void test_targetSubIndex_masterSendRack_shouldAddressTheBusChain();
    void test_sendChainEffectCount_shouldCountTheChainOfTheGivenBus();
    void test_effectParametersSummary_lufsMeter_shouldPadReadingsToConstantWidth();
    void test_effectParametersSummary_dbtpMeter_shouldPadReadingsToConstantWidth();
    void test_isEffectEnabled_shouldReturnEnabledState();
    void test_currentRack_drumVoiceSubIndex_shouldTargetVoiceRack();
    void test_revision_shouldIncrementOnPropertySet();
    void test_exportSettings_shouldSerializeEffects();
    void test_importSettings_shouldRestoreEffects();
    void test_importEffectSettings_matchingType_shouldEmitConfirmationWithoutMismatch();
    void test_importEffectSettings_differentType_shouldEmitConfirmationWithMismatch();
    void test_confirmImportEffectSettings_shouldImportAndNotify();
    void test_copyEffect_shouldDuplicateAndNotify();
    void test_copyRackFrom_device_shouldReplaceTargetRackAndNotify();
    void test_copyRackFrom_sameRack_shouldFail();
    void test_availableRackSources_shouldLeaveOutTheTargetRack();
    void test_revertEffect_shouldRestoreSnapshotAndNotify();
    void test_revertEffect_withoutSnapshot_shouldKeepEdits();
    void test_revertEffect_otherSlot_shouldKeepEdits();
    void test_populatedEffects_shouldReturnOnlyFilledSlots();

    void test_effectPresetNames_eq8_shouldOfferItsFactoryPresets();
    void test_effectPresetNames_effectWithoutPresets_shouldBeEmpty();
    void test_effectPresetNames_emptySlot_shouldBeEmpty();
    void test_loadEffectPreset_shouldApplyItAndNotify();
    void test_loadEffectPreset_shouldNotBeMarkedAsAUserPreset();
    void test_snapshotEffect_shouldSelectTheFirstPreset();
    void test_saveEffectUserPreset_shouldOfferItAndSelectIt();
    void test_saveEffectUserPreset_shouldRoundTripThroughTheStore();
    void test_deleteCurrentEffectUserPreset_shouldRemoveItAndClampTheSelection();
    void test_deleteCurrentEffectUserPreset_factoryPreset_shouldFail();
    void test_effectUserPresets_shouldNotLeakBetweenEffectTypes();
};

} // namespace noteahead

#endif // EFFECT_RACK_CONTROLLER_TEST_HPP
