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

#include "release_checker.hpp"

#include "../../common/constants.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <chrono>

namespace noteahead {

static const auto TAG = "ReleaseChecker";

ReleaseChecker::ReleaseChecker(QObject * parent)
  : QObject { parent }
{
}

ReleaseChecker::~ReleaseChecker() = default;

std::optional<ReleaseChecker::Release> ReleaseChecker::parseLatestRelease(const QByteArray & json)
{
    const auto document = QJsonDocument::fromJson(json);
    if (!document.isObject()) {
        return {};
    }
    const auto object = document.object();
    const auto version = SemanticVersion::parse(object.value("tag_name").toString().toStdString());
    if (!version.has_value()) {
        return {};
    }
    // The page as GitHub gives it, or as it is always made
    auto url = object.value("html_url").toString();
    if (url.isEmpty()) {
        url = Constants::releasePageUrl(QString::fromStdString(version->toString()));
    }
    return Release { .version = *version, .url = url };
}

void ReleaseChecker::requestLatestRelease()
{
    if (m_requesting) {
        return;
    }
    // Made on first use, so that a session that never checks never creates one
    if (!m_network) {
        m_network = std::make_unique<QNetworkAccessManager>();
    }
    m_requesting = true;

    QNetworkRequest request { QUrl { Constants::latestReleaseApiUrl() } };
    request.setHeader(QNetworkRequest::UserAgentHeader, Constants::applicationName() + "/" + Constants::applicationVersion());
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setTransferTimeout(static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(Constants::versionCheckTimeout()).count()));

    juzzlin::L(TAG).info() << "Asking for the latest release";
    const auto reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        m_requesting = false;
        if (reply->error() != QNetworkReply::NoError) {
            juzzlin::L(TAG).info() << "Could not get the latest release: " << reply->errorString().toStdString();
            emit requestFailed(reply->errorString());
            return;
        }
        if (const auto release = parseLatestRelease(reply->readAll()); release.has_value()) {
            juzzlin::L(TAG).info() << "The latest release is " << release->version.toString();
            emit latestReleaseFound(QString::fromStdString(release->version.toString()), release->url);
        } else {
            juzzlin::L(TAG).info() << "The answer named no release version";
            emit requestFailed(tr("The answer named no release version"));
        }
    });
}

} // namespace noteahead
