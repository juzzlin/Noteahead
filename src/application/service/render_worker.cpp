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
#include "../../domain/midi/midi_cc_data.hpp"
#include "../../domain/midi/pitch_bend_data.hpp"
#include "../../domain/tracker/event.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../domain/tracker/note_data.hpp"
#include "../../domain/utility/loudness_analyzer.hpp"
#include "../../infra/audio/audio_engine.hpp"
#include "../../infra/audio/audio_file_recorder.hpp"
#include "../../infra/audio/backend/sndfile_reader.hpp"
#include "device_service.hpp"
#include "mixer_service.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <numbers>

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

void RenderWorker::render(const QString & fileName,
                          const noteahead::RenderWorker::EventList & events,
                          const noteahead::RenderWorker::Timing & timing,
                          quint64 maxTick,
                          quint32 sampleRate,
                          noteahead::RenderOptions options,
                          std::map<noteahead::AudioFileReader::TagType, std::string> tags)
{
    const auto bitDepth = options.bitDepth;
    const auto normalize = options.normalize;
    const auto analyze = options.analyze;
    const auto format = options.format;

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

        // Setup rendering path and bit depth based on two-pass options
        QString renderPath = fileName;
        QString tempPath = "";
        BitDepth actualBitDepth = bitDepth;
        if (normalize || analyze) {
            tempPath = QString { "%1/noteahead_temp_render_%2.wav" }
                         .arg(QDir::tempPath())
                         .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
            renderPath = tempPath;
            actualBitDepth = BitDepth::Float_32;
        }

        AudioFileRecorder recorder { m_audioFileReaderFactory ? m_audioFileReaderFactory() : nullptr };
        const quint32 channelCount = 2;
        const size_t recordingBufferSize = static_cast<size_t>(sampleRate) * channelCount * 10; // 10 seconds buffer
        recorder.start(renderPath.toStdString(), sampleRate, channelCount, recordingBufferSize, actualBitDepth);

        m_audioEngine->setBpm(static_cast<float>(timing.beatsPerMinute));
        m_deviceService->processMidiAllNotesOff();
        m_audioEngine->reset();

        const double secondsPerTick = 60.0 / (static_cast<double>(timing.beatsPerMinute * timing.linesPerBeat * timing.ticksPerLine));
        const double samplesPerTick = secondsPerTick * sampleRate;

        double sampleCounter = 0.0;
        std::vector<double> audioBuffer {};
        std::vector<float> finalBuffer {};

        quint64 totalFramesWritten = 0;
        const quint64 targetTrimFrames = static_cast<quint64>(options.trimMinutes * 60 + options.trimSeconds) * sampleRate;
        const quint64 silenceFrames = options.silence ? static_cast<quint64>(options.silenceSeconds * 10 + options.silenceTenths) * sampleRate / 10 : 0;
        const quint64 requestedFadeFrames = options.fadeOut ? static_cast<quint64>(options.fadeOutSeconds * 10 + options.fadeOutTenths) * sampleRate / 10 : 0;

        // Trimming fixes the length of the whole file, so the tail silence is carved out of its end.
        // Without it there is nothing to fit inside and the silence simply extends the file past the
        // song end. The prediction matches what the loop below writes to within a single sample,
        // which the fade's flat landing on zero absorbs.
        const quint64 predictedFrames = static_cast<quint64>(std::llround(static_cast<double>(maxTick + 1) * samplesPerTick));
        const quint64 targetTotalFrames = options.trim ? targetTrimFrames : predictedFrames + silenceFrames;
        const quint64 audioEndFrames = targetTotalFrames > silenceFrames ? targetTotalFrames - silenceFrames : 0;
        const quint64 fadeStartFrames = audioEndFrames > requestedFadeFrames ? audioEndFrames - requestedFadeFrames : 0;
        // A fade longer than the music itself simply fades the whole of it.
        const quint64 fadeFrames = audioEndFrames - fadeStartFrames;
        // Only trimming and the tail silence fix the length of the file. A plain render still runs
        // for as long as it runs, so that a mispredicted end cannot truncate it.
        const bool hasFixedLength = options.trim || silenceFrames > 0;

        // Equal-power cosine: it leaves the music at full level and lands on zero with zero slope, so
        // being a sample off at either end cannot be heard.
        const auto fadeGain = [fadeStartFrames, audioEndFrames, fadeFrames](quint64 frame) {
            if (frame <= fadeStartFrames) {
                return 1.0;
            }
            if (frame >= audioEndFrames) {
                return 0.0;
            }
            const double position = static_cast<double>(frame - fadeStartFrames) / static_cast<double>(fadeFrames);
            return 0.5 * (1.0 + std::cos(std::numbers::pi * position));
        };

        // Both the per-tick chunk and the sub-sample remainder below go out through this.
        const auto convertAndPush = [&](size_t totalSamples, quint64 startFrame) {
            for (size_t i = 0; i < totalSamples; i++) {
                auto sample = audioBuffer[i];
                if (options.fadeOut) {
                    sample *= fadeGain(startFrame + i / 2);
                }
                // Clamp to prevent overflow when writing to PCM, but leave Float_32 alone
                finalBuffer[i] = static_cast<float>(actualBitDepth == BitDepth::Float_32 ? sample : std::clamp(sample, -1.0, 1.0));
            }
            while (!recorder.push(finalBuffer.data(), totalSamples)) {
                std::this_thread::sleep_for(std::chrono::milliseconds { 1 });
            }
        };

        for (quint64 tick = 0; tick <= maxTick; tick++) {
            if (hasFixedLength && totalFramesWritten >= audioEndFrames) {
                break;
            }

            if (auto it = eventMap.find(tick); it != eventMap.end()) {
                for (auto && event : it->second) {
                    handleEvent(*event);
                }
            }

            sampleCounter += samplesPerTick;
            quint32 framesToProcess = static_cast<quint32>(sampleCounter);
            if (hasFixedLength && totalFramesWritten + framesToProcess > audioEndFrames) {
                framesToProcess = static_cast<quint32>(audioEndFrames - totalFramesWritten);
            }

            if (framesToProcess > 0) {
                const size_t totalSamples = static_cast<size_t>(framesToProcess) * 2;
                if (audioBuffer.size() < totalSamples) {
                    audioBuffer.resize(totalSamples);
                    finalBuffer.resize(totalSamples);
                }

                std::fill(audioBuffer.begin(), audioBuffer.begin() + totalSamples, 0.0);
                AudioContext audioContext { std::span(audioBuffer.data(), totalSamples), framesToProcess, sampleRate };
                audioContext.oversampleFactor = options.oversampleFactor;
                m_audioEngine->process(audioContext);

                convertAndPush(totalSamples, totalFramesWritten);

                totalFramesWritten += framesToProcess;
                sampleCounter -= static_cast<double>(framesToProcess);
            }

            if (tick % 100 == 0) {
                emit progressChanged(static_cast<double>(tick) / static_cast<double>(maxTick));
            }
        }

        // Process any remaining sub-sample (round to nearest)
        quint32 finalFrames = static_cast<quint32>(std::round(sampleCounter));
        if (hasFixedLength && totalFramesWritten >= audioEndFrames) {
            finalFrames = 0;
        } else if (hasFixedLength && totalFramesWritten + finalFrames > audioEndFrames) {
            finalFrames = static_cast<quint32>(audioEndFrames - totalFramesWritten);
        }

        if (finalFrames > 0) {
            const size_t totalSamples = static_cast<size_t>(finalFrames) * 2;
            if (audioBuffer.size() < totalSamples) {
                audioBuffer.resize(totalSamples);
                finalBuffer.resize(totalSamples);
            }
            std::fill(audioBuffer.begin(), audioBuffer.begin() + totalSamples, 0.0);
            AudioContext audioContext { std::span(audioBuffer.data(), totalSamples), finalFrames, sampleRate };
            audioContext.oversampleFactor = options.oversampleFactor;
            m_audioEngine->process(audioContext);

            convertAndPush(totalSamples, totalFramesWritten);

            totalFramesWritten += finalFrames;
        }

        // Pads out to the target length: the tail silence, and the shortfall when the song is shorter
        // than the trim
        if (hasFixedLength && totalFramesWritten < targetTotalFrames) {
            const quint64 remainingFrames = targetTotalFrames - totalFramesWritten;
            std::vector<float> silenceBuffer(16384 * 2, 0.0f);
            quint64 framesLeft = remainingFrames;
            while (framesLeft > 0) {
                const size_t chunkFrames = std::min(static_cast<size_t>(framesLeft), size_t { 16384 });
                while (!recorder.push(silenceBuffer.data(), chunkFrames * 2)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds { 1 });
                }
                framesLeft -= chunkFrames;
            }
            totalFramesWritten = targetTotalFrames;
        }

        juzzlin::L(TAG).info() << "Finalizing record...";
        recorder.stop();

        double gain = 1.0;
        if (normalize && !tempPath.isEmpty()) {
            const double maxPeak = runNormalizationScan(tempPath);
            if (maxPeak > 0.0) {
                gain = std::pow(10.0, options.normalizeTargetDb / 20.0) / maxPeak;
            }
            juzzlin::L(TAG).info() << "Normalization scan finished. Max peak: " << maxPeak << ", calculated gain factor: " << gain;
        }

        if (!tempPath.isEmpty()) {
            writeFinalFile(tempPath, fileName, gain, sampleRate, recordingBufferSize, bitDepth, format, tags);
        }

        QString report = "";
        if (analyze) {
            const auto result = runLoudnessAnalysis(fileName, sampleRate);
            report = formatReportHtml(result);
            writeAnalysisFile(fileName, formatReportText(result, fileName, sampleRate));
            report += QString { "<br/>Saved to: %1" }.arg(QFileInfo { analysisFilePath(fileName) }.fileName());
        }

        m_audioEngine->reset();
        m_audioEngine->setIsExclusive(false);
        m_isRendering = false;
        juzzlin::L(TAG).info() << "Render finished successfully";
        emit finished(true, report);
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
    event.visit([&](auto && data) {
        using T = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<T, NoteData>) {
            if (auto && instrument = event.instrument(); instrument) {
                if (const auto portName = instrument->midiAddress().portName(); m_deviceService->isInternalDevice(portName)) {
                    if (data.type() == NoteData::Type::NoteOff) {
                        m_deviceService->processMidiNoteOff(portName, *data.note());
                    } else if (data.type() == NoteData::Type::NoteOn && data.note().has_value()) {
                        if (m_mixerService->shouldColumnPlay(data.track(), data.column())) {
                            const auto effectiveVelocity = m_mixerService->effectiveVelocity(data.track(), data.column(), data.velocity());
                            m_deviceService->processMidiNoteOn(portName, *data.note(), effectiveVelocity);
                        }
                    }
                }
            }
        } else if constexpr (std::is_same_v<T, MidiCcData>) {
            if (auto && instrument = event.instrument(); instrument) {
                if (const auto portName = instrument->midiAddress().portName(); m_deviceService->isInternalDevice(portName)) {
                    m_deviceService->processMidiCc(portName, data.controller(), data.value(), instrument->midiAddress().channel());
                }
            }
        } else if constexpr (std::is_same_v<T, PitchBendData>) {
            if (auto && instrument = event.instrument(); instrument) {
                if (const auto portName = instrument->midiAddress().portName(); m_deviceService->isInternalDevice(portName)) {
                    m_deviceService->processMidiPitchBend(portName, (static_cast<uint16_t>(data.msb()) << 7) | data.lsb(), instrument->midiAddress().channel());
                }
            }
        } else if constexpr (std::is_same_v<T, Event::InstrumentSettingsS>) {
            if (data) {
                if (auto && instrument = event.instrument(); instrument) {
                    m_deviceService->processMidiAllNotesOff(instrument->midiAddress().portName());
                    // Settings like transpose/delay are already applied to events by Song::applyInstrumentsOnEvents
                }
            }
        }
    });
}

