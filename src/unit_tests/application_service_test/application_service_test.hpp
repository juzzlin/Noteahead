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

#ifndef APPLICATION_SERVICE_TEST_HPP
#define APPLICATION_SERVICE_TEST_HPP

#include <QTest>

namespace noteahead {

class ApplicationServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void test_initialState();
    void test_applicationProperties();
    void test_editMode();
    void test_recentFiles();
    void test_stateMachineInteractions();
    void test_liveNoteLogic();
};

} // namespace noteahead

#endif // APPLICATION_SERVICE_TEST_HPP
