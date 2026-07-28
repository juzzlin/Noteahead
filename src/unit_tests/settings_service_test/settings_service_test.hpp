// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef SETTINGS_SERVICE_TEST_HPP
#define SETTINGS_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class SettingsServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_patternPeekEnabled_setter_shouldUpdateGetterAndEmitSignal();
    void test_patternPeekEnabled_sameValue_shouldNotEmitSignal();
    void test_patternPeekEnabled_externalChange_shouldNotAffectCachedValue();

    void test_getters_shouldBeServedFromMembers();

    void test_step_unset_shouldReturnDefault();
    void test_velocity_unset_shouldReturnDefault();

    void test_windowSize_unset_shouldReturnGivenDefault();
    void test_windowSize_stored_shouldOverrideGivenDefault();

    void test_setters_shouldPersistAcrossInstances();
};

} // namespace noteahead

#endif // SETTINGS_SERVICE_TEST_HPP
