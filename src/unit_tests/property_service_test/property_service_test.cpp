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

#include "property_service_test.hpp"

#include "../../application/service/property_service.hpp"

namespace noteahead {

void PropertyServiceTest::test_availableMidiControllers()
{
    PropertyService service;
    const auto controllers = service.availableMidiControllers();

    // Verify size
    QCOMPARE(controllers.size(), 128);

    // Verify first element (BankSelectMSB)
    const auto first = controllers.first().toMap();
    QCOMPARE(first["number"].toInt(), 0);
    QCOMPARE(first["name"].toString(), QString("0: BankSelectMSB"));

    // Verify an undefined element (CC 3)
    const auto third = controllers.at(3).toMap();
    QCOMPARE(third["number"].toInt(), 3);
    QCOMPARE(third["name"].toString(), QString("3"));

    // Verify last element (PolyModeOn)
    const auto last = controllers.last().toMap();
    QCOMPARE(last["number"].toInt(), 127);
    QCOMPARE(last["name"].toString(), QString("127: PolyModeOn"));
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::PropertyServiceTest)