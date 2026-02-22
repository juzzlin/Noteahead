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

#include "jack_service.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "settings_service.hpp"

namespace noteahead {

static const auto TAG = "JackService";

JackService::JackService(SettingsServiceS settingsService, QObject * parent)
  : QObject { parent }
  , m_settingsService { settingsService }
{
    connect(m_settingsService.get(), &SettingsService::jackSyncEnabledChanged, this, &JackService::onJackSyncEnabledChanged);
}

JackService::~JackService()
{
    deinitialize();
}

void JackService::onJackSyncEnabledChanged()
{
    if (m_settingsService->jackSyncEnabled()) {
        initialize();
    } else {
        deinitialize();
    }
}

#ifdef HAVE_JACK
static QString jackStatusToString(jack_status_t status)
{
    QStringList statusStrings;
    if (status & JackFailure)
        statusStrings << "Overall operation failed";
    if (status & JackInvalidOption)
        statusStrings << "The operation contained an invalid or unsupported option";
    if (status & JackNameNotUnique)
        statusStrings << "The desired client name was not unique";
    if (status & JackServerStarted)
        statusStrings << "The JACK server was started as a result of this operation";
    if (status & JackServerFailed)
        statusStrings << "Unable to connect to the JACK server";
    if (status & JackServerError)
        statusStrings << "Communication error with the JACK server";
    if (status & JackNoSuchClient)
        statusStrings << "Requested client does not exist";
    if (status & JackLoadFailure)
        statusStrings << "Unable to load internal client";
    if (status & JackInitFailure)
        statusStrings << "Unable to initialize client";
    if (status & JackShmFailure)
        statusStrings << "Unable to access shared memory";
    if (status & JackVersionError)
        statusStrings << "Client's protocol version does not match";
    if (status & JackBackendError)
        statusStrings << "Backend error";
    if (status & JackClientZombie)
        statusStrings << "Client is now a zombie";
    return statusStrings.join(", ");
}
#endif

void JackService::initialize()
{
#ifdef HAVE_JACK
    if (m_client) {
        return;
    }

    if (!m_settingsService->jackSyncEnabled()) {
        return;
    }

    const jack_options_t options = JackNoStartServer;
    jack_status_t status;
    m_client = jack_client_open("Noteahead", options, &status);

    if (m_client) {
        juzzlin::L(TAG).info() << "JACK client opened: " << jack_get_client_name(m_client);

        jack_position_t pos;
        jack_transport_query(m_client, &pos);
        m_lastFrame = pos.frame;

        jack_set_process_callback(m_client, &JackService::processCallback, this);
        if (jack_activate(m_client) != 0) {
            const auto message = tr("Could not activate JACK client!");
            juzzlin::L(TAG).error() << message.toStdString();
            emit errorOccurred(message);
            jack_client_close(m_client);
            m_client = nullptr;
        }
    } else {
        const auto message = tr("Could not open JACK client: %1").arg(jackStatusToString(status));
        juzzlin::L(TAG).error() << message.toStdString() << " (status " << status << ")";
        emit errorOccurred(message);
    }
#else
    if (m_settingsService->jackSyncEnabled()) {
        const auto message = tr("JACK support not compiled in!");
        juzzlin::L(TAG).warning() << message.toStdString();
        emit errorOccurred(message);
    }
#endif
}

void JackService::deinitialize()
{
#ifdef HAVE_JACK
    if (m_client) {
        jack_client_close(m_client);
        m_client = nullptr;
        juzzlin::L(TAG).info() << "JACK client closed";
    }
#endif
}

#ifdef HAVE_JACK
double JackService::bpm() const
{
    return m_lastBpm;
}

uint32_t JackService::sampleRate() const
{
    return m_client ? jack_get_sample_rate(m_client) : 48000;
}

jack_nframes_t JackService::currentFrame() const
{
    return m_lastFrame;
}

int JackService::processCallback(jack_nframes_t, void * arg)
{
    auto self = static_cast<JackService *>(arg);

    jack_position_t pos;
    jack_transport_state_t state = jack_transport_query(self->m_client, &pos);

    if (state != self->m_lastState) {
        if (state == JackTransportRolling) {
            emit self->playRequested();
        } else if (state == JackTransportStopped) {
            emit self->stopRequested();
        }
        self->m_lastState = state;
    }

    if (pos.valid & JackPositionBBT) {
        if (std::abs(pos.beats_per_minute - self->m_lastBpm) > 0.001) {
            self->m_lastBpm = pos.beats_per_minute;
            emit self->bpmChanged(self->m_lastBpm);
        }
    }

    if (pos.frame < self->m_lastFrame) {
        juzzlin::L(TAG).debug() << "Rewind detected: " << self->m_lastFrame << " -> " << pos.frame;
        emit self->rewindRequested();
    }
    self->m_lastFrame = pos.frame;

    return 0;
}
#endif

} // namespace noteahead
