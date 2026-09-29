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

#ifndef METRONOME_TEST_HPP
#define METRONOME_TEST_HPP

#include <QObject>

namespace noteahead {

class MetronomeTest : public QObject
{
    Q_OBJECT

private slots:
    void test_render_stopped_shouldAddNothing();
    void test_render_shouldClickOnEveryBeat();
    void test_render_acrossBufferBoundaries_shouldKeepTheBeatsEvenlySpaced();
    void test_render_firstBeatOfTheBar_shouldBeAccented();
    void test_countIn_shouldFinishOnTheLastCountedBeat();
    void test_countIn_zeroBeats_shouldStartFinished();
    void test_start_afterStopping_shouldClickImmediately();
    void test_setBpm_shouldChangeTheBeatSpacing();
};

} // namespace noteahead

#endif // METRONOME_TEST_HPP
