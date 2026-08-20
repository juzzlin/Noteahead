#ifndef RENDERING_TEST_HPP
#define RENDERING_TEST_HPP

#include <QObject>

namespace noteahead {

class RenderingTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_renderSynth_shouldPreserveParameters();
    void test_renderSynth_shouldNotBeSilent();
    void test_renderStringVoice_shouldNotBeSilent();
    void test_renderSampler_shouldPreserveParameters();
    void test_renderDrumSynth_shouldPreserveParameters();
    void test_render_shouldNotCrashWithNullInstrumentEvents();
    void test_render_shouldClampSignal();
    void test_render_shouldApplyTrackInstrumentSettings();
    void test_render_midiSideChain_shouldProcessEventWhenSourceTrackIsMuted();
    void test_render_pitchBend_shouldProcessEvent();
    void test_render_shouldTrimAudio();
    void test_render_silence_shouldFitInsideTrim();
    void test_render_silence_withoutTrim_shouldExtendFile();
    void test_render_fadeOut_shouldRampDownToZero();
    void test_render_shouldNormalizeAudio();

    void test_render_analysis_shouldWriteReportBesideTheRenderedFile();
    void test_render_analysisDisabled_shouldWriteNoReport();
};

} // namespace noteahead

#endif // RENDERING_TEST_HPP
