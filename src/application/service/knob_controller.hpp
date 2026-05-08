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

#ifndef KNOB_CONTROLLER_HPP
#define KNOB_CONTROLLER_HPP

#include <QObject>
#include <QString>

namespace noteahead {

class KnobController : public QObject
{
    Q_OBJECT
public:
    explicit KnobController(QObject *parent = nullptr);

    Q_INVOKABLE double mapIntensity(double value, double from, double to) const;
    Q_INVOKABLE double unmapIntensity(double value, double from, double to) const;
    Q_INVOKABLE QString intensityToString(double value, double from, double to) const;

    Q_INVOKABLE double mapPan(double value, double from, double to) const;
    Q_INVOKABLE double unmapPan(double value, double from, double to) const;
    Q_INVOKABLE QString panToString(double value, double from, double to) const;

    Q_INVOKABLE double mapTime(double value, double from, double to) const;
    Q_INVOKABLE double unmapTime(double value, double from, double to) const;
    Q_INVOKABLE QString timeToString(double value, const QString & suffix) const;

    Q_INVOKABLE QString percentageToString(double value) const;
    Q_INVOKABLE QString decibelToString(double value) const;
    Q_INVOKABLE QString frequencyToString(double value, double cutoffHz, bool isHpf) const;

    Q_INVOKABLE int syncIndex(double value) const;
    Q_INVOKABLE double syncValue(int index) const;
    Q_INVOKABLE QString syncLabel(int index) const;
    Q_INVOKABLE int syncCount() const;
};

} // namespace noteahead

#endif // KNOB_CONTROLLER_HPP
