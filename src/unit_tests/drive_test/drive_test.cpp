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

#include "drive_test.hpp"
#include "../../common/constants.hpp"
#include "../../domain/effects/drive.hpp"

#include <QTest>

#include <cmath>

namespace noteahead {

namespace {
void setParameter(Drive & effect, const QString & key, float value)
{
    // Not a silent no-op on an unknown key: a parameter that has been renamed would otherwise
    // leave every test that sets it asserting against the default and still passing.
    const auto p = effect.parameter(key.toStdString());
    Q_ASSERT(p.has_value());
    p->get().update(value);
    effect.sync();
}
} // namespace

void DriveTest::test_mixZero_shouldPassSignalThrough()
{
    Drive effect;
    setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), 0.8f);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.0f);

    double l = 0.5;
    double r = -0.3;
    effect.process(l, r);

    // A fully dry mix must leave the signal untouched regardless of drive.
    QVERIFY(qFuzzyCompare(l, 0.5));
    QVERIFY(qFuzzyCompare(r, -0.3));
}

void DriveTest::test_mixFull_shouldShapeSignal()
{
    Drive effect;
    setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), 0.5f);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 1.0f);

    double l = 0.5;
    double r = 0.5;
    effect.process(l, r);

    // With drive engaged and a full wet mix, the output must differ from the input.
    QVERIFY(std::abs(l - 0.5) > 0.001);
    QVERIFY(qFuzzyCompare(l, r));
}

void DriveTest::test_higherDrive_shouldSaturateMore()
{
    Drive low;
    setParameter(low, Constants::NahdXml::xmlKeyDriveDb(), 0.1f);
    setParameter(low, Constants::NahdXml::xmlKeyMix(), 1.0f);

    Drive high;
    setParameter(high, Constants::NahdXml::xmlKeyDriveDb(), 0.9f);
    setParameter(high, Constants::NahdXml::xmlKeyMix(), 1.0f);

    double lowL = 0.5, lowR = 0.5;
    double highL = 0.5, highR = 0.5;
    low.process(lowL, lowR);
    high.process(highL, highR);

    // A hotter drive pushes a positive input further toward the saturation ceiling.
    QVERIFY(highL > lowL);
}

void DriveTest::test_hardMode_shouldClipToUnity()
{
    Drive effect;
    setParameter(effect, Constants::NahdXml::xmlKeyMode(), 1.0f); // Hard
    setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 1.0f);

    double l = 0.8;
    double r = -0.8;
    effect.process(l, r);

    // Hard clipping with a high drive gain must clamp to +/- 1.
    QVERIFY(qFuzzyCompare(l, 1.0));
    QVERIFY(qFuzzyCompare(r, -1.0));
}

void DriveTest::test_foldMode_shouldStayWithinUnity()
{
    Drive effect;
    setParameter(effect, Constants::NahdXml::xmlKeyMode(), 2.0f); // Fold
    setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), 1.0f);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 1.0f);

    for (double input = -1.0; input <= 1.0; input += 0.1) {
        double l = input;
        double r = input;
        effect.process(l, r);
        QVERIFY(l <= 1.0 + 1e-9 && l >= -1.0 - 1e-9);
    }
}

void DriveTest::test_distMode_shouldShapeAsymmetrically()
{
    Drive effect;
    setParameter(effect, Constants::NahdXml::xmlKeyMode(), 3.0f); // Dist
    setParameter(effect, Constants::NahdXml::xmlKeyDriveDb(), 0.5f);
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 1.0f);

    double posL = 0.5, posR = 0.5;
    double negL = -0.5, negR = -0.5;
    effect.process(posL, posR);
    effect.process(negL, negR);

    // Guitar-style distortion is asymmetric: equal-magnitude inputs must not produce
    // equal-magnitude outputs, and the result stays within unity.
    QVERIFY(std::abs(std::abs(posL) - std::abs(negL)) > 0.001);
    QVERIFY(posL <= 1.0 + 1e-9 && negL >= -1.0 - 1e-9);
}

void DriveTest::test_gain_shouldScaleOutput()
{
    Drive effect;
    setParameter(effect, Constants::NahdXml::xmlKeyMix(), 0.0f); // Fully dry to isolate the gain stage
    setParameter(effect, Constants::NahdXml::xmlKeyGain(), 1.0f); // +12 dB

    double l = 0.1;
    double r = 0.1;
    effect.process(l, r);

    // +12 dB is roughly a 4x linear boost.
    QVERIFY(l > 0.1);
    QVERIFY(std::abs(l - 0.1 * std::pow(10.0, 12.0 / 20.0)) < 0.01);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::DriveTest)
