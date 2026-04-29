#ifndef SYNTH_TEST_HPP
#define SYNTH_TEST_HPP

#include <QtTest>

namespace noteahead {

class SynthTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_defaultValues_shouldBeCorrect();
    void test_oscillatorParameters_shouldSetCorrectValues();
    void test_adsrParameters_shouldSetCorrectValues();
    void test_filterParameters_shouldSetCorrectValues();
    void test_volume_shouldAdjustMasterVolume();
    void test_velocity_shouldAffectOutputLevel();
    void test_detune_shouldAdjustDetuneAmount();
    void test_reset_shouldRestoreDefaults();
    void test_serialization_shouldPreserveValues();
};

} // namespace noteahead

#endif // SYNTH_TEST_HPP
