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
    void test_renderSampler_shouldPreserveParameters();
    void test_renderDrumSynth_shouldPreserveParameters();
    void test_render_shouldNotCrashWithNullInstrumentEvents();
    void test_render_shouldClampSignal();
    void test_render_midiSideChain_shouldProcessEventWhenSourceTrackIsMuted();
    void test_render_pitchBend_shouldProcessEvent();
};

} // namespace noteahead

#endif // RENDERING_TEST_HPP
