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
    void test_parameterSetting_shouldUpdateValues();
    void test_polyphony_shouldActiveMultipleVoices();
    void test_presets_shouldLoadCorrectValues();
    void test_midiCc_shouldUpdateParameters();
    void test_reset_shouldRestoreDefaults();
    void test_serialization_shouldPreserveValues();
};

} // namespace noteahead

#endif // SYNTH_TEST_HPP
