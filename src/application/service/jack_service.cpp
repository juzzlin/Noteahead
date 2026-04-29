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
#include "../../infra/audio/audio_engine.hpp"
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
        stopRecording();
        stopPlayback();
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
    return m_client ? jack_get_sample_rate(m_client) : 48000;
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

    m_sfInfo = {};
    m_sfInfo.samplerate = static_cast<int>(sampleRate());
    m_sfInfo.channels = 2;
    m_sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;

    m_sndFile = sf_open(filePath.toStdString().c_str(), SFM_WRITE, &m_sfInfo);
    if (!m_sndFile) {
        const auto message = tr("Could not open audio file for recording: %1").arg(filePath);
        juzzlin::L(TAG).error() << message.toStdString();
        emit errorOccurred(message);
        return;
    }

    // 2 seconds buffer
    m_recordingBuffer.resize(sampleRate() * 2 * 2);

    m_stopThread = false;
    m_diskWriteThread = std::thread(&JackService::diskWriteLoop, this);

    m_isRecording = true;
    juzzlin::L(TAG).info() << "JACK recording started: " << filePath.toStdString();
}

void JackService::stopRecording()
{
    if (!m_isRecording) {
        return;
    }

    m_isRecording = false;
    m_stopThread = true;

    if (m_diskWriteThread.joinable()) {
        m_diskWriteThread.join();
    }

    if (m_sndFile) {
        sf_close(m_sndFile);
        m_sndFile = nullptr;
    }

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

    m_isPlayingPlaybackFinished = false;

    m_playbackSfInfo = {};
    m_playbackSndFile = sf_open(filePath.toStdString().c_str(), SFM_READ, &m_playbackSfInfo);
    if (!m_playbackSndFile) {
        const auto message = tr("Could not open audio file for playback: %1").arg(filePath);
        juzzlin::L(TAG).error() << message.toStdString();
        emit errorOccurred(message);
        return;
    }

    // 500ms buffer
    m_playbackBuffer.resize(sampleRate() * 2 / 2);

    // Respect existing position if set
    if (m_playbackPosition > 0.0 && m_playbackPosition < 1.0) {
        const sf_count_t targetFrame = static_cast<sf_count_t>(m_playbackPosition * m_playbackSfInfo.frames);
        sf_seek(m_playbackSndFile, targetFrame, SEEK_SET);
        m_playedPlaybackFrames = targetFrame;
    } else {
        m_playedPlaybackFrames = 0;
        m_playbackPosition = 0.0;
    }

    m_stopPlaybackThread = false;
    m_diskReadThread = std::thread(&JackService::diskReadLoop, this);

    m_isPlayingPlayback = true;
    juzzlin::L(TAG).info() << "JACK playback started: " << filePath.toStdString();
}

void JackService::stopPlayback()
{
    if (!m_isPlayingPlayback) {
        return;
    }

    m_isPlayingPlayback = false;
    m_stopPlaybackThread = true;

    if (m_diskReadThread.joinable()) {
        m_diskReadThread.join();
    }

    if (m_playbackSndFile) {
        sf_close(m_playbackSndFile);
        m_playbackSndFile = nullptr;
    }

    juzzlin::L(TAG).info() << "JACK playback stopped";
}

bool JackService::isPlayingPlayback() const
{
    return m_isPlayingPlayback;
}

bool JackService::isPlayingPlaybackFinished() const
{
    return m_isPlayingPlaybackFinished.load();
}

void JackService::setPlaybackPosition(double position)
{
    if (m_playbackSndFile && m_playbackSfInfo.frames > 0) {
        const sf_count_t targetFrame = static_cast<sf_count_t>(position * m_playbackSfInfo.frames);
        sf_seek(m_playbackSndFile, targetFrame, SEEK_SET);
        m_playbackBuffer.clear();
        m_playedPlaybackFrames = targetFrame;
        m_playbackPosition = position;
        m_isPlayingPlaybackFinished = targetFrame >= m_playbackSfInfo.frames;
    }
}

double JackService::playbackPosition() const
{
    return m_playbackPosition.load();
}

