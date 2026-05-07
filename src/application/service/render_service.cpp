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
#include "../../domain/song.hpp"
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
    if (m_isRendering) return;

    juzzlin::L(TAG).info() << "Rendering master to " << fileName.toStdString();

    m_queue.clear();
    m_queue.push_back({ fileName, {} });
    m_currentJobIndex = 0;
    m_isRendering = true;
    emit isRenderingChanged();

    startNextRender();
}

void RenderService::renderIndividualTracks(const QString & directory)
{
    if (m_isRendering) return;

    juzzlin::L(TAG).info() << "Rendering individual tracks to " << directory.toStdString();

    m_queue.clear();
    for (auto trackIndex : m_editorService->trackIndices()) {
        const auto trackName = m_editorService->trackName(trackIndex);
        const auto fileName = QDir(directory).filePath(trackName + ".wav");
        m_queue.push_back({ fileName, { trackIndex } });
    }

    m_currentJobIndex = 0;
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

QString RenderService::defaultRenderFileName() const
{
    const auto projectFileName = m_editorService->currentFileName();
    if (projectFileName.isEmpty()) {
        return {};
    }
    const auto date = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return projectFileName + "_" + date + ".wav";
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
    if (!success) {
        juzzlin::L(TAG).error() << "Worker reported failure: " << message.toStdString();
        m_isRendering = false;
        m_queue.clear();
        emit isRenderingChanged();
        emit renderingFinished(false, message);
        return;
    }

    m_currentJobIndex++;
    if (m_currentJobIndex < m_queue.size()) {
        startNextRender();
    } else {
        juzzlin::L(TAG).info() << "All render jobs completed successfully";
        m_isRendering = false;
        m_queue.clear();
        emit isRenderingChanged();
        emit renderingFinished(true, "");
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

    // Setup mixer for solo if needed
    if (!job.soloTracks.empty()) {
        juzzlin::L(TAG).info() << "Setting up solo tracks for individual render";
        for (auto trackIndex : m_editorService->trackIndices()) {
            bool shouldSolo = std::find(job.soloTracks.begin(), job.soloTracks.end(), trackIndex) != job.soloTracks.end();
            m_mixerService->soloTrack(trackIndex, shouldSolo);
        }
    }

    const auto song = m_editorService->song();
    RenderWorker::Timing timing;
    timing.beatsPerMinute = m_editorService->beatsPerMinute();
    timing.linesPerBeat = m_editorService->linesPerBeat();
    timing.ticksPerLine = m_editorService->ticksPerLine();

    const auto events = song->renderToEvents(m_automationService, m_sideChainService, 0);
    const auto maxTick = song->totalTicks();
    const quint32 sampleRate = static_cast<quint32>(Constants::defaultSampleRate());

    juzzlin::L(TAG).info() << "Invoking RenderWorker::render... events=" << events.size() << " maxTick=" << maxTick;

    bool success = QMetaObject::invokeMethod(m_worker.get(), "render",
                              Qt::QueuedConnection,
                              Q_ARG(QString, job.fileName),
                              Q_ARG(noteahead::RenderWorker::EventList, events),
                              Q_ARG(noteahead::RenderWorker::Timing, timing),
                              Q_ARG(quint64, maxTick),
                              Q_ARG(quint32, sampleRate));
    if (!success) {
        juzzlin::L(TAG).error() << "Failed to invoke RenderWorker::render!";
        onWorkerFinished(false, "Internal error: Failed to start render worker.");
    }
}

} // namespace noteahead