double RenderWorker::runNormalizationScan(const QString & tempPath)
{
    juzzlin::L(TAG).info() << "Running normalization scan on temporary file...";
    auto reader = m_audioFileReaderFactory ? m_audioFileReaderFactory() : std::make_unique<SndFileReader>();
    AudioFileReader::Info info {};
    if (!reader->open(tempPath.toStdString(), AudioFileReader::Mode::Read, info)) {
        throw std::runtime_error { "Failed to open temporary file for normalization scan: " + tempPath.toStdString() };
    }

    double maxPeak = 0.0;
    std::vector<float> scanBuffer(16384);
    int64_t read = 0;
    while ((read = reader->readFloat(scanBuffer)) > 0) {
        const int64_t numSamples = read * info.channels;
        for (int64_t i = 0; i < numSamples; i++) {
            maxPeak = std::max(maxPeak, static_cast<double>(std::abs(scanBuffer[i])));
        }
    }
    reader->close();
    return maxPeak;
}

void RenderWorker::writeFinalFile(const QString & tempPath,
                                  const QString & finalPath,
                                  double gain,
                                  quint32 sampleRate,
                                  quint32 recordingBufferSize,
                                  noteahead::BitDepth bitDepth,
                                  noteahead::AudioFormat format,
                                  const std::map<noteahead::AudioFileReader::TagType, std::string> & tags)
{
    juzzlin::L(TAG).info() << "Writing final audio to " << finalPath.toStdString();
    auto reader = m_audioFileReaderFactory ? m_audioFileReaderFactory() : std::make_unique<SndFileReader>();
    AudioFileRecorder finalRecorder { m_audioFileReaderFactory ? m_audioFileReaderFactory() : nullptr };
    finalRecorder.start(finalPath.toStdString(), sampleRate, 2, recordingBufferSize, bitDepth, format);

    for (const auto & [type, value] : tags) {
        finalRecorder.setTag(type, value);
    }

    AudioFileReader::Info info {};
    if (!reader->open(tempPath.toStdString(), AudioFileReader::Mode::Read, info)) {
        throw std::runtime_error { "Failed to open temporary file for writing pass: " + tempPath.toStdString() };
    }

    std::vector<float> writeBuffer(16384);
    int64_t read = 0;
    while ((read = reader->readFloat(writeBuffer)) > 0) {
        const int64_t numSamples = read * info.channels;
        for (int64_t i = 0; i < numSamples; i++) {
            writeBuffer[i] = static_cast<float>(std::clamp(writeBuffer[i] * gain, -1.0, 1.0));
        }
        if (!finalRecorder.push(writeBuffer.data(), numSamples)) {
            while (!finalRecorder.push(writeBuffer.data(), numSamples)) {
                std::this_thread::sleep_for(std::chrono::milliseconds { 1 });
            }
        }
    }
    reader->close();
    finalRecorder.stop();

    juzzlin::L(TAG).info() << "Removing temporary file: " << tempPath.toStdString();
    QFile::remove(tempPath);
}

