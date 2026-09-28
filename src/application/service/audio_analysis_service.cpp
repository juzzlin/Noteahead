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

#include "audio_analysis_service.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../infra/settings.hpp"
#include "analysis_report_formatter.hpp"

#include <QFileInfo>
#include <QMetaObject>

namespace noteahead {

static const auto TAG = "AudioAnalysisService";

AudioAnalysisService::AudioAnalysisService(QObject * parent)
  : QObject { parent }
  , m_worker { std::make_unique<AudioAnalysisWorker>() }
  , m_recentFiles { Settings::recentAnalysisFiles() }
{
    qRegisterMetaType<noteahead::AudioAnalysis>("noteahead::AudioAnalysis");

    m_worker->moveToThread(&m_workerThread);

    connect(m_worker.get(), &AudioAnalysisWorker::analysisFinished, this, &AudioAnalysisService::onAnalysisFinished);
    connect(m_worker.get(), &AudioAnalysisWorker::analysisFailed, this, &AudioAnalysisService::onAnalysisFailed);

    m_workerThread.start();
}

AudioAnalysisService::~AudioAnalysisService()
{
    m_workerThread.quit();
    m_workerThread.wait();
}

void AudioAnalysisService::setAudioFileReaderFactory(AudioAnalysisWorker::AudioFileReaderFactory factory)
{
    m_worker->setAudioFileReaderFactory(std::move(factory));
}

size_t AudioAnalysisService::index(Side side)
{
    return static_cast<size_t>(side);
}

void AudioAnalysisService::analyze(Side side, const QString & filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    juzzlin::L(TAG).info() << "Requesting analysis of " << filePath.toStdString();

    const auto wasAnalyzing = isAnalyzing();
    m_pending.at(index(side)) = true;
    if (!wasAnalyzing) {
        emit isAnalyzingChanged();
    }

    // Queued on purpose: the worker lives in its own thread, and the file is read there.
    QMetaObject::invokeMethod(m_worker.get(), "analyze", Qt::QueuedConnection,
                              Q_ARG(int, static_cast<int>(side)), Q_ARG(QString, filePath));
}

void AudioAnalysisService::clear(Side side)
{
    m_analyses.at(index(side)) = AudioAnalysis {};
    emit analysisChanged(static_cast<int>(side));
}

void AudioAnalysisService::swap()
{
    std::swap(m_analyses.at(index(Side::Left)), m_analyses.at(index(Side::Right)));
    emit analysisChanged(static_cast<int>(Side::Left));
    emit analysisChanged(static_cast<int>(Side::Right));
}

const AudioAnalysis & AudioAnalysisService::analysis(Side side) const
{
    return m_analyses.at(index(side));
}

bool AudioAnalysisService::isAnalyzing() const
{
    return m_pending.at(index(Side::Left)) || m_pending.at(index(Side::Right));
}

QString AudioAnalysisService::reportText() const
{
    const auto & left = analysis(Side::Left);
    const auto & right = analysis(Side::Right);
    if (!left.isValid && !right.isValid) {
        return {};
    }
    return AnalysisReportFormatter::comparisonText(left, right);
}

QStringList AudioAnalysisService::recentFiles() const
{
    return m_recentFiles;
}

void AudioAnalysisService::rememberFile(const QString & filePath)
{
    // Remembered once it has been measured rather than when it was asked for: a path that failed to
    // open is not a file worth offering again.
    const auto absolute = QFileInfo { filePath }.absoluteFilePath();
    m_recentFiles.removeAll(absolute);
    m_recentFiles.push_front(absolute);

    constexpr int maxFileCount = 10;
    while (m_recentFiles.size() > maxFileCount) {
        m_recentFiles.pop_back();
    }

    Settings::setRecentAnalysisFiles(m_recentFiles);
    emit recentFilesChanged();
}

bool AudioAnalysisService::saveReport(const QString & filePath) const
{
    const auto report = reportText();
    if (report.isEmpty()) {
        return false;
    }
    return AnalysisReportFormatter::writeReportFile(filePath, report);
}

void AudioAnalysisService::onAnalysisFinished(int side, noteahead::AudioAnalysis analysis)
{
    rememberFile(analysis.filePath);

    m_analyses.at(index(static_cast<Side>(side))) = std::move(analysis);
    m_pending.at(index(static_cast<Side>(side))) = false;

    emit analysisChanged(side);
    if (!isAnalyzing()) {
        emit isAnalyzingChanged();
    }
}

void AudioAnalysisService::onAnalysisFailed(int side, QString message)
{
    m_analyses.at(index(static_cast<Side>(side))) = AudioAnalysis {};
    m_pending.at(index(static_cast<Side>(side))) = false;

    emit analysisChanged(side);
    emit errorOccurred(side, message);
    if (!isAnalyzing()) {
        emit isAnalyzingChanged();
    }
}

} // namespace noteahead