void JackService::diskWriteLoop()
{
    std::vector<int32_t> tempBuffer(16384);

    while (!m_stopThread || m_recordingBuffer.readAvailable() > 0) {
        const size_t available = m_recordingBuffer.readAvailable();
        if (available > 0) {
            const size_t toRead = std::min(available, tempBuffer.size());
            const size_t read = m_recordingBuffer.pop(tempBuffer.data(), toRead);
            if (read > 0 && m_sndFile) {
                const int channels = m_sfInfo.channels;
                if (channels > 0) {
                    sf_writef_int(m_sndFile, tempBuffer.data(), read / channels);
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void JackService::diskReadLoop()
{
    std::vector<int32_t> tempBuffer(16384);

    while (!m_stopPlaybackThread) {
        const size_t available = m_playbackBuffer.writeAvailable();
        if (available > 0 && m_playbackSndFile) {
            const size_t toRead = std::min(available, tempBuffer.size());
            const int channels = m_playbackSfInfo.channels;
            if (channels > 0) {
                const sf_count_t read = sf_readf_int(m_playbackSndFile, tempBuffer.data(), toRead / channels);
                if (read > 0) {
                    m_playbackBuffer.push(tempBuffer.data(), read * channels);
                } else {
                    // EOF or error
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

int JackService::processCallback(jack_nframes_t nframes, void * arg)
{
    auto self = static_cast<JackService *>(arg);

    if (self->m_isRecording) {
        auto inL = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_inputPortL, nframes));
        auto inR = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_inputPortR, nframes));

        // Interleave and convert to int32_t (24-bit PCM scaled to 32-bit range)
        std::vector<int32_t> interleaved(nframes * 2);
        for (jack_nframes_t i = 0; i < nframes; ++i) {
            interleaved[i * 2] = static_cast<int32_t>(inL[i] * 2147483647.0f);
            interleaved[i * 2 + 1] = static_cast<int32_t>(inR[i] * 2147483647.0f);
        }

        if (!self->m_recordingBuffer.push(interleaved.data(), interleaved.size())) {
            // Under PipeWire, we might want to log this but very carefully (not in RT thread)
        }
    }

    if (self->m_isPlayingPlayback) {
        auto outL = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_outputPortL, nframes));
        auto outR = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_outputPortR, nframes));

        std::vector<int32_t> interleaved(nframes * 2);
        const size_t read = self->m_playbackBuffer.pop(interleaved.data(), interleaved.size());

        for (jack_nframes_t i = 0; i < nframes; ++i) {
            if (i * 2 + 1 < read) {
                outL[i] = static_cast<float>(interleaved[i * 2]) / 2147483647.0f;
                outR[i] = static_cast<float>(interleaved[i * 2 + 1]) / 2147483647.0f;
            } else {
                outL[i] = 0.0f;
                outR[i] = 0.0f;
            }
        }

        if (read > 0) {
            self->m_playedPlaybackFrames += read / 2;
        }

        // Update playback position
        if (self->m_playbackSndFile && self->m_playbackSfInfo.frames > 0) {
            self->m_playbackPosition = static_cast<double>(self->m_playedPlaybackFrames.load()) / self->m_playbackSfInfo.frames;
            if (self->m_playedPlaybackFrames >= static_cast<uint64_t>(self->m_playbackSfInfo.frames)) {
                self->m_isPlayingPlaybackFinished = true;
            }
        }
    } else {
        auto outL = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_outputPortL, nframes));
        auto outR = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_outputPortR, nframes));
        std::fill_n(outL, nframes, 0.0f);
        std::fill_n(outR, nframes, 0.0f);
    }

    if (self->m_audioEngine) {
        std::vector<float> interleaved(nframes * 2, 0.0f);
        self->m_audioEngine->process(interleaved.data(), nframes, self->sampleRate());

        auto outL = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_outputPortL, nframes));
        auto outR = static_cast<jack_default_audio_sample_t *>(jack_port_get_buffer(self->m_outputPortR, nframes));
        for (jack_nframes_t i = 0; i < nframes; ++i) {
            outL[i] += interleaved[i * 2];
            outR[i] += interleaved[i * 2 + 1];
        }
    }

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
