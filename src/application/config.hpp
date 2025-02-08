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

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <QObject>
#include <QSize>

namespace noteahead {

class Config : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int visibleLines READ visibleLines WRITE setVisibleLines NOTIFY visibleLinesChanged)

public:
    Config();

    ~Config() override;

    Q_INVOKABLE int autoNoteOffOffset() const;

    Q_INVOKABLE void setAutoNoteOffOffset(int autoNoteOffOffset);

    Q_INVOKABLE QSize windowSize(QSize defaultSize) const;

    Q_INVOKABLE void setWindowSize(QSize size);

    Q_INVOKABLE int step(int defaultStep) const;

    Q_INVOKABLE void setStep(int step);

    Q_INVOKABLE int velocity(int defaultVelocity) const;

    Q_INVOKABLE void setVelocity(int velocity);

    Q_INVOKABLE int visibleLines() const;

    Q_INVOKABLE void setVisibleLines(int visibleLines);

signals:
    void visibleLinesChanged();

private:
    int m_visibleLines;
};

} // namespace noteahead

#endif // CONFIG_HPP
