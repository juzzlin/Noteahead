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

#include "render_worker.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/event.hpp"
#include "../../domain/instrument.hpp"
#include "../../domain/midi_cc_data.hpp"
#include "../../domain/note_data.hpp"
#include "../../domain/pitch_bend_data.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/audio/audio_file_recorder.hpp"
#include "device_service.hpp"
#include "mixer_service.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

static const auto TAG = "RenderWorker";

RenderWorker::RenderWorker(AudioEngineS audioEngine, DeviceServiceS deviceService, MixerServiceS mixerService, QObject * parent)
  : QObject { parent }
  , m_audioEngine { std::move(audioEngine) }
  , m_deviceService { std::move(deviceService) }
  , m_mixerService { std::move(mixerService) }
{
}

RenderWorker::~RenderWorker() = default;

void RenderWorker::setAudioFileReaderFactory(AudioFileReaderFactory factory)
{
    m_audioFileReaderFactory = std::move(factory);
}

void RenderWorker::render(const QString & fileName, const noteahead::RenderWorker::EventList & events, const noteahead::RenderWorker::Timing & timing, quint64 maxTick, quint32 sampleRate)
{
    if (m_isRendering) {
        return;
    }

    m_isRendering = true;

    juzzlin::L(TAG).info() << "Starting render to " << fileName.toStdString() << " events=" << events.size() << " maxTick=" << maxTick;

    // Isolate engine from real-time process
    m_audioEngine->setIsExclusive(true);

    try {
        std::map<quint64, std::vector<EventS>> eventMap {};
        for (auto && event : events) {
            eventMap[event->tick()].push_back(event);
        }

        AudioFileRecorder recorder { m_audioFileReaderFactory ? m_audioFileReaderFactory() : nullptr };
        const quint32 channelCount = 2;
        const size_t recordingBufferSize = static_cast<size_t>(sampleRate) * channelCount * 10; // 10 seconds buffer
        recorder.start(fileName.toStdString(), sampleRate, channelCount, recordingBufferSize);

        m_audioEngine->setBpm(static_cast<float>(timing.beatsPerMinute));
        m_deviceService->processMidiAllNotesOff();
        m_audioEngine->reset();

        const double secondsPerTick = 60.0 / (static_cast<double>(timing.beatsPerMinute * timing.linesPerBeat * timing.ticksPerLine));
        const double samplesPerTick = secondsPerTick * sampleRate;

        double sampleCounter = 0.0;
        std::vector<float> audioBuffer {};

        for (quint64 tick = 0; tick <= maxTick; tick++) {
            if (auto it = eventMap.find(tick); it != eventMap.end()) {
                for (auto && event : it->second) {
                    handleEvent(*event);
                }
            }

            sampleCounter += samplesPerTick;
            const quint32 framesToProcess = static_cast<quint32>(sampleCounter);
            if (framesToProcess > 0) {
                if (audioBuffer.size() < framesToProcess * 2) {
                    audioBuffer.resize(framesToProcess * 2);
                }

                std::fill(audioBuffer.begin(), audioBuffer.begin() + framesToProcess * 2, 0.0f);
                AudioContext audioContext { std::span(audioBuffer.data(), static_cast<size_t>(framesToProcess) * 2), framesToProcess, sampleRate };
                m_audioEngine->process(audioContext);

                // Clamp signal to prevent overflow when writing to 16-bit PCM
                for (float & s : audioBuffer) {
                    s = std::clamp(s, -1.0f, 1.0f);
                }

                if (!recorder.push(audioBuffer.data(), framesToProcess * 2)) {
                    while (!recorder.push(audioBuffer.data(), framesToProcess * 2)) {
                        std::this_thread::sleep_for(std::chrono::milliseconds { 1 });
                    }
                }
                sampleCounter -= static_cast<double>(framesToProcess);
            }

            if (tick % 100 == 0) {
                emit progressChanged(static_cast<double>(tick) / static_cast<double>(maxTick));
            }
        }

        // Process any remaining sub-sample (round to nearest)
        const quint32 finalFrames = static_cast<quint32>(std::round(sampleCounter));
        if (finalFrames > 0) {
            if (audioBuffer.size() < finalFrames * 2) {
                audioBuffer.resize(finalFrames * 2);
            }
            std::fill(audioBuffer.begin(), audioBuffer.begin() + finalFrames * 2, 0.0f);
            AudioContext audioContext { std::span(audioBuffer.data(), static_cast<size_t>(finalFrames) * 2), finalFrames, sampleRate };
            m_audioEngine->process(audioContext);

            // Clamp signal to prevent overflow when writing to 16-bit PCM
            for (float & s : audioBuffer) {
                s = std::clamp(s, -1.0f, 1.0f);
            }

            while (!recorder.push(audioBuffer.data(), finalFrames * 2)) {
                std::this_thread::sleep_for(std::chrono::milliseconds { 1 });
            }
        }

        juzzlin::L(TAG).info() << "Finalizing record...";
        recorder.stop();
        m_audioEngine->reset();
        m_audioEngine->setIsExclusive(false);
        m_isRendering = false;
        juzzlin::L(TAG).info() << "Render finished successfully";
        emit finished(true, "");
    } catch (const std::exception & e) {
        m_audioEngine->reset();
        m_audioEngine->setIsExclusive(false);
        m_isRendering = false;
        juzzlin::L(TAG).error() << "Render failed: " << e.what();
        emit finished(false, QString::fromStdString(e.what()));
    }
}

