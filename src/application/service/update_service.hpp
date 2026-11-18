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

#ifndef UPDATE_SERVICE_HPP
#define UPDATE_SERVICE_HPP

#include <QDateTime>
#include <QObject>
#include <QString>

#include <functional>
#include <memory>

namespace noteahead {

class ReleaseChecker;
class SettingsService;

//! Tells the user when a newer release of Noteahead is out.
//!
//! Decides whether to ask and what to make of the answer; ReleaseChecker does the asking. The check
//! at startup respects the user's setting and asks at most once per Constants::versionCheckInterval(),
//! showing what the last answer said in between. A check the user asks for always asks, and always
//! reports back, whether there is something new or not or the check failed. A dismissed version
//! stays dismissed until a newer one comes out.
class UpdateService : public QObject
{
    Q_OBJECT

    //! The newer version to tell the user about, or empty.
    Q_PROPERTY(QString availableVersion READ availableVersion NOTIFY availableVersionChanged)
    Q_PROPERTY(QString availableUrl READ availableUrl NOTIFY availableVersionChanged)
    Q_PROPERTY(bool checking READ checking NOTIFY checkingChanged)

public:
    using SettingsServiceS = std::shared_ptr<SettingsService>;
    using ReleaseCheckerS = std::shared_ptr<ReleaseChecker>;
    using Clock = std::function<QDateTime()>;

    //! @p currentVersion is this application's, and @p clock the time now; both are there so that
    //! tests can pin them.
    UpdateService(SettingsServiceS settingsService, ReleaseCheckerS releaseChecker, QString currentVersion, Clock clock, QObject * parent = nullptr);
    UpdateService(SettingsServiceS settingsService, ReleaseCheckerS releaseChecker, QObject * parent = nullptr);
    ~UpdateService() override;

    QString availableVersion() const;
    QString availableUrl() const;
    bool checking() const;

    //! The check at startup: nothing if the user turned it off, the last answer if it is recent.
    Q_INVOKABLE void checkAtStartup();
    //! The check the user asked for.
    Q_INVOKABLE void checkNow();
    //! Stops telling the user about the version on offer.
    Q_INVOKABLE void dismiss();

signals:
    void availableVersionChanged();
    void checkingChanged();
    void statusTextRequested(QString message);

private:
    void request(bool requestedByUser);
    void onLatestReleaseFound(QString version, QString url);
    void onRequestFailed(QString reason);
    void offer(QString version, QString url, bool requestedByUser);
    bool isNewer(const QString & version) const;
    void setAvailable(QString version, QString url);
    void setChecking(bool checking);

    SettingsServiceS m_settingsService;
    ReleaseCheckerS m_releaseChecker;
    QString m_currentVersion;
    Clock m_clock;

    QString m_availableVersion;
    QString m_availableUrl;
    bool m_checking { false };
    //! Whether the user is waiting on the request under way, and so has to hear how it went.
    bool m_requestedByUser { false };
};

} // namespace noteahead

#endif // UPDATE_SERVICE_HPP
