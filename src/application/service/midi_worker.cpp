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

#include "midi_worker.hpp"

#include "../../common/utils.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../infra/midi/midi_backend.hpp"

#include <chrono>

#include <QTimer>

namespace noteahead {

static const auto TAG = "MidiWorker";

using namespace std::chrono_literals;

MidiWorker::MidiWorker(MidiBackendS midi, QString role, QObject * parent)
  : QObject { parent }
  , m_midiBackend { midi }
  , m_role { role }
{
    initializeScanTimer();
}

void MidiWorker::initializeScanTimer()
{
    if (!m_midiScanTimer) {
        m_midiScanTimer = std::make_unique<QTimer>();
        m_midiScanTimer->setInterval(2500ms);
        connect(m_midiScanTimer.get(), &QTimer::timeout, this, [this] {
            if (!isPlaying()) {
                const auto oldPortNames = Utils::Misc::stdStringVectorToQStringList(m_midiBackend->portNames());
                const auto availablePortNames = Utils::Misc::stdStringVectorToQStringList(m_midiBackend->availablePortNames());
                if (oldPortNames != availablePortNames || oldPortNames.empty()) {
                    updatePorts();
                    const auto updatedPortNames = Utils::Misc::stdStringVectorToQStringList(m_midiBackend->portNames());
                    QStringList newPortNames;
                    for (auto && portName : updatedPortNames) {
                        if (!oldPortNames.contains(portName)) {
                            newPortNames << portName;
                        }
                    }
                    QStringList offlinePortNames;
                    for (auto && portName : oldPortNames) {
                        if (!updatedPortNames.contains(portName)) {
                            offlinePortNames << portName;
                        }
                    }
                    if (!newPortNames.isEmpty()) {
                        for (auto && portName : newPortNames) {
                            juzzlin::L(TAG).info() << QString { "Detected MIDI %1 port " }.arg(m_role).toStdString() << portName.toStdString();
                        }
                        if (newPortNames.size() <= 3) {
                            emit statusTextRequested(tr("New MIDI %1 ports found: ").arg(m_role) + newPortNames.join(","));
                        } else {
                            emit statusTextRequested(tr("New MIDI %1 port(s) found").arg(m_role));
                        }
                    }
                    if (!offlinePortNames.isEmpty()) {
                        for (auto && portName : offlinePortNames) {
                            if (const auto port = m_midiBackend->portByName(portName.toStdString()); port) {
                                juzzlin::L(TAG).info() << QString { "Closing MIDI %1 port " }.arg(m_role).toStdString() << portName.toStdString();
                                m_midiBackend->closePort(*port);
                            }
                        }
                        if (newPortNames.size() <= 3) {
                            emit statusTextRequested(tr("MIDI %1 ports went offline: ").arg(m_role) + offlinePortNames.join(","));
                        } else {
                            emit statusTextRequested(tr("MIDI %1 port(s) went offline ").arg(m_role));
                        }
                    }

                    emit portsChanged(availablePortNames);
                    emit portsAppeared(newPortNames);
                    emit portsDisappeared(offlinePortNames);

                    handlePortsChanged();
                }
            }
        });
        m_midiScanTimer->start();
    }
}

void MidiWorker::setIsPlaying(bool isPlaying)
{
    m_isPlaying = isPlaying;
}

bool MidiWorker::isPlaying() const
{
    return m_isPlaying;
}

MidiWorker::MidiBackendS MidiWorker::midiBackend() const
{
    return m_midiBackend;
}

void MidiWorker::updatePorts()
{
    m_midiBackend->updatePorts();
}

void MidiWorker::handlePortsChanged()
{
}

MidiWorker::~MidiWorker() = default;

} // namespace noteahead