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

#ifndef AUDIO_ANALYSIS_SERVICE_HPP
#define AUDIO_ANALYSIS_SERVICE_HPP

#include "audio_analysis.hpp"
#include "audio_analysis_worker.hpp"

#include <QObject>
#include <QStringList>
#include <QThread>

#include <array>
#include <memory>

namespace noteahead {

//! Analyses audio files the user points at, two at a time, so that one mix can be held against
//! another.
//!
//! Two sides rather than a list: the question this answers is "how does this differ from that",
//! which is asked of a pair. Each file's bands come out relative to its own midrange, so the
//! difference between the two sides is a balance difference whatever either was mastered to.
class AudioAnalysisService : public QObject
{
    Q_OBJECT

public:
    //! Which of the two files a call is about. Left is the reference: differences read right minus
    //! left, so a positive number means the right file has more there.
    enum class Side
    {
        Left = 0,
        Right = 1
    };

    explicit AudioAnalysisService(QObject * parent = nullptr);
    ~AudioAnalysisService() override;

    //! Swaps in another reader, for tests that analyse audio no file holds.
    void setAudioFileReaderFactory(AudioAnalysisWorker::AudioFileReaderFactory factory);

    void analyze(Side side, const QString & filePath);
    void clear(Side side);
    void swap();

    const AudioAnalysis & analysis(Side side) const;

    //! True while either side is being measured.
    bool isAnalyzing() const;

    //! True while this side in particular is being measured. The side keeps showing its previous
    //! result meanwhile, so this is what says the reading on screen is about to be replaced.
    bool isAnalyzing(Side side) const;

    //! The comparison as it is written to disk. Empty when neither side holds a result.
    QString reportText() const;

    //! Writes reportText() to @p filePath.
    bool saveReport(const QString & filePath) const;

    //! Audio files measured before, newest first. Kept in the settings rather than in a project: a
    //! reference track is held against every song, not against one.
    QStringList recentFiles() const;

signals:
    //! One side changed: analysed, cleared, or swapped with the other.
    void analysisChanged(int side);
    void recentFilesChanged();
    void isAnalyzingChanged();
    void errorOccurred(int side, QString message);

private:
    void onAnalysisFinished(int side, noteahead::AudioAnalysis analysis);
    void onAnalysisFailed(int side, QString message);

    static size_t index(Side side);

    void rememberFile(const QString & filePath);

    QThread m_workerThread;
    std::unique_ptr<AudioAnalysisWorker> m_worker;

    QStringList m_recentFiles;

    std::array<AudioAnalysis, 2> m_analyses;
    std::array<bool, 2> m_pending { false, false };
};

} // namespace noteahead

#endif // AUDIO_ANALYSIS_SERVICE_HPP