LoudnessAnalyzer::Result RenderWorker::runLoudnessAnalysis(const QString & finalPath, quint32 sampleRate)
{
    juzzlin::L(TAG).info() << "Running loudness/peak analysis on final file...";
    auto reader = m_audioFileReaderFactory ? m_audioFileReaderFactory() : std::make_unique<SndFileReader>();
    AudioFileReader::Info info {};
    if (!reader->open(finalPath.toStdString(), AudioFileReader::Mode::Read, info)) {
        throw std::runtime_error { "Failed to open final file for analysis: " + finalPath.toStdString() };
    }

    LoudnessAnalyzer analyzer { static_cast<double>(sampleRate) };
    std::vector<float> analyzeBuffer(16384);
    int64_t read = 0;
    while ((read = reader->readFloat(analyzeBuffer)) > 0) {
        const int64_t numSamples = read * info.channels;
        analyzer.process(analyzeBuffer.data(), numSamples);
    }
    reader->close();

    return analyzer.calculate();
}

QString RenderWorker::analysisFilePath(const QString & renderedPath)
{
    return renderedPath + ".loudness.txt";
}

QString RenderWorker::formatReportHtml(const LoudnessAnalyzer::Result & result)
{
    QString report = QString("<table width='100%' cellpadding='5' cellspacing='0'>"
                             "<tr>"
                             "<td bgcolor='#2c2c2c'><b>Parameter</b></td><td align='right' bgcolor='#2c2c2c'><b>Value</b></td>"
                             "</tr>"
                             "<tr>"
                             "<td>Integrated Loudness</td><td align='right'><font color='#4CAF50'><b>%1 LUFS</b></font></td>"
                             "</tr>"
                             "<tr>"
                             "<td>True Peak</td><td align='right'><font color='#FF9800'><b>%2 dBTP</b></font></td>"
                             "</tr>"
                             "<tr>"
                             "<td>Loudness Range (LRA)</td><td align='right'><font color='#2196F3'><b>%3 LU</b></font></td>"
                             "</tr>"
                             "<tr>"
                             "<td>Threshold</td><td align='right'><font color='#888888'>%4 LUFS</font></td>"
                             "</tr>"
                             "</table>")
                       .arg(result.integratedLoudness, 0, 'f', 1)
                       .arg(result.truePeak, 0, 'f', 1)
                       .arg(result.loudnessRange, 0, 'f', 1)
                       .arg(result.threshold, 0, 'f', 1);

    juzzlin::L(TAG).info() << "Analysis completed:\n"
                           << report.toStdString();
    return report;
}

