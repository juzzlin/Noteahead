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

#ifndef VINTAGE_PASSIVE_EQ_TEST_HPP
#define VINTAGE_PASSIVE_EQ_TEST_HPP

#include <QObject>

namespace noteahead {

class VintagePassiveEqTest : public QObject
{
    Q_OBJECT

private slots:
    void test_neutral_allZero_shouldPassThrough();
    void test_lowBoost_engaged_shouldAmplifyLowFrequency();
    void test_lowAtten_engaged_shouldAttenuateLowFrequency();
    void test_highBoost_engaged_shouldAmplifyCenterFrequency();
    void test_highAtten_engaged_shouldAttenuateHighFrequency();
    void test_lowFrequencySelector_higherCorner_shouldBoostWiderRange();
    void test_bandwidth_broader_shouldBoostWiderSkirt();
    void test_lowSection_boostAndAtten_shouldInteract();
};

} // namespace noteahead

#endif // VINTAGE_PASSIVE_EQ_TEST_HPP
