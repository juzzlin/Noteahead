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

#ifndef AUDIO_ENGINE_HPP
#define AUDIO_ENGINE_HPP

#include "../../domain/devices/device.hpp"
#include "../../domain/devices/lfo_device.hpp"
#include "../../domain/dsp/metronome.hpp"
#include "../../domain/tracker/parameter_automation.hpp"

#include <atomic>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "../../domain/utility/load_meter.hpp"

#include <span>

namespace noteahead {

class EffectRack;
class ParameterAutomationTarget;
class RealTimeWorkerPool;

struct AudioEngineWorkBuffer
{
    std::vector<double> deviceBuffer {};
    //! Holds the signal as it stood before the fader, for devices whose sends tap pre-fader.
    std::vector<double> preFaderBuffer {};
    std::vector<double> outputBuffer {};
    std::vector<std::vector<double>> sendBuffers {};
    //! The same buffers as spans, so a device can be handed them without anything allocating on the
    //! audio thread. Rebuilt only when sendBuffers is resized.
    std::vector<std::span<double>> sendBufferSpans {};
    //! Scratch for the insert rack's side chain sources, so asking for them on the audio thread
    //! allocates nothing once it has grown to size.
    std::vector<size_t> sidechainSources {};
};

class AudioEngine
{
public:
    using DeviceS = std::shared_ptr<Device>;

    AudioEngine();
    ~AudioEngine();

    void setDevice(size_t slotIndex, DeviceS device);
    void clearDevice(size_t slotIndex);
    DeviceS device(size_t slotIndex) const;
    DeviceS device(const std::string & name) const;

    using DeviceNames = std::vector<std::string>;
    DeviceNames deviceNames() const;

    void setBpm(float bpm);
    float bpm() const;

    //! The click, which anything that wants one switches on. Rendered after the master rack, so
    //! nothing shapes it, and never while rendering offline.
    Metronome & metronome();
    const Metronome & metronome() const;

    void process(AudioContext & context);

    //! Frames the engine has rendered since it was created, which is the timeline a scheduled event
    //! names its frame on.
    //!
    //! Free-running and never reset: whoever schedules against it anchors on what it reads when
    //! playback starts, so all it has to be is monotonic. Read off the audio thread, so atomic.
    uint64_t framesRendered() const;

    //! What frame the engine was on and when, so that a time can be turned into a frame.
    //!
    //! The player knows when a tick is due but not where that falls in the stream. Published once
    //! per block from the audio thread and read from the player's, torn reads guarded by the
    //! sequence number rather than by a lock the audio thread would have to take.
    struct FrameAnchor
    {
        uint64_t frame {};
        //! steady_clock nanoseconds at which that frame's block was handed to the devices.
        int64_t nanoseconds {};
        //! Frames per second the stream is actually running at.
        uint32_t sampleRate {};
        //! Frames in that block, i.e. the buffer size the backend is actually handing over.
        uint32_t blockFrames {};
        //! Longest recent gap between two blocks, which is how far apart the engine's chances to
        //! start a note are. A backend that hands over a burst of buffers and then sleeps has a gap
        //! far larger than one block, and that is what anything scheduling has to stay ahead of.
        int64_t maxBlockGapNanoseconds {};
        //! Whether the engine is rendering at all. A stalled stream cannot place anything, and
        //! whoever is scheduling has to fall back to the clock on the wall.
        bool running {};
    };

    FrameAnchor frameAnchor() const;

    //! A value a parameter automation wants an effect or a device at.
    struct ParameterChange
    {
        std::weak_ptr<ParameterAutomationTarget> target;
        std::string parameter;
        ParameterAutomation::Mode mode = ParameterAutomation::Mode::Absolute;
        double value = 0.0;
        //! Frame on the stream timeline it is meant for. Nothing means as soon as possible, which is
        //! what an offline render wants: it hands the engine each event just before the block that
        //! starts on it.
        std::optional<uint64_t> frame;
    };

    //! Queues a change for the audio thread, which applies it at the start of the block that holds
    //! its frame. Called off the audio thread.
    //!
    //! Applied there rather than by whoever calls this, because the target is in use on the audio
    //! thread the whole time a song plays: writing its parameters from anywhere else would race the
    //! very code reading them. A block's worth early at worst, which a parameter sweep cannot be
    //! heard to be.
    void scheduleParameterChange(ParameterChange change);

    //! Drops whatever is still queued and puts everything the queue has touched back to its
    //! authored values, so that nothing a song played is left behind when it stops.
    void clearParameterAutomation();

    void reset();
    void clear();

    void setIsExclusive(bool exclusive);

    //! Opt-in fan-out for real-time playback. Honoured only when the worker pool actually got
    //! real-time scheduling; without that, threading playback is a priority inversion waiting to
    //! happen and the engine stays serial.
    void setPlaybackThreadingEnabled(bool enabled);
    bool playbackThreadingEnabled() const;
    //! Whether threaded playback can be used at all on this system.
    bool supportsPlaybackThreading() const;

