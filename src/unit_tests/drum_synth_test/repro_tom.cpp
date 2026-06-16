#include "../../common/constants.hpp"
#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/dsp/drum/tom_engine.hpp"
#include <QTest>

using namespace noteahead;

class ReproTomTest : public QObject
{
    Q_OBJECT
private slots:

    void test_toms_shouldHaveDifferentTunes()
    {
        DrumSynthDevice device("Test");

        auto getTune = [&](int padIndex) {
            std::string prefix = "Pad" + std::to_string(padIndex) + "_";
            auto p = device.parameter(prefix + "tune");
            return p ? p->get().value() : -1.0f;
        };

        float lowTomTune = getTune(5);
        float midTomTune = getTune(6);
        float hiTomTune = getTune(7);

        qDebug() << "Low Tom Tune:" << lowTomTune;
        qDebug() << "Mid Tom Tune:" << midTomTune;
        qDebug() << "Hi Tom Tune:" << hiTomTune;

        QVERIFY(lowTomTune != midTomTune);
        QVERIFY(midTomTune != hiTomTune);
        QVERIFY(lowTomTune != hiTomTune);
    }
};

QTEST_GUILESS_MAIN(ReproTomTest)
#include "repro_tom.moc"
