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

#ifndef STEREO_EXCITER_TEST_HPP
#define STEREO_EXCITER_TEST_HPP

#include <QObject>

namespace noteahead {

class StereoExciterTest : public QObject
{
    Q_OBJECT

private slots:
    void test_harmonicsZero_shouldPassSignalThrough();
    void test_harmonics_shouldGenerateContentAboveTheInput();
    void test_belowTune_shouldBeLeftAlone();
    void test_timbre_odd_shouldFavourOddHarmonics();
    void test_timbre_even_shouldFavourEvenHarmonics();
    void test_solo_shouldPassOnlyTheHarmonics();
    void test_mix_zero_shouldPassSignalThrough();
};

} // namespace noteahead

#endif // STEREO_EXCITER_TEST_HPP
