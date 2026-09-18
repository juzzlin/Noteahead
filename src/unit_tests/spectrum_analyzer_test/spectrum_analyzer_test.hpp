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

#ifndef SPECTRUM_ANALYZER_TEST_HPP
#define SPECTRUM_ANALYZER_TEST_HPP

#include <QObject>

namespace noteahead {

class SpectrumAnalyzerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_bands_silence_shouldNotBeValid();
    void test_bands_belowTheGate_shouldNotBeValid();
    void test_bands_tone_shouldPeakInItsOwnBand();
    void test_bands_whiteNoise_shouldRiseThreeDbPerThird();
    void test_bands_level_shouldNotDependOnMasteredLoudness();
    void test_summary_scoopedPresence_shouldLowerPresenceAgainstHighs();
    void test_bands_mono_shouldMatchStereo();
};

} // namespace noteahead

#endif // SPECTRUM_ANALYZER_TEST_HPP
