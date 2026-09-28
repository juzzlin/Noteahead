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

#include "audio_analysis_worker.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/utility/loudness_analyzer.hpp"
#include "../../domain/utility/spectrum_analyzer.hpp"
#include "../../infra/audio/backend/sndfile_reader.hpp"

#include <QCoreApplication>
#include <QFileInfo>

#include <vector>

namespace noteahead {

static const auto TAG = "AudioAnalysisWorker";

AudioAnalysisWorker::AudioAnalysisWorker(QObject * parent)
  : QObject { parent }
{
}

void AudioAnalysisWorker::setAudioFileReaderFactory(AudioFileReaderFactory factory)
{
    m_audioFileReaderFactory = std::move(factory);
}

void AudioAnalysisWorker::analyze(int side, QString filePath)
{
    juzzlin::L(TAG).info() << "Analyzing " << filePath.toStdString();

    auto reader = m_audioFileReaderFactory ? m_audioFileReaderFactory() : std::make_unique<SndFileReader>();
    AudioFileReader::Info info {};
    if (!reader->open(filePath.toStdString(), AudioFileReader::Mode::Read, info)) {
        const auto message = QCoreApplication::translate("AudioAnalysis", "Failed to open '%1'").arg(QFileInfo { filePath }.fileName());
        juzzlin::L(TAG).error() << message.toStdString();
        emit analysisFailed(side, message);
        return;
    }

    if (!info.samplerate || !info.channels) {
        reader->close();
        const auto message = QCoreApplication::translate("AudioAnalysis", "'%1' holds no audio").arg(QFileInfo { filePath }.fileName());
        juzzlin::L(TAG).error() << message.toStdString();
        emit analysisFailed(side, message);
        return;
    }

    // The same pass the render makes over a file it has just written: one read, both analyzers.
    LoudnessAnalyzer loudness { static_cast<double>(info.samplerate) };
    SpectrumAnalyzer spectrum { static_cast<double>(info.samplerate), info.channels };
    std::vector<float> buffer(16384);
    int64_t read = 0;
    while ((read = reader->readFloat(buffer)) > 0) {
        const auto numSamples = static_cast<size_t>(read * info.channels);
        loudness.process(buffer.data(), numSamples);
        spectrum.process(buffer.data(), numSamples);
    }
    reader->close();

    const AudioAnalysis analysis { loudness.calculate(), spectrum.calculate(), filePath,
                                   static_cast<quint32>(info.samplerate), info.channels, info.frames, true };

    juzzlin::L(TAG).info() << "Analyzed " << filePath.toStdString() << ": "
                           << analysis.loudness.integratedLoudness << " LUFS";

    emit analysisFinished(side, analysis);
}

} // namespace noteahead
