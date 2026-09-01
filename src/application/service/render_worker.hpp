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

#ifndef RENDER_WORKER_HPP
#define RENDER_WORKER_HPP

#include "../../common/audio_backend.hpp"
#include "../../domain/tracker/song.hpp"
#include "../../domain/utility/loudness_analyzer.hpp"

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "../../infra/audio/backend/audio_file_reader.hpp"

#include <QObject>

namespace noteahead {

class AudioEngine;
class DeviceService;
class MixerService;
class AudioFileReader;

//! Everything the song's RenderSettings say about one export. A struct rather than yet more
//! positional arguments, because render() already took fifteen of them. Lives here at namespace
//! scope rather than nested in RenderWorker, because a nested aggregate's default member
//! initializers cannot be used in a default argument of its own enclosing class.
struct RenderOptions
{
    BitDepth bitDepth = BitDepth::PCM_16;
    AudioFormat format = AudioFormat::Wav;
    bool normalize = false;
    double normalizeTargetDb = -0.3;
    bool trim = false;
    int trimMinutes = 0;
    int trimSeconds = 0;
    //! Fades the audio out so that it reaches silence where the tail silence begins.
    bool fadeOut = false;
    int fadeOutSeconds = 0;
    int fadeOutTenths = 0;
    //! Silence ending the file. Carved out of the trimmed length when trimming, so that the trim
    //! stays the exact length of the file, and appended after the song end otherwise.
    bool silence = false;
    int silenceSeconds = 0;
    int silenceTenths = 0;
    //! Analyzes the rendered file for loudness, which is both reported back and written next to it
    //! as "<rendered file name>.loudness.txt".
    bool analyze = false;
    //! Lets the engine spread the devices over the worker threads, at the cost of a render that is
    //! no longer bit-identical from one run to the next. See RenderSettings::fastRender().
    bool fastRender = false;
    quint8 oversampleFactor = 2;
};

class RenderWorker : public QObject
{
    Q_OBJECT
public:
    using AudioEngineS = std::shared_ptr<AudioEngine>;
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using MixerServiceS = std::shared_ptr<MixerService>;
    using EventS = std::shared_ptr<Event>;
    using EventList = Song::EventList;
    using AudioFileReaderFactory = std::function<std::unique_ptr<AudioFileReader>()>;

    struct Timing
    {
        quint64 beatsPerMinute = 0;
        quint64 linesPerBeat = 0;
        quint64 ticksPerLine = 0;
    };

    RenderWorker(AudioEngineS audioEngine,
                 DeviceServiceS deviceService,
                 MixerServiceS mixerService,
                 QObject * parent = nullptr);
    ~RenderWorker() override;

    void setAudioFileReaderFactory(AudioFileReaderFactory factory);

public slots:
    void render(const QString & fileName,
                const noteahead::RenderWorker::EventList & events,
                const noteahead::RenderWorker::Timing & timing,
                quint64 maxTick,
                quint32 sampleRate,
                noteahead::RenderOptions options = {},
                std::map<noteahead::AudioFileReader::TagType, std::string> tags = {});

signals:
    void progressChanged(double progress);
    void finished(bool success, QString message);

private:
    void handleEvent(const Event & event);
    double runNormalizationScan(const QString & tempPath);
    void writeFinalFile(const QString & tempPath, const QString & finalPath, double gain, quint32 sampleRate, quint32 recordingBufferSize, noteahead::BitDepth bitDepth, noteahead::AudioFormat format, const std::map<noteahead::AudioFileReader::TagType, std::string> & tags);
    LoudnessAnalyzer::Result runLoudnessAnalysis(const QString & finalPath, quint32 sampleRate);

    //! Path of the report written beside a rendered file: its whole name plus ".loudness.txt", so
    //! that rendering the same song to both WAV and FLAC cannot have one report overwrite the other.
    static QString analysisFilePath(const QString & renderedPath);

    //! The analysis as the report dialog shows it. Both formatters read the same result, so the file
    //! and the dialog cannot end up disagreeing.
    static QString formatReportHtml(const LoudnessAnalyzer::Result & result);

    //! The analysis as it is written to disk.
    static QString formatReportText(const LoudnessAnalyzer::Result & result, const QString & renderedPath, quint32 sampleRate);

    //! Writes the report beside the rendered file. Failing to write it is logged and swallowed: the
    //! audio is what the user asked for, and it is already on disk by this point.
    static void writeAnalysisFile(const QString & renderedPath, const QString & report);

    AudioEngineS m_audioEngine;
    DeviceServiceS m_deviceService;
    MixerServiceS m_mixerService;

    AudioFileReaderFactory m_audioFileReaderFactory;

    bool m_isRendering = false;
};

} // namespace noteahead

Q_DECLARE_METATYPE(noteahead::RenderWorker::Timing)
Q_DECLARE_METATYPE(noteahead::RenderOptions)
Q_DECLARE_METATYPE(noteahead::RenderWorker::EventList)
Q_DECLARE_METATYPE(noteahead::BitDepth)

#endif // RENDER_WORKER_HPP
