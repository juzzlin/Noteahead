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

#include "parameter_mapper_test.hpp"
#include "../../common/parameter_mapper.hpp"
#include <QTest>
#include <cmath>

namespace noteahead {

void ParameterMapperTest::test_exponentialMapping()
{
    const double min = 0.001;
    const double max = 10.0;
    
    QCOMPARE(ParameterMapper::mapExponential(0.0, min, max), min);
    QCOMPARE(ParameterMapper::mapExponential(1.0, min, max), max);
    
    // Halfway (0.5) in exponential should be sqrt(min * max)
    const double expectedMid = std::sqrt(min * max);
    QVERIFY(std::abs(ParameterMapper::mapExponential(0.5, min, max) - expectedMid) < 0.000001);
}

void ParameterMapperTest::test_exponentialUnmapping()
{
    const double min = 0.001;
    const double max = 10.0;
    
    QVERIFY(std::abs(ParameterMapper::unmapExponential(min, min, max) - 0.0) < 0.000001);
    QVERIFY(std::abs(ParameterMapper::unmapExponential(max, min, max) - 1.0) < 0.000001);
    
    const double mid = std::sqrt(min * max);
    QVERIFY(std::abs(ParameterMapper::unmapExponential(mid, min, max) - 0.5) < 0.000001);
}

void ParameterMapperTest::test_cubicMapping()
{
    const double min = 10.0;
    const double max = 110.0;
    
    QCOMPARE(ParameterMapper::mapCubic(0.0, min, max), min);
    QCOMPARE(ParameterMapper::mapCubic(1.0, min, max), max);
    
    // 0.5^3 = 0.125. Value should be 10 + 0.125 * 100 = 22.5
    QCOMPARE(ParameterMapper::mapCubic(0.5, min, max), 22.5);
}

void ParameterMapperTest::test_cubicUnmapping()
{
    const double min = 10.0;
    const double max = 110.0;
    
    QVERIFY(std::abs(ParameterMapper::unmapCubic(min, min, max) - 0.0) < 0.000001);
    QVERIFY(std::abs(ParameterMapper::unmapCubic(max, min, max) - 1.0) < 0.000001);
    QVERIFY(std::abs(ParameterMapper::unmapCubic(22.5, min, max) - 0.5) < 0.000001);
}

void ParameterMapperTest::test_cubicCenteredMapping()
{
    const double min = -100.0;
    const double max = 100.0;
    
    // Center is 0.0
    QCOMPARE(ParameterMapper::mapCubicCentered(0.0, min, max), 0.0);
    QCOMPARE(ParameterMapper::mapCubicCentered(1.0, min, max), 100.0);
    QCOMPARE(ParameterMapper::mapCubicCentered(-1.0, min, max), -100.0);
    
    // 0.5^3 = 0.125. range is 100. Should be 12.5
    QCOMPARE(ParameterMapper::mapCubicCentered(0.5, min, max), 12.5);

    // Test snapping removal: 0.1^3 = 0.001. range is 100. outVal should be 0.1.
    // It should NO LONGER snap to 0.0.
    QCOMPARE(ParameterMapper::mapCubicCentered(0.1, min, max), 0.1);
}

void ParameterMapperTest::test_cubicCenteredUnmapping()
{
    const double min = -100.0;
    const double max = 100.0;
    
    QVERIFY(std::abs(ParameterMapper::unmapCubicCentered(0.0, min, max) - 0.0) < 0.000001);
    QVERIFY(std::abs(ParameterMapper::unmapCubicCentered(100.0, min, max) - 1.0) < 0.000001);
    QVERIFY(std::abs(ParameterMapper::unmapCubicCentered(-100.0, min, max) - -1.0) < 0.000001);
    QVERIFY(std::abs(ParameterMapper::unmapCubicCentered(12.5, min, max) - 0.5) < 0.000001);
}

void ParameterMapperTest::test_logFrequencyMapping()
{
    const double maxFreq = 20000.0;
    
    QCOMPARE(ParameterMapper::mapLogFrequency(0.0, 0, maxFreq), 0.0);
    
    // Check top end
    QVERIFY(std::abs(ParameterMapper::mapLogFrequency(1.0, 0, maxFreq) - maxFreq) < 0.01);
    
    // Check a middle value (0.5)
    // f = 20000 * (pow(1000.0, 0.5) - 1.0) / 999.0
    // sqrt(1000) ~= 31.62277
    // f = 20000 * 30.62277 / 999 ~= 613.068
    const double expected = 20000.0 * (std::pow(1000.0, 0.5) - 1.0) / 999.0;
    QVERIFY(std::abs(ParameterMapper::mapLogFrequency(0.5, 0, maxFreq) - expected) < 0.000001);
}

void ParameterMapperTest::test_logFrequencyUnmapping()
{
    const double maxFreq = 20000.0;
    
    QVERIFY(std::abs(ParameterMapper::unmapLogFrequency(0.0, 0, maxFreq) - 0.0) < 0.000001);
    QVERIFY(std::abs(ParameterMapper::unmapLogFrequency(maxFreq, 0, maxFreq) - 1.0) < 0.000001);
    
    const double midFreq = 20000.0 * (std::pow(1000.0, 0.5) - 1.0) / 999.0;
    QVERIFY(std::abs(ParameterMapper::unmapLogFrequency(midFreq, 0, maxFreq) - 0.5) < 0.000001);
}

} // namespace noteahead

QTEST_APPLESS_MAIN(noteahead::ParameterMapperTest)
