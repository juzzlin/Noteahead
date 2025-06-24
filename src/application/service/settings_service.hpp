// This file is part of Noteahead.
// Copyright (C) 2020 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef SETTINGS_SERVICE_H
#define SETTINGS_SERVICE_H

#include <QObject>
#include <QSize>

namespace noteahead {

class SettingsService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString controllerPort READ controllerPort WRITE setControllerPort NOTIFY controllerPortChanged)
    Q_PROPERTY(int visibleLines READ visibleLines WRITE setVisibleLines NOTIFY visibleLinesChanged)

public:
    SettingsService();
    ~SettingsService() override;

    Q_INVOKABLE int autoNoteOffOffset() const;
    Q_INVOKABLE void setAutoNoteOffOffset(int autoNoteOffOffset);

    Q_INVOKABLE QString controllerPort() const;
    Q_INVOKABLE void setControllerPort(QString controllerPort);

    Q_INVOKABLE QSize windowSize(QSize defaultSize) const;
    Q_INVOKABLE void setWindowSize(QSize size);

    Q_INVOKABLE int step(int defaultStep) const;
    Q_INVOKABLE void setStep(int step);

    Q_INVOKABLE int velocity(int defaultVelocity) const;
    Q_INVOKABLE void setVelocity(int velocity);

    Q_INVOKABLE int visibleLines() const;
    Q_INVOKABLE void setVisibleLines(int visibleLines);

signals:
    void controllerPortChanged();

    void visibleLinesChanged();

private:
    QString m_controllerPort;

    int m_visibleLines;
};

} // namespace noteahead

#endif // SETTINGS_SERVICE_H
