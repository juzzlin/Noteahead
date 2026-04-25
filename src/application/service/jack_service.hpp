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

#ifndef JACK_SERVICE_HPP
#define JACK_SERVICE_HPP

#include <QObject>
#include <QCoreApplication>
#include <memory>
#include <atomic>
#include <thread>

#ifdef HAVE_JACK
#include <jack/jack.h>
#endif

#include <sndfile.h>
#include "../../infra/audio/ring_buffer.hpp"
#include "../../infra/audio/audio_engine.hpp"

namespace noteahead {

class SettingsService;
class AudioEngine;

class JackService : public QObject
{
    Q_OBJECT

public:
    using SettingsServiceS = std::shared_ptr<SettingsService>;
    using AudioEngineS = std::shared_ptr<AudioEngine>;
    explicit JackService(SettingsServiceS settingsService, AudioEngineS audioEngine, QObject * parent = nullptr);
    ~JackService() override;

    void initialize();

    double bpm() const;
    uint32_t sampleRate() const;
#ifdef HAVE_JACK
    jack_nframes_t currentFrame() const;
#endif

    void startRecording(const QString & filePath);
    void stopRecording();
    bool isRecording() const;

    void startPlayback(const QString & filePath);
    void stopPlayback();
    bool isPlayingPlayback() const;
    bool isPlayingPlaybackFinished() const;
    void setPlaybackPosition(double position);
    double playbackPosition() const;

signals:
    void playRequested();
    void stopRequested();
    void rewindRequested();
    void bpmChanged(double bpm);
    void errorOccurred(QString message);

private slots:
    void onJackSyncEnabledChanged();

private:
    void deinitialize();
    void diskWriteLoop();
    void diskReadLoop();

#ifdef HAVE_JACK
    static int processCallback(jack_nframes_t nframes, void * arg);
    jack_client_t * m_client = nullptr;
    jack_port_t * m_inputPortL = nullptr;
    jack_port_t * m_inputPortR = nullptr;
    jack_port_t * m_outputPortL = nullptr;
    jack_port_t * m_outputPortR = nullptr;
    jack_transport_state_t m_lastState = JackTransportStopped;
    jack_nframes_t m_lastFrame = 0;
    double m_lastBpm = 120.0;
#endif

    std::atomic<bool> m_isRecording { false };
    SNDFILE * m_sndFile = nullptr;
    SF_INFO m_sfInfo = {};
    RingBuffer<int32_t> m_recordingBuffer;
    std::thread m_diskWriteThread;
    std::atomic<bool> m_stopThread { false };

    std::atomic<bool> m_isPlayingPlayback { false };
    SNDFILE * m_playbackSndFile = nullptr;
    SF_INFO m_playbackSfInfo = {};
    RingBuffer<int32_t> m_playbackBuffer;
    std::thread m_diskReadThread;
    std::atomic<bool> m_stopPlaybackThread { false };
    std::atomic<uint64_t> m_playedPlaybackFrames { 0 };
    std::atomic<double> m_playbackPosition { 0.0 };
    std::atomic<bool> m_isPlayingPlaybackFinished { false };

    SettingsServiceS m_settingsService;
    AudioEngineS m_audioEngine;
};

} // namespace noteahead

#endif // JACK_SERVICE_HPP
