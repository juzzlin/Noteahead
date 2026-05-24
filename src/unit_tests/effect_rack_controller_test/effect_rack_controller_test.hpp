#ifndef EFFECT_RACK_CONTROLLER_TEST_HPP
#define EFFECT_RACK_CONTROLLER_TEST_HPP

#include <QObject>

namespace noteahead {

class EffectRackControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_effectParametersSummary_reverb();
    void test_effectParametersSummary_compressor();
    void test_effectParametersSummary_emptySlot();
};

} // namespace noteahead

#endif // EFFECT_RACK_CONTROLLER_TEST_HPP
