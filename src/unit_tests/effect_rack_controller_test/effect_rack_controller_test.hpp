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
    void test_effectParametersSummary_compressor_shouldReturnFormattedSummary();
    void test_effectParametersSummary_autoPanner_shouldReturnFormattedSummary();
    void test_effectParametersSummary_panner_shouldReturnFormattedSummary();
    void test_effectParametersSummary_clipper_shouldReturnFormattedSummary();
    void test_effectParametersSummary_eq8BandParametric_shouldReturnFormattedSummary();
    void test_effectParametersSummary_emptySlot_shouldReturnEmptyString();
    void test_isEffectEnabled_shouldReturnEnabledState();
    void test_revision_shouldIncrementOnPropertySet();
    void test_exportSettings_shouldSerializeEffects();
    void test_importSettings_shouldRestoreEffects();
    void test_importEffectSettings_matchingType_shouldEmitConfirmationWithoutMismatch();
    void test_importEffectSettings_differentType_shouldEmitConfirmationWithMismatch();
    void test_confirmImportEffectSettings_shouldImportAndNotify();
};

} // namespace noteahead

#endif // EFFECT_RACK_CONTROLLER_TEST_HPP
