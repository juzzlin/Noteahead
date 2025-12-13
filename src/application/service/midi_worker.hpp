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

#ifndef MIDI_WORKER_HPP
#define MIDI_WORKER_HPP

#include <memory>

#include <QObject>

class QTimer;

namespace noteahead {

class MidiBackend;

class MidiWorker : public QObject
{
    Q_OBJECT

public:
    using MidiBackendS = std::shared_ptr<MidiBackend>;
    explicit MidiWorker(MidiBackendS midiBackend, QString role, QObject * parent = nullptr);
    virtual ~MidiWorker() override;

    Q_INVOKABLE void setIsPlaying(bool isPlaying);

signals:
    void portsChanged(QStringList portNames);
    void portsAppeared(QStringList portNames);
    void portsDisappeared(QStringList portNames);

    void statusTextRequested(QString message);

protected:
    virtual void handlePortsChanged();

    bool isPlaying() const;

    MidiBackendS midiBackend() const;

private:
    void initializeScanTimer();

    std::atomic_bool m_isPlaying = false;
    std::unique_ptr<QTimer> m_midiScanTimer;
    MidiBackendS m_midiBackend;
    QString m_role;
};

} // namespace noteahead

#endif // MIDI_WORKER_HPP
