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

#include "common/audio_backend.hpp"
#include "common/constants.hpp"
#include "common/denormal_protection.hpp"
#include "contrib/SimpleLogger/src/simple_logger.hpp"
#include "infra/audio/audio_engine.hpp"
#include "settings_service.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

static const auto TAG = "JackService";

JackService::JackService(SettingsServiceS settingsService, AudioEngineS audioEngine, QObject * parent)
  : QObject { parent }
  , m_settingsService { settingsService }
  , m_audioEngine { std::move(audioEngine) }
{
}

JackService::~JackService()
{
    deinitialize();
}

void JackService::onAudioBackendChanged()
{
    if (static_cast<AudioBackend>(m_settingsService->audioBackend()) == AudioBackend::Jack) {
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

    m_isDeinitializing = false;

    const jack_options_t options = JackNoStartServer;
    jack_status_t status;
    m_client = jack_client_open("Noteahead", options, &status);

    if (m_client) {
        juzzlin::L(TAG).info() << "JACK client opened: " << jack_get_client_name(m_client);

        m_inputPortL = jack_port_register(m_client, "input_1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        m_inputPortR = jack_port_register(m_client, "input_2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        m_outputPortL = jack_port_register(m_client, "output_1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        m_outputPortR = jack_port_register(m_client, "output_2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

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
    if (static_cast<AudioBackend>(m_settingsService->audioBackend()) == AudioBackend::Jack) {
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
        m_isDeinitializing = true;
        stopRecording();
        stopPlayback();
        jack_deactivate(m_client);
        jack_client_close(m_client);
        m_client = nullptr;
        m_inputPortL = nullptr;
        m_inputPortR = nullptr;
        m_outputPortL = nullptr;
        m_outputPortR = nullptr;
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
    return m_client ? jack_get_sample_rate(m_client) : static_cast<uint32_t>(Constants::defaultSampleRate());
}

jack_nframes_t JackService::currentFrame() const
{
    return m_lastFrame;
}

void JackService::startRecording(const QString & filePath)
{
    if (!m_client || filePath.isEmpty()) {
        return;
    }

    if (m_isRecording) {
        stopRecording();
    }

    try {
        // 2 seconds buffer
        m_recorder.start(filePath.toStdString(), sampleRate(), 2, sampleRate() * 2 * 2);
        m_isRecording = true;
        juzzlin::L(TAG).info() << "JACK recording started: " << filePath.toStdString();
    } catch (const std::exception & e) {
        const auto message = tr("Could not open audio file for recording: %1").arg(filePath);
        juzzlin::L(TAG).error() << message.toStdString() << " (" << e.what() << ")";
        emit errorOccurred(message);
    }
}

void JackService::stopRecording()
{
    if (!m_isRecording) {
        return;
    }

    m_isRecording = false;
    m_recorder.stop();
    juzzlin::L(TAG).info() << "JACK recording stopped";
}

bool JackService::isRecording() const
{
    return m_isRecording;
}

void JackService::startPlayback(const QString & filePath)
{
    if (!m_client || filePath.isEmpty()) {
        return;
    }

    if (m_isPlayingPlayback) {
        stopPlayback();
    }

    try {
        // 500ms buffer
        m_streamer.start(filePath.toStdString(), sampleRate() * 2 / 2, playbackPosition());
        m_isPlayingPlayback = true;
        juzzlin::L(TAG).info() << "JACK playback started: " << filePath.toStdString();
    } catch (const std::exception & e) {
        const auto message = tr("Could not open audio file for playback: %1").arg(filePath);
        juzzlin::L(TAG).error() << message.toStdString() << " (" << e.what() << ")";
        emit errorOccurred(message);
    }
}

void JackService::stopPlayback()
{
    if (!m_isPlayingPlayback) {
        return;
    }

    m_isPlayingPlayback = false;
    m_streamer.stop();
    juzzlin::L(TAG).info() << "JACK playback stopped";
}

bool JackService::isPlayingPlayback() const
{
    return m_isPlayingPlayback;
}

bool JackService::isPlayingPlaybackFinished() const
{
    return m_streamer.isFinished();
}

void JackService::setPlaybackPosition(double position)
{
    m_streamer.setPosition(position);
}

double JackService::playbackPosition() const
{
    return m_streamer.position();
}

int JackService::processCallback(jack_nframes_t frameCount, void * arg)
{
    enableHardwareDenormalProtection();

    auto self = static_cast<JackService *>(arg);

    if (self->m_isDeinitializing) {
        return 0;
    }

    if (self->m_isRecording) {
        auto inL = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_inputPortL, frameCount));
        auto inR = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_inputPortR, frameCount));

        const size_t totalSamples = frameCount * 2;
        if (self->m_recordingInterleavedBuffer.size() < totalSamples) {
            self->m_recordingInterleavedBuffer.resize(totalSamples);
        }

        for (jack_nframes_t i = 0; i < frameCount; i++) {
            self->m_recordingInterleavedBuffer[i * 2] = static_cast<int32_t>(inL[i] * 2147483647.0f);
            self->m_recordingInterleavedBuffer[i * 2 + 1] = static_cast<int32_t>(inR[i] * 2147483647.0f);
        }

        if (!self->m_recorder.push(self->m_recordingInterleavedBuffer.data(), totalSamples)) {
            // Under PipeWire, we might want to log this but very carefully (not in RT thread)
        }
    }

    if (self->m_isPlayingPlayback) {
        auto outL = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_outputPortL, frameCount));
        auto outR = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_outputPortR, frameCount));

        const size_t totalSamples = frameCount * 2;
        if (self->m_playbackInterleavedBuffer.size() < totalSamples) {
            self->m_playbackInterleavedBuffer.resize(totalSamples);
        }

        const size_t read = self->m_streamer.pop(self->m_playbackInterleavedBuffer.data(), totalSamples);

        for (jack_nframes_t i = 0; i < frameCount; i++) {
            if (i * 2 + 1 < read) {
                outL[i] = static_cast<float>(self->m_playbackInterleavedBuffer[i * 2]) / 2147483647.0f;
                outR[i] = static_cast<float>(self->m_playbackInterleavedBuffer[i * 2 + 1]) / 2147483647.0f;
            } else {
                outL[i] = 0.0f;
                outR[i] = 0.0f;
            }
        }
    } else {
        auto outL = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_outputPortL, frameCount));
        auto outR = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_outputPortR, frameCount));
        std::fill_n(outL, frameCount, 0.0f);
        std::fill_n(outR, frameCount, 0.0f);
    }

    if (self->m_audioEngine && !self->m_audioEngine->isExclusive()) {
        const size_t totalSamples = frameCount * 2;
        if (self->m_doubleAccumulationBuffer.size() < totalSamples) {
            self->m_doubleAccumulationBuffer.assign(totalSamples, 0.0);
        } else {
            std::fill(self->m_doubleAccumulationBuffer.begin(), self->m_doubleAccumulationBuffer.begin() + totalSamples, 0.0);
        }

        AudioContext audioContext { std::span(self->m_doubleAccumulationBuffer.data(), totalSamples), frameCount, self->sampleRate() };
        self->m_audioEngine->process(audioContext);

        auto outL = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_outputPortL, frameCount));
        auto outR = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_outputPortR, frameCount));
        for (jack_nframes_t i = 0; i < frameCount; i++) {
            outL[i] += static_cast<jack_default_audio_sample_t>(self->m_doubleAccumulationBuffer[i * 2]);
            outR[i] += static_cast<jack_default_audio_sample_t>(self->m_doubleAccumulationBuffer[i * 2 + 1]);
        }
    }

    jack_position_t pos;
    jack_transport_state_t state = jack_transport_query(self->m_client, &pos);

    if (self->m_settingsService->jackSyncEnabled()) {
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

        if (pos.frame < self->m_lastFrame && (self->m_lastFrame - pos.frame) > 1024) {
            juzzlin::L(TAG).debug() << "Rewind detected: " << self->m_lastFrame << " -> " << pos.frame;
            emit self->rewindRequested();
        }
    }
    self->m_lastFrame = pos.frame;

    return 0;
}
#endif

} // namespace noteahead
