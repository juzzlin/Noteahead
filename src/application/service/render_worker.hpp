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
#include "../../domain/song.hpp"

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include <QObject>

namespace noteahead {

class AudioEngine;
class DeviceService;
class MixerService;
class AudioFileReader;

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
    void render(const QString & fileName, const noteahead::RenderWorker::EventList & events, const noteahead::RenderWorker::Timing & timing, quint64 maxTick, quint32 sampleRate, noteahead::BitDepth bitDepth = BitDepth::PCM_16);

signals:
    void progressChanged(double progress);
    void finished(bool success, QString message);

private:
    void handleEvent(const Event & event);

    AudioEngineS m_audioEngine;
    DeviceServiceS m_deviceService;
    MixerServiceS m_mixerService;

    AudioFileReaderFactory m_audioFileReaderFactory;

    bool m_isRendering = false;
};

} // namespace noteahead

Q_DECLARE_METATYPE(noteahead::RenderWorker::Timing)
Q_DECLARE_METATYPE(noteahead::RenderWorker::EventList)
Q_DECLARE_METATYPE(noteahead::BitDepth)

#endif // RENDER_WORKER_HPP