QString RenderWorker::formatReportText(const LoudnessAnalyzer::Result & result, const QString & renderedPath, quint32 sampleRate)
{
    const auto label = [](const QString & text, const QString & value) {
        return QString { "%1%2\n" }.arg(text, -22).arg(value);
    };
    const auto number = [](float value, const QString & unit) {
        // Written the same way the report dialog writes it, so the file and the dialog agree
        return QString { "%1 %2" }.arg(value, 0, 'f', 1).arg(unit);
    };

    QString report = "Noteahead loudness analysis\n\n";
    report += label("File:", QFileInfo { renderedPath }.fileName());
    report += label("Date:", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    report += label("Sample rate:", QString { "%1 Hz" }.arg(sampleRate));
    report += "\n";
    report += label("Integrated loudness:", number(result.integratedLoudness, "LUFS"));
    report += label("True peak:", number(result.truePeak, "dBTP"));
    report += label("Loudness range (LRA):", number(result.loudnessRange, "LU"));
    report += label("Threshold:", number(result.threshold, "LUFS"));
    return report;
}

void RenderWorker::writeAnalysisFile(const QString & renderedPath, const QString & report)
{
    const auto path = analysisFilePath(renderedPath);
    QFile file { path };
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        juzzlin::L(TAG).error() << "Failed to write the analysis file: " << path.toStdString();
        return;
    }
    QTextStream stream { &file };
    stream << report;
    file.close();
    juzzlin::L(TAG).info() << "Analysis written to " << path.toStdString();
}

} // namespace noteahead