void RenderWorker::handleEvent(const Event & event)
{
    if (event.type() == Event::Type::NoteData) {
        if (auto && noteData = event.noteData(); noteData) {
            if (auto && instrument = event.instrument(); instrument) {
                const auto portName = instrument->midiAddress().portName();
                if (m_deviceService->isInternalDevice(portName)) {
                    if (noteData->type() == NoteData::Type::NoteOff) {
                        m_deviceService->processMidiNoteOff(portName, *noteData->note());
                    } else if (noteData->type() == NoteData::Type::NoteOn && noteData->note().has_value()) {
                        if (m_mixerService->shouldColumnPlay(noteData->track(), noteData->column())) {
                            const auto effectiveVelocity = m_mixerService->effectiveVelocity(noteData->track(), noteData->column(), noteData->velocity());
                            m_deviceService->processMidiNoteOn(portName, *noteData->note(), effectiveVelocity);
                        }
                    }
                }
            }
        }
    } else if (event.type() == Event::Type::MidiCcData) {
        if (auto && ccData = event.midiCcData(); ccData) {
            if (auto && instrument = event.instrument(); instrument) {
                const auto portName = instrument->midiAddress().portName();
                if (m_deviceService->isInternalDevice(portName)) {
                    m_deviceService->processMidiCc(portName, ccData->controller(), ccData->value(), instrument->midiAddress().channel());
                }
            }
        }
    } else if (event.type() == Event::Type::PitchBendData) {
        if (auto && pbData = event.pitchBendData(); pbData) {
            if (auto && instrument = event.instrument(); instrument) {
                const auto portName = instrument->midiAddress().portName();
                if (m_deviceService->isInternalDevice(portName)) {
                    // FIXME: DeviceService doesn't have processMidiPitchBend yet, but let's keep it for future
                }
            }
        }
    } else if (event.type() == Event::Type::InstrumentSettings) {
        if (auto && instrumentSettings = event.instrumentSettings(); instrumentSettings) {
            if (auto && instrument = event.instrument(); instrument) {
                m_deviceService->processMidiAllNotesOff(instrument->midiAddress().portName());
                // Settings like transpose/delay are already applied to events by Song::applyInstrumentsOnEvents
            }
        }
    }
}

} // namespace noteahead
