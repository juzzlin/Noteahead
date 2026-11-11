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

#ifndef EARLY_REFLECTIONS_TEST_HPP
#define EARLY_REFLECTIONS_TEST_HPP

#include <QObject>

namespace noteahead {

class EarlyReflectionsTest : public QObject
{
    Q_OBJECT

private slots:
    void test_mix_zero_shouldLeaveTheSignalAlone();

    void test_reflections_beforeThePreDelay_shouldBeSilent();
    void test_preDelay_raised_shouldPushTheFirstReflectionLater();
    void test_size_raised_shouldSpreadTheReflectionsFurther();

    void test_reflections_shouldDifferBetweenTheChannels();
    void test_damping_raised_shouldTakeTheTopOffTheReflections();
    void test_damping_shouldDarkenLateReflectionsMoreThanEarlyOnes();

    void test_reflections_shouldStopRatherThanTail();
    void test_diffusion_raised_shouldFillTheGapsBetweenTaps();
    void test_diffusion_raised_shouldStillDecay();
};

} // namespace noteahead

#endif // EARLY_REFLECTIONS_TEST_HPP