    //! Told by a backend that knows its callback thread's real-time priority up front — JACK does,
    //! through jack_client_real_time_priority(). Without this the engine only learns the priority
    //! from inside the callback, too late to have sized the workers against it.
    void setCallbackRealTimePriority(int priority);

    //! Allocates the engine's working buffers ahead of time for the given block size.
    //!
    //! Without this the first audio callback does every allocation and first touch itself, which is
    //! heard as a stutter on the first play. Call it from a normal thread whenever the block size is
    //! known; it is cheap and idempotent.
    void prepare(uint32_t frameCount);

private:
    void detectCallbackScheduling();
    bool callbackIsRealTime() const;
    void applyWorkerPriority();

public:
    bool isExclusive() const;

    //! Whole-callback DSP load, in percent of the real-time budget, plus an overrun count.
    LoadMeter & loadMeter();
    const LoadMeter & loadMeter() const;

    //! Oversampling factor used for realtime playback (offline export sets its own factor directly on
    //! the AudioContext). Read by the playback context builders and stamped onto each device's context.
    void setPlaybackOversampleFactor(uint8_t factor);
    uint8_t playbackOversampleFactor() const;

    //! Lets an offline render spread its devices over the worker threads. Off by default, because
    //! the lane a device lands in decides how its output is grouped into the sum and float addition
    //! is not associative: the same song then renders to a file that differs in the last bits from
    //! one run to the next. Real-time playback is unaffected -- it always fans out when it can.
    void setFastRender(bool enabled);
    bool fastRender() const;

    EffectRack & sendEffectRack();
    EffectRack & insertEffectRack();

    //! The chain of effects a send bus runs after its own effect, so that what a send returns can be
    //! shaped further: an EQ on a reverb, a compressor on a delay.
    //!
    //! A bus's effect list is its effect in sendEffectRack() followed by this rack. The first
    //! enabled effect of that list runs in send mode, i.e. only what it adds is returned to the
    //! master; every later one runs as an insert on that return. See processEffectTask().
    EffectRack & sendChainRack(size_t busIndex);
    const EffectRack & sendChainRack(size_t busIndex) const;
    size_t sendChainRackCount() const;

private:
    //! Applies every queued parameter change whose frame falls before @p blockEndFrame.
    void applyParameterChanges(uint64_t blockEndFrame);

    //! Asks every LFO device for this block's modulation steps and hands them on: a device target
    //! takes each at its own frame as it renders, an effect target the block's last at its start.
    //! Single-threaded and before anything renders, so the order is fixed and nothing races a lane.
    void evaluateModulation(const AudioContext & context);
    //! The device or effect a modulation names, from the devices and racks this engine holds, cached
    //! so that a running LFO looks it up once. Not through the services: those reach back in here.
    std::shared_ptr<ParameterAutomationTarget> resolveModulationTarget(const ParameterAutomation::Target & target);
    //! Takes every modulation @p device put on anything back off. For a device leaving the rack.
    void releaseModulationsOf(const Device * device);

    void ensureWorkBuffers(size_t laneCount, size_t sendCount, uint32_t bufferSize);
    void ensureEffectWetBuffers(size_t effectCount, uint32_t bufferSize);
    void ensureEffectActiveFlags(size_t effectCount);
    void ensureDeviceActiveFlags(size_t deviceCount);
    void ensureDeviceOutputBuffers(uint32_t bufferSize);

    void rebuildProcessingGraph();
    //! Marks which devices still feed the master directly. A device claimed as a SubMixer member
    //! is heard through that SubMixer instead, so its direct contribution has to be suppressed.
    void updateDirectOutSnapshot();
    //! Builds a signature of the current graph inputs (device slots + their sidechain deps) into a
    //! reusable buffer and reports whether it changed since the last rebuild. Allocation-free in
    //! steady state so it is safe to call every audio callback.
    bool processingGraphChanged();

    //! Refreshes the cached snapshot of every send chain whose rack has changed. Called under
    //! m_mutex at the top of process(), so the audio work below never touches a rack's own lock.
    void refreshSendChainSnapshots();

    std::map<size_t, DeviceS> m_devices;
    std::unique_ptr<EffectRack> m_sendEffectRack;
    std::unique_ptr<EffectRack> m_insertEffectRack;
    //! One chain per send bus, indexed like m_sendEffectRack's slots. Always sized to the send rack,
    //! so a bus always has a chain to address even while it is empty.
    std::vector<std::unique_ptr<EffectRack>> m_sendChainRacks;

    // Cached snapshot of the send effect rack, refreshed only when the rack changes so that process()
    // does not copy the effect vector (and bump shared_ptr refcounts) on every audio callback.
    std::vector<std::shared_ptr<Effect>> m_sendEffectsSnapshot;
    uint64_t m_sendEffectsVersion = std::numeric_limits<uint64_t>::max();

