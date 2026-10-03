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

#ifndef PARAMETER_AUTOMATIONS_MODEL_TEST_HPP
#define PARAMETER_AUTOMATIONS_MODEL_TEST_HPP

#include <QObject>

namespace noteahead {

class ParameterAutomationsModelTest : public QObject
{
    Q_OBJECT

private slots:
    void test_setParameterAutomations_shouldExposeEveryRole();
    void test_requestParameterAutomationsByLine_shouldFilter();
    void test_setData_shouldHoldTheEditUntilApplied();
    void test_setData_target_shouldBeRefused();
    void test_removeAt_shouldDeleteOnApply();
};

} // namespace noteahead

#endif // PARAMETER_AUTOMATIONS_MODEL_TEST_HPP
