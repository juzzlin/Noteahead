#ifndef RENDER_SERVICE_TEST_HPP
#define RENDER_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class RenderServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_renderIndividualTracks_shouldSkipNonInternalInstruments();
    void test_renderIndividualTracks_shouldRestoreMixerState();
    void test_renderMaster_secondRender_shouldStartFromZeroProgress();
};

} // namespace noteahead

#endif // RENDER_SERVICE_TEST_HPP
