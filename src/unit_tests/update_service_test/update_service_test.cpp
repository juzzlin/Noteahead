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

#include "update_service_test.hpp"

#include "../../application/service/settings_service.hpp"
#include "../../application/service/update_service.hpp"
#include "../../common/constants.hpp"
#include "../../common/semantic_version.hpp"
#include "../../infra/network/release_checker.hpp"

#include <QSignalSpy>
#include <QTest>
#include <QTimeZone>

#include <memory>

namespace noteahead {

namespace {

//! Counts the requests and never touches the network; the test delivers the answer.
class FakeReleaseChecker : public ReleaseChecker
{
public:
    void requestLatestRelease() override
    {
        requests++;
    }

    int requests { 0 };
};

//! Keeps the version check's settings in memory, so that no test writes the user's.
class StubSettingsService : public SettingsService
{
public:
    bool checkForNewVersions() const override
    {
        return enabled;
    }

    void setCheckForNewVersions(bool value) override
    {
        enabled = value;
    }

    QDateTime lastVersionCheck() const override
    {
        return lastCheck;
    }

    void setLastVersionCheck(QDateTime time) override
    {
        lastCheck = time;
    }

    QString latestKnownVersion() const override
    {
        return knownVersion;
    }

    void setLatestKnownVersion(QString version) override
    {
        knownVersion = version;
    }

    QString skippedVersion() const override
    {
        return skipped;
    }

    void setSkippedVersion(QString version) override
    {
        skipped = version;
    }

    bool enabled { true };
    QDateTime lastCheck;
    QString knownVersion;
    QString skipped;
};

const QDateTime Now { QDate { 2026, 11, 17 }, QTime { 12, 0 }, QTimeZone::utc() };
const QString CurrentVersion { "12.1.1" };

struct Fixture
{
    Fixture()
      : settings { std::make_shared<StubSettingsService>() }
      , checker { std::make_shared<FakeReleaseChecker>() }
      , service { settings, checker, CurrentVersion, [] { return Now; } }
    {
    }

