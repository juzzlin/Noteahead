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

#ifndef PARAMETER_MAPPER_TEST_HPP
#define PARAMETER_MAPPER_TEST_HPP

#include <QObject>

namespace noteahead {

class ParameterMapperTest : public QObject
{
    Q_OBJECT
private slots:
    void test_exponentialMapping_shouldReturnCorrectValues();
    void test_exponentialUnmapping_shouldReturnCorrectValues();
    void test_cubicMapping_shouldReturnCorrectValues();
    void test_cubicUnmapping_shouldReturnCorrectValues();
    void test_cubicCenteredMapping_shouldReturnCorrectValues();
    void test_cubicCenteredUnmapping_shouldReturnCorrectValues();
    void test_logFrequencyMapping_shouldReturnCorrectValues();
    void test_logFrequencyUnmapping_shouldReturnCorrectValues();
    void test_decibelMapping_shouldReturnCorrectValues();
    void test_decibelUnmapping_shouldReturnCorrectValues();
};

} // namespace noteahead

#endif // PARAMETER_MAPPER_TEST_HPP
