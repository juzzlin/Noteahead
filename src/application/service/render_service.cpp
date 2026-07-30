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

#include "render_service.hpp"

#include "../../common/constants.hpp"
#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/tracker/song.hpp"
#include "editor_service.hpp"
#include "mixer_service.hpp"
#include "render_worker.hpp"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

namespace noteahead {

static const auto TAG = "RenderService";

RenderService::RenderService(AudioEngineS audioEngine,
                             DeviceServiceS deviceService,
                             MixerServiceS mixerService,
                             EditorServiceS editorService,
                             AutomationServiceS automationService,
                             SideChainServiceS sideChainService,
                             QObject * parent)
  : QObject { parent }
  , m_audioEngine { std::move(audioEngine) }
  , m_deviceService { std::move(deviceService) }
  , m_mixerService { std::move(mixerService) }
  , m_editorService { std::move(editorService) }
  , m_automationService { std::move(automationService) }
  , m_sideChainService { std::move(sideChainService) }
{
    m_worker = std::make_unique<RenderWorker>(m_audioEngine, m_deviceService, m_mixerService);
    m_worker->moveToThread(&m_workerThread);

    connect(m_worker.get(), &RenderWorker::progressChanged, this, &RenderService::onWorkerProgressChanged);
    connect(m_worker.get(), &RenderWorker::finished, this, &RenderService::onWorkerFinished);

    m_workerThread.start();
}

RenderService::~RenderService()
{
    m_workerThread.quit();
    m_workerThread.wait();
}

void RenderService::renderMaster(const QString & fileName)
{
    if (m_isRendering)
        return;

    juzzlin::L(TAG).info() << "Rendering master to " << fileName.toStdString();

    m_mixerService->pushState();

    m_queue.clear();
    m_queue.push_back({ fileName, {} });
    m_currentJobIndex = 0;
    m_aggregatedReport.clear();
    m_isRendering = true;
    emit isRenderingChanged();

    startNextRender();
}

void RenderService::renderIndividualTracks(const QString & directory)
{
    if (m_isRendering)
        return;

    juzzlin::L(TAG).info() << "Rendering individual tracks to " << directory.toStdString();

    m_mixerService->pushState();

    m_queue.clear();
    for (auto trackIndex : m_editorService->trackIndices()) {
        const auto portName = m_editorService->instrumentPortName(trackIndex);
        if (!m_deviceService->isInternalDevice(portName)) {
            juzzlin::L(TAG).info() << "Skipping track " << trackIndex << " (" << portName.toStdString() << ") because it is not an internal instrument";
            continue;
        }

        const auto trackName = m_editorService->trackName(trackIndex);
        const auto fileName = QDir(directory).filePath(renderFileName(trackName));
        m_queue.push_back({ fileName, { trackIndex } });
    }

    if (m_queue.empty()) {
        juzzlin::L(TAG).info() << "Nothing to render";
        m_mixerService->popState();
        return;
    }

    m_currentJobIndex = 0;
    m_aggregatedReport.clear();
    m_isRendering = true;
    emit isRenderingChanged();

    startNextRender();
}

bool RenderService::isRendering() const
{
    return m_isRendering;
}

double RenderService::progress() const
{
    return m_progress;
}

QString RenderService::renderFileName(const QString & baseName) const
{
    const auto date = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    const auto renderSettings = m_editorService->song() ? m_editorService->song()->metadata().renderSettings() : RenderSettings {};
    const auto sampleRateSuffix = QString::number(renderSettings.sampleRate() / 1000) + "k";
    const auto bitDepthSuffix = [&renderSettings]() {
        switch (static_cast<BitDepth>(renderSettings.bitDepth())) {
        case BitDepth::PCM_24:
            return "24bit";
        case BitDepth::PCM_32:
            return "32bit";
        case BitDepth::Float_32:
            return "32bitFloat";
        case BitDepth::PCM_16:
        default:
            return "16bit";
        }
    }();
    const auto ext = static_cast<AudioFormat>(renderSettings.format()) == AudioFormat::Flac ? ".flac" : ".wav";
    return QString { "%1_%2_%3_%4%5" }.arg(baseName, sampleRateSuffix, bitDepthSuffix, date, ext);
}

QString RenderService::defaultRenderFileName() const
{
    const auto projectFileName = m_editorService->currentFileName();
    if (projectFileName.isEmpty()) {
        return {};
    }
    return renderFileName(projectFileName);
}

QString RenderService::defaultRenderDirectory() const
{
    const auto projectFileName = m_editorService->currentFileName();
    if (projectFileName.isEmpty()) {
        return {};
    }
    return QFileInfo(projectFileName).absolutePath();
}

void RenderService::onWorkerFinished(bool success, QString message)
{
    auto finalize = [this](bool success, QString message) {
        m_mixerService->popState();

        m_isRendering = false;
        m_queue.clear();
        emit isRenderingChanged();
        emit renderingFinished(success, message);
    };

    if (!success) {
        juzzlin::L(TAG).error() << "Worker reported failure: " << message.toStdString();
        finalize(false, message);
        return;
    }

    if (!message.isEmpty()) {
        if (m_queue.size() > 1) {
            const auto baseName = QFileInfo { m_queue[m_currentJobIndex].fileName }.fileName();
            if (!m_aggregatedReport.isEmpty()) {
                m_aggregatedReport += "<br/><br/>";
            }
            m_aggregatedReport += "<b>" + baseName + "</b>:<br/>" + message;
        } else {
            m_aggregatedReport = message;
        }
    }

    m_currentJobIndex++;
    if (m_currentJobIndex < m_queue.size()) {
        startNextRender();
    } else {
        juzzlin::L(TAG).info() << "All render jobs completed successfully";
        finalize(true, m_aggregatedReport);
    }
}

void RenderService::onWorkerProgressChanged(double progress)
{
    m_progress = (static_cast<double>(m_currentJobIndex) + progress) / static_cast<double>(m_queue.size());
    emit progressChanged();
}

void RenderService::startNextRender()
{
    const auto & job = m_queue[m_currentJobIndex];

    juzzlin::L(TAG).info() << "Starting job " << m_currentJobIndex + 1 << "/" << m_queue.size() << ": " << job.fileName.toStdString();

    m_mixerService->blockSignals(true);
    // Setup mixer for solo if needed
    if (!job.soloTracks.empty()) {
        juzzlin::L(TAG).info() << "Setting up solo tracks for individual render";
        for (auto trackIndex : m_editorService->trackIndices()) {
            bool shouldSolo = std::find(job.soloTracks.begin(), job.soloTracks.end(), trackIndex) != job.soloTracks.end();
            m_mixerService->soloTrack(trackIndex, shouldSolo);
            if (shouldSolo) {
                m_mixerService->muteTrack(trackIndex, false);
            }
        }
    }
    m_mixerService->blockSignals(false);

    const auto song = m_editorService->song();
    RenderWorker::Timing timing;
    timing.beatsPerMinute = m_editorService->beatsPerMinute();
    timing.linesPerBeat = m_editorService->linesPerBeat();
    timing.ticksPerLine = m_editorService->ticksPerLine();

    const auto events = song->renderToEvents(m_automationService, m_sideChainService, 0);
    const auto maxTick = song->totalTicks();
    // Render settings belong to the song, so exporting one song cannot inherit another's trim.
    const auto & renderSettings = song->metadata().renderSettings();
    const auto sampleRate = renderSettings.sampleRate();
    const auto bitDepth = static_cast<BitDepth>(renderSettings.bitDepth());
    const auto format = static_cast<AudioFormat>(renderSettings.format());
    const auto normalize = renderSettings.normalizeEnabled();
    const auto normalizeTargetDb = static_cast<double>(renderSettings.normalizeLevelTenthsDb()) / 10.0;
    const auto trim = renderSettings.trimEnabled();
    const auto trimMinutes = renderSettings.trimMinutes();
    const auto trimSeconds = renderSettings.trimSeconds();
    const auto analyze = renderSettings.analyzeEnabled();
    const auto oversampleFactor = static_cast<quint8>(renderSettings.oversampleFactor());

    const std::map<AudioFileReader::TagType, std::string> tags = [&song]() {
        std::map<AudioFileReader::TagType, std::string> t;
        for (const auto & [key, value] : song->metadata().tags()) {
            if (key == Constants::NahdXml::xmlKeyTitle().toStdString())
                t[AudioFileReader::TagType::Title] = value;
            else if (key == Constants::NahdXml::xmlKeyArtist().toStdString())
                t[AudioFileReader::TagType::Artist] = value;
            else if (key == Constants::NahdXml::xmlKeyAlbum().toStdString())
                t[AudioFileReader::TagType::Album] = value;
            else if (key == Constants::NahdXml::xmlKeyDate().toStdString())
                t[AudioFileReader::TagType::Date] = value;
            else if (key == Constants::NahdXml::xmlKeyComment().toStdString())
                t[AudioFileReader::TagType::Comment] = value;
            else if (key == Constants::NahdXml::xmlKeyGenre().toStdString())
                t[AudioFileReader::TagType::Genre] = value;
            else if (key == Constants::NahdXml::xmlKeyTrackNumber().toStdString())
                t[AudioFileReader::TagType::TrackNumber] = value;
        }
        return t;
    }();

    juzzlin::L(TAG).info() << "Invoking RenderWorker::render... events=" << events.size() << " maxTick=" << maxTick << " sampleRate=" << sampleRate << " bitDepth=" << static_cast<int>(bitDepth);

    const auto renderWorker = m_worker.get();
    bool success = QMetaObject::invokeMethod(renderWorker, [renderWorker, fileName = job.fileName, events, timing, maxTick, sampleRate, bitDepth, normalize, normalizeTargetDb, trim, trimMinutes, trimSeconds, analyze, oversampleFactor, format, tags]() { renderWorker->render(fileName, events, timing, maxTick, sampleRate, bitDepth, normalize, normalizeTargetDb, trim, trimMinutes, trimSeconds, analyze, oversampleFactor, format, tags); }, Qt::QueuedConnection);
    if (!success) {
        juzzlin::L(TAG).error() << "Failed to invoke RenderWorker::render!";
        onWorkerFinished(false, "Internal error: Failed to start render worker.");
    }
}

} // namespace noteahead