    // The same treatment for the chains, one snapshot per bus. A chain is read on the audio thread
    // once per block and EffectRack guards itself with a mutex, so going through the rack there
    // would put a lock on the send path -- see the jitter notes in DeviceService.
    //
    // Null slots are dropped but disabled effects are kept: Effect's enabled flag is live and does
    // not bump the rack's version, so filtering on it here would freeze whatever it read last.
    // Effect::process() honours the flag itself, and the send-mode rule reads it per block.
    std::vector<std::vector<std::shared_ptr<Effect>>> m_sendChainSnapshots;
    std::vector<uint64_t> m_sendChainVersions;
    //! Last seen bypass state per chain. Bypassing a rack leaves its contents alone and so does not
    //! bump its version, so the snapshot above would not otherwise be refreshed for it.
    std::vector<uint8_t> m_sendChainEnabled;
    std::unique_ptr<RealTimeWorkerPool> m_workerPool;
    std::vector<AudioEngineWorkBuffer> m_workBuffers;
    std::vector<DeviceS> m_deviceSnapshot;
    std::vector<size_t> m_deviceSlotSnapshot;
    std::vector<uint8_t> m_deviceActiveFlags;
    std::vector<double> m_deviceSendSnapshot;
    std::vector<uint8_t> m_deviceDirectOutSnapshot;
    std::vector<std::vector<double>> m_sendBusBuffers;
    std::vector<std::vector<double>> m_effectWetBuffers;
    std::vector<uint8_t> m_effectActiveFlags;
    //! Whether each send bus carries anything this block. Decided once here rather than inside the
    //! per-effect task, because the dispatch has to know how much work there is before handing it out.
    std::vector<uint8_t> m_sendBusHasSignal;
    std::vector<std::vector<double>> m_deviceOutputBuffers;
    std::vector<std::span<const double>> m_deviceOutputBufferSpans;
    std::vector<std::vector<size_t>> m_processingLayers;
    std::vector<size_t> m_scratchDeps;
    std::vector<size_t> m_graphSignature;
    std::vector<size_t> m_prevGraphSignature;
    mutable std::mutex m_mutex;
    //! Separate from m_mutex so that queueing a change never waits for a block to finish rendering.
    std::mutex m_parameterChangesMutex;
    std::vector<ParameterChange> m_parameterChanges;
    //! Everything a change has been applied to since the last clear, so that a stop can put it back
    //! without walking every rack and device in the project.
    std::vector<std::weak_ptr<ParameterAutomationTarget>> m_automatedTargets;

    //! This block's steps from every LFO device, alive until the next block: the devices' queues
    //! point into it rather than copy the keys.
    std::vector<LfoDevice::Step> m_modulationSteps;
    //! Where each LFO device's steps start in m_modulationSteps.
    std::vector<std::pair<const LfoDevice *, size_t>> m_modulationSources;

    struct CachedModulationTarget
    {
        ParameterAutomation::Target target;
        std::weak_ptr<ParameterAutomationTarget> resolved;
    };

    std::vector<CachedModulationTarget> m_modulationTargetCache;

    //! What each LFO is modulating, so that a device leaving the rack can let go of it.
    struct ActiveModulation
    {
        const LfoDevice * device {};
        size_t lfo {};
        ParameterAutomation::Target target;
    };

    std::vector<ActiveModulation> m_activeModulations;
    //! The latest step per LFO for an effect target, by index into m_modulationSteps.
    std::vector<size_t> m_effectModulationSteps;
    std::atomic<uint64_t> m_framesRendered { 0 };
    //! Even means settled, odd means being written. The audio thread bumps it either side of the
    //! anchor, so a reader that sees the same even value twice knows it read one whole anchor.
    std::atomic<uint32_t> m_frameAnchorSequence { 0 };
    FrameAnchor m_frameAnchor;
    int64_t m_previousBlockNanoseconds { 0 };
    int64_t m_maxBlockGapNanoseconds { 0 };
    //! The song tempo, as the last setBpm() left it. Kept here so that every block can be stamped
    //! with it: AudioContext carries a tempo field that no backend ever filled in, so everything
    //! reading it -- a device's sequencer, a rack on a sampler voice, the effects below -- was
    //! running at the field's default of 120 whatever the song said.
    std::atomic<float> m_bpm { 120.0f };

    std::atomic<bool> m_isExclusive { false };
    std::atomic<bool> m_playbackThreadingEnabled { false };
    //! Scheduling of the thread that drives playback, sampled once from process(). Threading
    //! playback is only safe when that thread is itself real-time: workers above a non-real-time
    //! callback preempt the very thread waiting on them, which is heard as stuttering.
    std::atomic<int> m_callbackPolicy { -1 };
    std::atomic<int> m_callbackPriority { 0 };
    LoadMeter m_loadMeter;
    Metronome m_metronome;
    std::atomic<uint8_t> m_playbackOversampleFactor { 2 };
    std::atomic<bool> m_fastRender { false };
};

} // namespace noteahead

#endif // AUDIO_ENGINE_HPP
