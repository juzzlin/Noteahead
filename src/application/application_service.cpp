// This file is part of Cacophony.
// Copyright (C) 2024 Jussi Lind <jussi.lind@iki.fi>
//
// Cacophony is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Cacophony is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Cacophony. If not, see <http://www.gnu.org/licenses/>.

#include "application_service.hpp"

#include "../common/constants.hpp"
#include "../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../domain/instrument.hpp"
#include "../infra/midi_service.hpp"
#include "editor_service.hpp"
#include "player_service.hpp"
#include "state_machine.hpp"

#include <chrono>
#include <ranges>

namespace cacophony {

static const auto TAG = "ApplicationService";

using namespace std::chrono_literals;

ApplicationService::ApplicationService()
  : m_midiScanTimer { std::make_unique<QTimer>() }
{
    m_midiScanTimer->setInterval(2500ms);
    connect(m_midiScanTimer.get(), &QTimer::timeout, this, [this] {
        if (!m_playerService->isPlaying()) {
            m_midiService->updateAvailableDevices();
            QStringList updatedDeviceList;
            std::ranges::transform(m_midiService->listDevices(), std::back_inserter(updatedDeviceList),
                                   [](const auto & device) { return QString::fromStdString(device->portName()); });
            if (m_availableMidiPorts != updatedDeviceList) {
                QStringList newDevices;
                for (auto && port : updatedDeviceList) {
                    if (!m_availableMidiPorts.contains(port)) {
                        newDevices << port;
                    }
                }
                QStringList offDevices;
                for (auto && port : m_availableMidiPorts) {
                    if (!updatedDeviceList.contains(port)) {
                        offDevices << port;
                    }
                }
                if (!newDevices.isEmpty()) {
                    if (newDevices.size() <= 3) {
                        emit statusTextRequested(tr("New MIDI devices found: ") + newDevices.join(","));
                    } else {
                        emit statusTextRequested(tr("New MIDI device(s) found"));
                    }
                }
                if (!offDevices.isEmpty()) {
                    if (newDevices.size() <= 3) {
                        emit statusTextRequested(tr("MIDI devices went offline: ") + offDevices.join(","));
                    } else {
                        emit statusTextRequested(tr("MIDI device(s) went offline "));
                    }
                }
                m_availableMidiPorts = updatedDeviceList;
                emit availableMidiPortsChanged();
            }
        }
    });
    m_midiScanTimer->start();
}

QString ApplicationService::applicationName() const
{
    return Constants::applicationName();
}

QString ApplicationService::applicationVersion() const
{
    return Constants::applicationVersion();
}

QString ApplicationService::copyright() const
{
    return Constants::copyright();
}

QString ApplicationService::license() const
{
    return Constants::license();
}

void ApplicationService::acceptUnsavedChangesDialog()
{
    juzzlin::L(TAG).info() << "Unsaved changes accepted";
    m_stateMachine->calculateState(StateMachine::Action::UnsavedChangesDialogAccepted);
}

void ApplicationService::discardUnsavedChangesDialog()
{
    juzzlin::L(TAG).info() << "Unsaved changes discarded";
    m_stateMachine->calculateState(StateMachine::Action::UnsavedChangesDialogDiscarded);
}

void ApplicationService::rejectUnsavedChangesDialog()
{
    juzzlin::L(TAG).info() << "Unsaved changes rejected";
    m_stateMachine->calculateState(StateMachine::Action::UnsavedChangesDialogCanceled);
}

void ApplicationService::requestNewProject()
{
    juzzlin::L(TAG).info() << "'New file' requested";
    m_stateMachine->calculateState(StateMachine::Action::NewProjectRequested);
}

void ApplicationService::requestOpenProject()
{
    juzzlin::L(TAG).info() << "'Open file' requested";
    m_stateMachine->calculateState(StateMachine::Action::OpenProjectRequested);
}

void ApplicationService::requestQuit()
{
    juzzlin::L(TAG).info() << "Quit requested";
    m_stateMachine->calculateState(StateMachine::Action::QuitSelected);
}

void ApplicationService::requestSaveProject()
{
    juzzlin::L(TAG).info() << "'Save file' requested";
    m_stateMachine->calculateState(StateMachine::Action::SaveProjectRequested);
}

void ApplicationService::requestSaveProjectAs()
{
    juzzlin::L(TAG).info() << "'Save file as' requested";
    m_stateMachine->calculateState(StateMachine::Action::SaveProjectAsRequested);
}

void ApplicationService::requestPatchChange(QString portName, uint8_t channel, uint8_t patch)
{
    try {
        juzzlin::L(TAG).info() << "Patch change requested: portName = '" + portName.toStdString() + "', channel = " + std::to_string(channel) + ", patch = " + std::to_string(patch);
        if (const auto device = m_midiService->deviceByPortName(portName.toStdString()); device) {
            juzzlin::L(TAG).info() << "Mapped device index: " << device->portIndex();
            m_midiService->openDevice(device);
            m_midiService->sendPatchChange(device, channel, patch);
        } else {
            juzzlin::L(TAG).error() << "No device found for portName '" << portName.toStdString() << "'";
        }
    } catch (const std::runtime_error & e) {
        juzzlin::L(TAG).error() << e.what();
        emit statusTextRequested(tr("Error: ") + e.what());
    }
}

void ApplicationService::cancelOpenProject()
{
    m_stateMachine->calculateState(StateMachine::Action::OpeningProjectCanceled);
}

void ApplicationService::openProject(QUrl url)
{
    try {
        m_editorService->load(url.toLocalFile());
        m_stateMachine->calculateState(StateMachine::Action::ProjectOpened);
    } catch (std::exception & e) {
        const auto message = QString { "Failed to load project: %1 " }.arg(e.what());
        juzzlin::L(TAG).error() << message.toStdString();
        emit statusTextRequested(message);
    }
}

void ApplicationService::cancelSaveProjectAs()
{
    m_stateMachine->calculateState(StateMachine::Action::SavingProjectAsCanceled);
}

void ApplicationService::saveProjectAs(QUrl url)
{
    try {
        auto fileName = url.toLocalFile();
        if (!fileName.endsWith(Constants::fileFormatExtension())) {
            fileName += Constants::fileFormatExtension();
        }
        m_editorService->saveAs(fileName);
        m_stateMachine->calculateState(StateMachine::Action::ProjectSavedAs);
    } catch (std::exception & e) {
        const auto message = QString { "Failed to save project: %1 " }.arg(e.what());
        juzzlin::L(TAG).error() << message.toStdString();
        emit statusTextRequested(message);
    }
}

void ApplicationService::requestUnsavedChangesDialog()
{
    juzzlin::L(TAG).info() << "Not saved dialog requested";
    emit unsavedChangesDialogRequested();
}

void ApplicationService::requestOpenDialog()
{
    juzzlin::L(TAG).info() << "Open dialog requested";
    emit openDialogRequested();
}

void ApplicationService::requestSaveAsDialog()
{
    juzzlin::L(TAG).info() << "Save as dialog requested";
    emit saveAsDialogRequested();
}

QStringList ApplicationService::availableMidiPorts() const
{
    return m_availableMidiPorts;
}

void ApplicationService::setStateMachine(StateMachineS stateMachine)
{
    m_stateMachine = stateMachine;
}

void ApplicationService::setEditorService(EditorServiceS editorService)
{
    m_editorService = editorService;
}

void ApplicationService::setMidiService(MidiServiceS midiService)
{
    m_midiService = midiService;
}

void ApplicationService::setPlayerService(PlayerServiceS playerService)
{
    m_playerService = playerService;
}

} // namespace cacophony
