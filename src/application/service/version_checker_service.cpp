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

#include "version_checker_service.hpp"

#include "../../common/constants.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../infra/settings.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

static const auto TAG = "VersionCheckerService";

namespace noteahead {

VersionCheckerService::VersionCheckerService(QObject * parent)
    : QObject(parent)
{
    connect(&m_networkAccessManager, &QNetworkAccessManager::finished, this, &VersionCheckerService::onReplyFinished);
}

void VersionCheckerService::check()
{
    if (!Settings::updateCheckEnabled()) {
        return;
    }

    juzzlin::L(TAG).info() << "Checking for updates...";

    QNetworkRequest request(QUrl("https://api.github.com/repos/juzzlin/Noteahead/releases/latest"));
    request.setHeader(QNetworkRequest::UserAgentHeader, "Noteahead");
    m_networkAccessManager.get(request);
}

void VersionCheckerService::onReplyFinished(QNetworkReply * reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        juzzlin::L(TAG).error() << "Failed to check for updates: " << reply->errorString().toStdString();
        return;
    }

    const auto data = reply->readAll();
    const auto json = QJsonDocument::fromJson(data);
    if (json.isNull() || !json.isObject()) {
        juzzlin::L(TAG).error() << "Failed to parse update check response.";
        return;
    }

    const auto root = json.object();
    if (!root.contains("tag_name")) {
        juzzlin::L(TAG).error() << "Update check response missing 'tag_name'.";
        return;
    }

    const auto latestVersion = root["tag_name"].toString();
    const auto currentVersion = Constants::applicationVersion();

    if (isNewerVersion(currentVersion, latestVersion)) {
        juzzlin::L(TAG).info() << "New version available: " << latestVersion.toStdString();
        emit newVersionAvailable(latestVersion);
    } else {
        juzzlin::L(TAG).info() << "No new version available. Current: " << currentVersion.toStdString() << ", latest: " << latestVersion.toStdString();
    }
}

bool VersionCheckerService::isNewerVersion(const QString & currentVersion, const QString & latestVersion)
{
    auto sanitize = [](const QString & v) {
        auto s = v;
        if (s.startsWith('v')) {
            s.remove(0, 1);
        }
        return s;
    };

    const auto cur = sanitize(currentVersion).split('.');
    const auto lat = sanitize(latestVersion).split('.');

    for (int i = 0; i < std::max(cur.size(), lat.size()); ++i) {
        const auto c = i < cur.size() ? cur[i].toInt() : 0;
        const auto l = i < lat.size() ? lat[i].toInt() : 0;

        if (l > c) {
            return true;
        }
        if (l < c) {
            return false;
        }
    }

    return false;
}

} // namespace noteahead
