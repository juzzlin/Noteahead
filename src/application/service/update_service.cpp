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

#include "update_service.hpp"

#include "../../common/constants.hpp"
#include "../../common/semantic_version.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../infra/network/release_checker.hpp"
#include "settings_service.hpp"

#include <chrono>

namespace noteahead {

static const auto TAG = "UpdateService";

UpdateService::UpdateService(SettingsServiceS settingsService, ReleaseCheckerS releaseChecker, QString currentVersion, Clock clock, QObject * parent)
  : QObject { parent }
  , m_settingsService { std::move(settingsService) }
  , m_releaseChecker { std::move(releaseChecker) }
  , m_currentVersion { std::move(currentVersion) }
  , m_clock { std::move(clock) }
{
    if (m_releaseChecker) {
        connect(m_releaseChecker.get(), &ReleaseChecker::latestReleaseFound, this, &UpdateService::onLatestReleaseFound);
        connect(m_releaseChecker.get(), &ReleaseChecker::requestFailed, this, &UpdateService::onRequestFailed);
    }
}

UpdateService::UpdateService(SettingsServiceS settingsService, ReleaseCheckerS releaseChecker, QObject * parent)
  : UpdateService { std::move(settingsService), std::move(releaseChecker), Constants::applicationVersion(), [] { return QDateTime::currentDateTimeUtc(); }, parent }
{
}

UpdateService::~UpdateService() = default;

QString UpdateService::availableVersion() const
{
    return m_availableVersion;
}

QString UpdateService::availableUrl() const
{
    return m_availableUrl;
}

bool UpdateService::checking() const
{
    return m_checking;
}

void UpdateService::checkAtStartup()
{
    if (!m_settingsService || !m_settingsService->checkForNewVersions()) {
        juzzlin::L(TAG).info() << "Checking for new versions is turned off";
        return;
    }
    // Within the interval the last answer still stands, and is offered without asking again
    if (const auto lastCheck = m_settingsService->lastVersionCheck(); lastCheck.isValid()) {
        const auto since = std::chrono::seconds { lastCheck.secsTo(m_clock()) };
        if (since >= std::chrono::seconds::zero() && since < Constants::versionCheckInterval()) {
            if (const auto known = m_settingsService->latestKnownVersion(); !known.isEmpty()) {
                offer(known, Constants::releasePageUrl(known), false);
            }
            return;
        }
    }
    request(false);
}

void UpdateService::checkNow()
{
    request(true);
}

void UpdateService::dismiss()
{
    if (m_availableVersion.isEmpty()) {
        return;
    }
    if (m_settingsService) {
        m_settingsService->setSkippedVersion(m_availableVersion);
    }
    setAvailable({}, {});
}

void UpdateService::request(bool requestedByUser)
{
    m_requestedByUser = m_requestedByUser || requestedByUser;
    if (!m_releaseChecker) {
        return;
    }
    setChecking(true);
    m_releaseChecker->requestLatestRelease();
}

void UpdateService::onLatestReleaseFound(QString version, QString url)
{
    const bool requestedByUser = m_requestedByUser;
    m_requestedByUser = false;
    setChecking(false);
    if (m_settingsService) {
        m_settingsService->setLastVersionCheck(m_clock());
        m_settingsService->setLatestKnownVersion(version);
    }
    offer(version, url, requestedByUser);
}

void UpdateService::onRequestFailed(QString reason)
{
    const bool requestedByUser = m_requestedByUser;
    m_requestedByUser = false;
    setChecking(false);
    // Only the user who asked is told; a failed check at startup stays in the log
    if (requestedByUser) {
        emit statusTextRequested(tr("Could not check for a new version: %1").arg(reason));
    }
}

void UpdateService::offer(QString version, QString url, bool requestedByUser)
{
    if (!isNewer(version)) {
        if (requestedByUser) {
            emit statusTextRequested(tr("Noteahead is up to date"));
        }
        return;
    }
    // A dismissed version is not brought up again on its own, only when the user asks
    if (!requestedByUser && m_settingsService && m_settingsService->skippedVersion() == version) {
        return;
    }
    setAvailable(version, url);
    if (requestedByUser) {
        emit statusTextRequested(tr("Noteahead %1 is available").arg(version));
    }
}

bool UpdateService::isNewer(const QString & version) const
{
    const auto offered = SemanticVersion::parse(version.toStdString());
    const auto current = SemanticVersion::parse(m_currentVersion.toStdString());
    return offered.has_value() && current.has_value() && *offered > *current;
}

void UpdateService::setAvailable(QString version, QString url)
{
    if (m_availableVersion != version || m_availableUrl != url) {
        m_availableVersion = std::move(version);
        m_availableUrl = std::move(url);
        emit availableVersionChanged();
    }
}

void UpdateService::setChecking(bool checking)
{
    if (m_checking != checking) {
        m_checking = checking;
        emit checkingChanged();
    }
}

} // namespace noteahead
