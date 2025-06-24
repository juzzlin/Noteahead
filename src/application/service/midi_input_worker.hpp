// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef MIDI_INPUT_WORKER_HPP
#define MIDI_INPUT_WORKER_HPP

#include <QObject>

namespace noteahead {

class MidiInputWorker : public QObject
{
    Q_OBJECT

public:
    explicit MidiInputWorker(QObject * parent = nullptr);

public slots:
    void setControllerPort(QString portName);

signals:

private:
    QString m_controllerPort;
};

} // namespace noteahead

#endif // MIDI_INPUT_WORKER_HPP
