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

#ifndef AUDIO_ANALYSIS_WORKER_HPP
#define AUDIO_ANALYSIS_WORKER_HPP

#include "../../infra/audio/backend/audio_file_reader.hpp"
#include "audio_analysis.hpp"

#include <QObject>

#include <functional>
#include <memory>

namespace noteahead {

//! Measures an audio file's loudness and balance, off the UI thread.
//!
//! Reading and analysing a few minutes of audio takes seconds, which is a frozen window if it
//! happens where the click did. Lives in AudioAnalysisService's thread, exactly as RenderWorker
//! lives in RenderService's.
class AudioAnalysisWorker : public QObject
{
    Q_OBJECT

public:
    using AudioFileReaderFactory = std::function<std::unique_ptr<AudioFileReader>()>;

    explicit AudioAnalysisWorker(QObject * parent = nullptr);

    //! Swaps in another reader, for tests that analyse audio no file holds.
    void setAudioFileReaderFactory(AudioFileReaderFactory factory);

public slots:
    //! Measures @p filePath and answers with one of the signals below. @p side is carried through
    //! untouched so the caller knows which of its two slots the result belongs to.
    void analyze(int side, QString filePath);

signals:
    void analysisFinished(int side, noteahead::AudioAnalysis analysis);
    void analysisFailed(int side, QString message);

private:
    AudioFileReaderFactory m_audioFileReaderFactory;
};

} // namespace noteahead

#endif // AUDIO_ANALYSIS_WORKER_HPP
