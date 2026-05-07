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

#ifndef RENDER_SERVICE_HPP
#define RENDER_SERVICE_HPP

#include "device_service.hpp"

#include <memory>

#include <QObject>
#include <QString>
#include <QThread>

namespace noteahead {

class AudioEngine;
class AutomationService;
class SideChainService;
class MixerService;
class EditorService;
class RenderWorker;

class RenderService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRendering READ isRendering NOTIFY isRenderingChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString defaultRenderFileName READ defaultRenderFileName CONSTANT)
    Q_PROPERTY(QString defaultRenderDirectory READ defaultRenderDirectory CONSTANT)

public:
    using AudioEngineS = std::shared_ptr<AudioEngine>;
    using DeviceServiceS = std::shared_ptr<DeviceService>;
    using MixerServiceS = std::shared_ptr<MixerService>;
    using EditorServiceS = std::shared_ptr<EditorService>;
    using AutomationServiceS = std::shared_ptr<AutomationService>;
    using SideChainServiceS = std::shared_ptr<SideChainService>;

    RenderService(AudioEngineS audioEngine,
                  DeviceServiceS deviceService,
                  MixerServiceS mixerService,
                  EditorServiceS editorService,
                  AutomationServiceS automationService,
                  SideChainServiceS sideChainService,
                  QObject * parent = nullptr);
    ~RenderService() override;

    Q_INVOKABLE void renderMaster(const QString & fileName);
    Q_INVOKABLE void renderIndividualTracks(const QString & directory);
    Q_INVOKABLE bool isRendering() const;
    Q_INVOKABLE double progress() const;

    QString defaultRenderFileName() const;
    QString defaultRenderDirectory() const;

signals:
    void isRenderingChanged();
    void progressChanged();
    void renderingFinished(bool success, QString message);

private slots:
    void onWorkerFinished(bool success, QString message);
    void onWorkerProgressChanged(double progress);

private:
    void startNextRender();

    AudioEngineS m_audioEngine;
    DeviceServiceS m_deviceService;
    MixerServiceS m_mixerService;
    EditorServiceS m_editorService;
    AutomationServiceS m_automationService;
    SideChainServiceS m_sideChainService;

    std::unique_ptr<RenderWorker> m_worker;
    QThread m_workerThread;

    bool m_isRendering = false;
    double m_progress = 0.0;

    struct RenderJob
    {
        QString fileName;
        std::vector<quint64> soloTracks;
    };
    std::vector<RenderJob> m_queue;
    size_t m_currentJobIndex = 0;
};

} // namespace noteahead

#endif // RENDER_SERVICE_HPP
