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

#ifndef RELEASE_CHECKER_HPP
#define RELEASE_CHECKER_HPP

#include "../../common/semantic_version.hpp"

#include <QByteArray>
#include <QObject>
#include <QString>

#include <memory>
#include <optional>

class QNetworkAccessManager;

namespace noteahead {

//! Asks GitHub which release of Noteahead is the latest.
//!
//! One HTTPS GET to the releases API, carrying nothing but a User-Agent naming the application and
//! its version, which GitHub requires. The answer comes back as a signal; a failure (no network, a
//! timeout, an answer that cannot be read) is reported, never thrown, as nothing depends on it.
class ReleaseChecker : public QObject
{
    Q_OBJECT

public:
    struct Release
    {
        SemanticVersion version;
        QString url;
    };

    explicit ReleaseChecker(QObject * parent = nullptr);
    ~ReleaseChecker() override;

    //! Starts a request; one already under way is left to finish instead.
    virtual void requestLatestRelease();

    //! The release in a reply from the releases API: its tag as the version and its page. Nothing for
    //! a reply that is not JSON or names no release version.
    static std::optional<Release> parseLatestRelease(const QByteArray & json);

signals:
    //! The version as major.minor.patch, without any prefix the tag had.
    void latestReleaseFound(QString version, QString url);
    void requestFailed(QString reason);

private:
    std::unique_ptr<QNetworkAccessManager> m_network;
    bool m_requesting { false };
};

} // namespace noteahead

#endif // RELEASE_CHECKER_HPP
