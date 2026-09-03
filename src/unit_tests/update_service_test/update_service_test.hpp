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

#ifndef UPDATE_SERVICE_TEST_HPP
#define UPDATE_SERVICE_TEST_HPP

#include <QObject>

namespace noteahead {

class UpdateServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void test_semanticVersion_parse_shouldReadReleaseVersions();
    void test_semanticVersion_parse_shouldRejectAnythingElse();
    void test_semanticVersion_compare_shouldCompareNumerically();

    void test_parseLatestRelease_valid_shouldGiveVersionAndPage();
    void test_parseLatestRelease_withoutPage_shouldMakeThePage();
    void test_parseLatestRelease_invalid_shouldGiveNothing();

    void test_checkAtStartup_turnedOff_shouldNotRequest();
    void test_checkAtStartup_neverChecked_shouldRequest();
    void test_checkAtStartup_recentCheck_shouldOfferTheKnownVersionWithoutRequesting();
    void test_checkAtStartup_oldCheck_shouldRequestAgain();
    void test_answer_newerVersion_shouldBeOfferedAndRemembered();
    void test_answer_sameOrOlderVersion_shouldOfferNothing();
    void test_answer_skippedVersion_shouldNotBeOfferedAtStartup();
    void test_dismiss_shouldSkipTheVersionAndHideIt();
    void test_checkNow_turnedOff_shouldStillRequestAndReport();
    void test_checkNow_failure_shouldReport();
    void test_checkAtStartup_failure_shouldStayQuiet();

    void test_checkAtStartup_pretending_shouldRequestDespiteRecentCheck();
    void test_checkAtStartup_pretending_shouldOfferSkippedVersion();
    void test_pretending_shouldWriteNoSettings();
};

} // namespace noteahead

#endif // UPDATE_SERVICE_TEST_HPP