    std::shared_ptr<StubSettingsService> settings;
    std::shared_ptr<FakeReleaseChecker> checker;
    UpdateService service;
};

} // namespace

void UpdateServiceTest::test_semanticVersion_parse_shouldReadReleaseVersions()
{
    QVERIFY((SemanticVersion::parse("12.1.1") == SemanticVersion { 12, 1, 1 }));
    QVERIFY((SemanticVersion::parse("v13.0.0") == SemanticVersion { 13, 0, 0 }));
    QVERIFY((SemanticVersion::parse("0.10.200") == SemanticVersion { 0, 10, 200 }));
}

void UpdateServiceTest::test_semanticVersion_parse_shouldRejectAnythingElse()
{
    for (const auto text : { "", "12", "12.1", "12.1.", "12.1.1.0", "12.2.0-beta", "a.b.c", "12..1", "-1.0.0", " 12.1.1" }) {
        QVERIFY2(!SemanticVersion::parse(text).has_value(), text);
    }
}

void UpdateServiceTest::test_semanticVersion_compare_shouldCompareNumerically()
{
    QVERIFY(*SemanticVersion::parse("12.10.0") > *SemanticVersion::parse("12.9.0"));
    QVERIFY(*SemanticVersion::parse("12.1.1") > *SemanticVersion::parse("12.1.0"));
    QVERIFY(*SemanticVersion::parse("13.0.0") > *SemanticVersion::parse("12.99.99"));
    QVERIFY(*SemanticVersion::parse("12.1.1") == *SemanticVersion::parse("v12.1.1"));
}

void UpdateServiceTest::test_parseLatestRelease_valid_shouldGiveVersionAndPage()
{
    const auto release = ReleaseChecker::parseLatestRelease(R"({"tag_name": "13.0.0", "html_url": "https://github.com/juzzlin/Noteahead/releases/tag/13.0.0", "draft": false})");

    QVERIFY(release.has_value());
    QVERIFY(release->version == (SemanticVersion { 13, 0, 0 }));
    QCOMPARE(release->url, QString { "https://github.com/juzzlin/Noteahead/releases/tag/13.0.0" });
}

void UpdateServiceTest::test_parseLatestRelease_withoutPage_shouldMakeThePage()
{
    const auto release = ReleaseChecker::parseLatestRelease(R"({"tag_name": "v13.0.0"})");

    QVERIFY(release.has_value());
    QCOMPARE(release->url, Constants::releasePageUrl("13.0.0"));
}

void UpdateServiceTest::test_parseLatestRelease_invalid_shouldGiveNothing()
{
    QVERIFY(!ReleaseChecker::parseLatestRelease("").has_value());
    QVERIFY(!ReleaseChecker::parseLatestRelease("<html>").has_value());
    QVERIFY(!ReleaseChecker::parseLatestRelease(R"({"message": "API rate limit exceeded"})").has_value());
    QVERIFY(!ReleaseChecker::parseLatestRelease(R"({"tag_name": "nightly"})").has_value());
    QVERIFY(!ReleaseChecker::parseLatestRelease(R"([{"tag_name": "13.0.0"}])").has_value());
}

void UpdateServiceTest::test_checkAtStartup_turnedOff_shouldNotRequest()
{
    Fixture fixture;
    fixture.settings->enabled = false;

    fixture.service.checkAtStartup();

    QCOMPARE(fixture.checker->requests, 0);
}

void UpdateServiceTest::test_checkAtStartup_neverChecked_shouldRequest()
{
    Fixture fixture;

    fixture.service.checkAtStartup();

    QCOMPARE(fixture.checker->requests, 1);
    QVERIFY(fixture.service.checking());
}

void UpdateServiceTest::test_checkAtStartup_recentCheck_shouldOfferTheKnownVersionWithoutRequesting()
{
    Fixture fixture;
    fixture.settings->lastCheck = Now.addSecs(-3600);
    fixture.settings->knownVersion = "13.0.0";

    fixture.service.checkAtStartup();

    QCOMPARE(fixture.checker->requests, 0);
    QCOMPARE(fixture.service.availableVersion(), QString { "13.0.0" });
    QCOMPARE(fixture.service.availableUrl(), Constants::releasePageUrl("13.0.0"));
}

void UpdateServiceTest::test_checkAtStartup_oldCheck_shouldRequestAgain()
{
    Fixture fixture;
    fixture.settings->lastCheck = Now.addDays(-2);
    fixture.settings->knownVersion = "13.0.0";

    fixture.service.checkAtStartup();

    QCOMPARE(fixture.checker->requests, 1);
}

void UpdateServiceTest::test_answer_newerVersion_shouldBeOfferedAndRemembered()
{
    Fixture fixture;
    QSignalSpy availableSpy { &fixture.service, &UpdateService::availableVersionChanged };
    QSignalSpy statusSpy { &fixture.service, &UpdateService::statusTextRequested };

    fixture.service.checkAtStartup();
    emit fixture.checker->latestReleaseFound("12.10.0", "https://example.org/12.10.0");

    QCOMPARE(fixture.service.availableVersion(), QString { "12.10.0" });
    QCOMPARE(fixture.service.availableUrl(), QString { "https://example.org/12.10.0" });
    QCOMPARE(availableSpy.count(), 1);
    QVERIFY(!fixture.service.checking());
    QCOMPARE(fixture.settings->lastCheck, Now);
    QCOMPARE(fixture.settings->knownVersion, QString { "12.10.0" });
    // A check nobody asked for shows the notice and says nothing more
    QCOMPARE(statusSpy.count(), 0);
}

void UpdateServiceTest::test_answer_sameOrOlderVersion_shouldOfferNothing()
{
    for (const auto version : { "12.1.1", "12.1.0", "11.9.9" }) {
        Fixture fixture;
        fixture.service.checkAtStartup();
        emit fixture.checker->latestReleaseFound(version, {});
        QVERIFY2(fixture.service.availableVersion().isEmpty(), version);
    }
}

void UpdateServiceTest::test_answer_skippedVersion_shouldNotBeOfferedAtStartup()
{
    Fixture fixture;
    fixture.settings->skipped = "13.0.0";

    fixture.service.checkAtStartup();
    emit fixture.checker->latestReleaseFound("13.0.0", {});
    QVERIFY(fixture.service.availableVersion().isEmpty());

    // A newer one than the one skipped is offered again
    fixture.settings->lastCheck = {};
    fixture.service.checkAtStartup();
    emit fixture.checker->latestReleaseFound("13.1.0", {});
    QCOMPARE(fixture.service.availableVersion(), QString { "13.1.0" });
}

void UpdateServiceTest::test_dismiss_shouldSkipTheVersionAndHideIt()
{
    Fixture fixture;
    fixture.service.checkAtStartup();
    emit fixture.checker->latestReleaseFound("13.0.0", {});

    fixture.service.dismiss();

    QVERIFY(fixture.service.availableVersion().isEmpty());
    QCOMPARE(fixture.settings->skipped, QString { "13.0.0" });
}

void UpdateServiceTest::test_checkNow_turnedOff_shouldStillRequestAndReport()
{
    Fixture fixture;
    fixture.settings->enabled = false;
    fixture.settings->skipped = "13.0.0";
    QSignalSpy statusSpy { &fixture.service, &UpdateService::statusTextRequested };

    fixture.service.checkNow();
    QCOMPARE(fixture.checker->requests, 1);
    // Asked for, even a skipped version is offered
    emit fixture.checker->latestReleaseFound("13.0.0", {});
    QCOMPARE(fixture.service.availableVersion(), QString { "13.0.0" });
    QCOMPARE(statusSpy.count(), 1);

    // And an answer of nothing new is reported too
    fixture.service.checkNow();
    emit fixture.checker->latestReleaseFound("12.1.1", {});
    QCOMPARE(statusSpy.count(), 2);
}

void UpdateServiceTest::test_checkNow_failure_shouldReport()
{
    Fixture fixture;
    QSignalSpy statusSpy { &fixture.service, &UpdateService::statusTextRequested };

    fixture.service.checkNow();
    emit fixture.checker->requestFailed("Host not found");

    QCOMPARE(statusSpy.count(), 1);
    QVERIFY(statusSpy.at(0).at(0).toString().contains("Host not found"));
    QVERIFY(!fixture.service.checking());
    // A failed check is not remembered as a check
    QVERIFY(!fixture.settings->lastCheck.isValid());
}

void UpdateServiceTest::test_checkAtStartup_failure_shouldStayQuiet()
{
    Fixture fixture;
    QSignalSpy statusSpy { &fixture.service, &UpdateService::statusTextRequested };

    fixture.service.checkAtStartup();
    emit fixture.checker->requestFailed("Host not found");

    QCOMPARE(statusSpy.count(), 0);
    QVERIFY(!fixture.service.checking());
}

void UpdateServiceTest::test_checkAtStartup_pretending_shouldRequestDespiteRecentCheck()
{
    Fixture fixture;
    fixture.settings->enabled = false;
    fixture.settings->lastCheck = Now.addSecs(-3600);
    fixture.settings->knownVersion = "12.1.1";
    fixture.service.pretendToBe("1.0.0");

    fixture.service.checkAtStartup();

    QCOMPARE(fixture.checker->requests, 1);
}

void UpdateServiceTest::test_checkAtStartup_pretending_shouldOfferSkippedVersion()
{
    Fixture fixture;
    fixture.settings->skipped = "12.1.1";
    fixture.service.pretendToBe("1.0.0");

    fixture.service.checkAtStartup();
    emit fixture.checker->latestReleaseFound("12.1.1", {});

    QCOMPARE(fixture.service.availableVersion(), QString { "12.1.1" });
}

void UpdateServiceTest::test_pretending_shouldWriteNoSettings()
{
    Fixture fixture;
    fixture.service.pretendToBe("1.0.0");

    fixture.service.checkAtStartup();
    emit fixture.checker->latestReleaseFound("12.1.1", {});
    fixture.service.dismiss();

    QVERIFY(!fixture.settings->lastCheck.isValid());
    QVERIFY(fixture.settings->knownVersion.isEmpty());
    QVERIFY(fixture.settings->skipped.isEmpty());
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::UpdateServiceTest)
