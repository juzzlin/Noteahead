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

#include "audio_analysis_controller.hpp"

#include "../../application/service/analysis_report_formatter.hpp"
#include "../../application/service/audio_analysis_service.hpp"
#include "../qml/Dialogs/spectrum_compare_renderer.hpp"

#include <QFileInfo>

namespace noteahead {

namespace {

using Side = AudioAnalysisService::Side;

SpectrumCompareRenderer::Bands bandsOf(const AudioAnalysis & analysis)
{
    SpectrumCompareRenderer::Bands bands;
    if (!analysis.isValid || !analysis.spectrum.isValid) {
        return bands;
    }
    bands.reserve(analysis.spectrum.bands.size());
    for (auto && band : analysis.spectrum.bands) {
        bands.emplace_back(band.centerHz, band.levelDb);
    }
    return bands;
}

QString fileNameOf(const AudioAnalysis & analysis)
{
    return analysis.isValid ? QFileInfo { analysis.filePath }.fileName() : QString {};
}

} // namespace

AudioAnalysisController::AudioAnalysisController(AudioAnalysisServiceS audioAnalysisService, QObject * parent)
  : QObject { parent }
  , m_audioAnalysisService { std::move(audioAnalysisService) }
{
    connect(m_audioAnalysisService.get(), &AudioAnalysisService::analysisChanged, this, [this](int) {
        emit analysisChanged();
    });
    connect(m_audioAnalysisService.get(), &AudioAnalysisService::isAnalyzingChanged, this, &AudioAnalysisController::isAnalyzingChanged);
    connect(m_audioAnalysisService.get(), &AudioAnalysisService::recentFilesChanged, this, &AudioAnalysisController::recentFilesChanged);
    connect(m_audioAnalysisService.get(), &AudioAnalysisService::errorOccurred, this, [this](int, QString message) {
        emit errorOccurred(message);
    });
}

void AudioAnalysisController::analyzeLeft(const QUrl & fileUrl)
{
    m_audioAnalysisService->analyze(Side::Left, fileUrl.toLocalFile());
}

void AudioAnalysisController::analyzeRight(const QUrl & fileUrl)
{
    m_audioAnalysisService->analyze(Side::Right, fileUrl.toLocalFile());
}

void AudioAnalysisController::analyzeLeftPath(const QString & filePath)
{
    m_audioAnalysisService->analyze(Side::Left, filePath);
}

void AudioAnalysisController::analyzeRightPath(const QString & filePath)
{
    m_audioAnalysisService->analyze(Side::Right, filePath);
}

QStringList AudioAnalysisController::recentFiles() const
{
    return m_audioAnalysisService->recentFiles();
}

QStringList AudioAnalysisController::recentFileNames() const
{
    QStringList names;
    for (auto && path : m_audioAnalysisService->recentFiles()) {
        names << QFileInfo { path }.fileName();
    }
    return names;
}

void AudioAnalysisController::clearLeft()
{
    m_audioAnalysisService->clear(Side::Left);
}

void AudioAnalysisController::clearRight()
{
    m_audioAnalysisService->clear(Side::Right);
}

void AudioAnalysisController::swap()
{
    m_audioAnalysisService->swap();
}

bool AudioAnalysisController::saveReport(const QUrl & fileUrl)
{
    return m_audioAnalysisService->saveReport(fileUrl.toLocalFile());
}

QString AudioAnalysisController::defaultReportFileName() const
{
    const auto left = QFileInfo { m_audioAnalysisService->analysis(Side::Left).filePath }.completeBaseName();
    const auto right = QFileInfo { m_audioAnalysisService->analysis(Side::Right).filePath }.completeBaseName();
    if (!left.isEmpty() && !right.isEmpty()) {
        return QString { "%1_vs_%2.analysis.txt" }.arg(left, right);
    }
    if (!left.isEmpty() || !right.isEmpty()) {
        return QString { "%1.analysis.txt" }.arg(left.isEmpty() ? right : left);
    }
    return "analysis.txt";
}

void AudioAnalysisController::updateRenderer(QObject * renderer)
{
    if (const auto compareRenderer = qobject_cast<SpectrumCompareRenderer *>(renderer); compareRenderer) {
        compareRenderer->setBands(bandsOf(m_audioAnalysisService->analysis(Side::Left)),
                                  bandsOf(m_audioAnalysisService->analysis(Side::Right)));
        compareRenderer->setNames(leftFileName(), rightFileName());
    }
}

QString AudioAnalysisController::leftFileName() const
{
    return fileNameOf(m_audioAnalysisService->analysis(Side::Left));
}

QString AudioAnalysisController::rightFileName() const
{
    return fileNameOf(m_audioAnalysisService->analysis(Side::Right));
}

QString AudioAnalysisController::leftReport() const
{
    const auto & analysis = m_audioAnalysisService->analysis(Side::Left);
    return analysis.isValid ? AnalysisReportFormatter::html(analysis) : QString {};
}

QString AudioAnalysisController::rightReport() const
{
    const auto & analysis = m_audioAnalysisService->analysis(Side::Right);
    return analysis.isValid ? AnalysisReportFormatter::html(analysis) : QString {};
}

bool AudioAnalysisController::hasLeft() const
{
    return m_audioAnalysisService->analysis(Side::Left).isValid;
}

bool AudioAnalysisController::hasRight() const
{
    return m_audioAnalysisService->analysis(Side::Right).isValid;
}

bool AudioAnalysisController::isAnalyzing() const
{
    return m_audioAnalysisService->isAnalyzing();
}

bool AudioAnalysisController::leftBusy() const
{
    return m_audioAnalysisService->isAnalyzing(Side::Left);
}

bool AudioAnalysisController::rightBusy() const
{
    return m_audioAnalysisService->isAnalyzing(Side::Right);
}

} // namespace noteahead
