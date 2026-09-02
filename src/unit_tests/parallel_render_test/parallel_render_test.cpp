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

#include "parallel_render_test.hpp"

#include "../../domain/devices/drum_synth_device.hpp"
#include "../../domain/devices/string_ensemble_device.hpp"
#include "../../domain/devices/sub_mixer_device.hpp"
#include "../../domain/devices/synth_device.hpp"
#include "../../domain/devices/wavetable_synth_device.hpp"
#include "../../domain/devices/device_factory.hpp"
#include "../../domain/effects/effect_factory.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/effects/reverb.hpp"
#include "../../infra/audio/audio_engine.hpp"

#include "../../application/service/automation_service.hpp"
#include "../../infra/data_service.hpp"
#include "../../application/service/device_service.hpp"
#include "../../application/service/editor_service.hpp"
#include "../../application/service/mixer_service.hpp"
#include "../../application/service/property_service.hpp"
#include "../../application/service/selection_service.hpp"
#include "../../application/service/settings_service.hpp"
#include "../../application/service/side_chain_service.hpp"
#include "../../domain/midi/midi_cc_data.hpp"
#include "../../domain/midi/pitch_bend_data.hpp"
#include "../../domain/tracker/event.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../domain/tracker/note_data.hpp"
#include "../../domain/tracker/song.hpp"

#include <QTest>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <map>
#include <memory>
#include <numeric>
#include <span>
#include <vector>

namespace noteahead {

namespace {

constexpr uint32_t SampleRate { 48000 };
constexpr uint32_t FrameCount { 128 };
constexpr int BufferCount { 60 };

//! Threading changes which lane a device's output is accumulated into, and the lanes are summed in
//! a fixed order afterwards, so the additions are grouped differently. That shifts the last bits of
//! the result — inaudible at around -180 dBFS, but it means the two paths are equal to within
//! floating-point summation order rather than bit-identical.
constexpr double Tolerance { 1.0e-9 };

//! Populates an engine identically every time, so two renders differ only in how they were driven.
//! The devices seed their own generators deterministically, so this is reproducible.
void populate(AudioEngine & engine, bool withSubMixer)
{
    const auto synth = std::make_shared<SynthDevice>("Synth");
    synth->processMidiNoteOn(48, 100);
    synth->processMidiNoteOn(55, 90);
    engine.setDevice(0, synth);

    const auto wavetable = std::make_shared<WavetableSynthDevice>("Wavetable");
    wavetable->processMidiNoteOn(60, 110);
    engine.setDevice(1, wavetable);

    const auto drums = std::make_shared<DrumSynthDevice>("Drums");
    drums->processMidiNoteOn(36, 127);
    drums->processMidiNoteOn(42, 100);
    engine.setDevice(2, drums);

    const auto strings = std::make_shared<StringEnsembleDevice>("Strings");
    strings->processMidiNoteOn(64, 96);
    engine.setDevice(3, strings);

    if (withSubMixer) {
        // A Sub Mixer depends on its members, which puts it in a second processing layer and so
        // exercises the barrier between layers as well as the fan-out within one.
        const auto subMixer = std::make_shared<SubMixerDevice>("SubMixer");
        subMixer->setMembers({ 0, 1 });
        engine.setDevice(4, subMixer);

        // A send bus, so the parallel send accumulation is covered too.
        strings->setReverbSend(0, 0.5f);
        drums->setReverbSend(0, 0.3f);
    }
}

std::vector<double> render(bool threaded, bool withSubMixer)
{
    AudioEngine engine;
    if (withSubMixer) {
        engine.sendEffectRack().setEffect(0, std::make_shared<Reverb>());
    }
    populate(engine, withSubMixer);
    engine.setIsExclusive(threaded);

    std::vector<double> collected;
    collected.reserve(static_cast<size_t>(BufferCount) * FrameCount * 2);
    std::vector<double> buffer(static_cast<size_t>(FrameCount) * 2, 0.0);
    for (int i = 0; i < BufferCount; i++) {
        std::fill(buffer.begin(), buffer.end(), 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), FrameCount, SampleRate };
        engine.process(context);
        collected.insert(collected.end(), buffer.begin(), buffer.end());
    }
    return collected;
}

double peakLevel(const std::vector<double> & samples)
{
    double peak = 0.0;
    for (const auto sample : samples) {
        peak = std::max(peak, std::abs(sample));
    }
    return peak;
}

void compare(const std::vector<double> & a, const std::vector<double> & b)
{
    QCOMPARE(a.size(), b.size());
    QVERIFY2(peakLevel(a) > 0.001, "The fixture produced no signal, so this would prove nothing");

    double worst = 0.0;
    for (size_t i = 0; i < a.size(); i++) {
        worst = std::max(worst, std::abs(a[i] - b[i]));
    }
    QVERIFY2(worst < Tolerance, qPrintable(QString { "worst difference %1" }.arg(worst)));
}

} // namespace

void ParallelRenderTest::test_realDevices_serialAndThreaded_shouldMatch()
{
    // Unlike a fixture of devices all emitting the same constant, real devices produce values whose
    // summation order actually matters, so this can see a fan-out that mixes work up.
    compare(render(false, false), render(true, false));
}

void ParallelRenderTest::test_subMixerAndSends_serialAndThreaded_shouldMatch()
{
    compare(render(false, true), render(true, true));
}

void ParallelRenderTest::test_threadedRender_repeated_shouldBeDeterministic()
{
    // Two threaded renders of the same project must agree, or an export would differ run to run.
    compare(render(true, true), render(true, true));
}

void ParallelRenderTest::test_songLoad_realTimeBuffers_shouldReportTiming()
{
    // Not a regression test: a measurement harness for "why is this song heavy live". Point it at a
    // project with NOTEAHEAD_LOAD_PROBE_SONG and it drives the engine exactly as real-time playback
    // does -- same buffer size, same single-threaded path -- timing every process() call.
    const auto songPath = qgetenv("NOTEAHEAD_LOAD_PROBE_SONG");
    if (songPath.isEmpty()) {
        QSKIP("Set NOTEAHEAD_LOAD_PROBE_SONG to a .nahd path to run the load probe.");
    }

    const uint32_t sampleRate = static_cast<uint32_t>(qEnvironmentVariableIntValue("NOTEAHEAD_LOAD_PROBE_RATE") ? qEnvironmentVariableIntValue("NOTEAHEAD_LOAD_PROBE_RATE") : 48000);
    const uint32_t frames = static_cast<uint32_t>(qEnvironmentVariableIntValue("NOTEAHEAD_LOAD_PROBE_FRAMES") ? qEnvironmentVariableIntValue("NOTEAHEAD_LOAD_PROBE_FRAMES") : 512);
    const double seconds = qEnvironmentVariableIntValue("NOTEAHEAD_LOAD_PROBE_SECONDS") ? qEnvironmentVariableIntValue("NOTEAHEAD_LOAD_PROBE_SECONDS") : 60;

    EffectFactory::init();
    DeviceFactory::init();

    const auto audioEngine = std::make_shared<AudioEngine>();
    const auto dataService = std::make_shared<DataService>();
    const auto deviceService = std::make_shared<DeviceService>(audioEngine, dataService);
    const auto propertyService = std::make_shared<PropertyService>();
    const auto automationService = std::make_shared<AutomationService>(propertyService);
    const auto sideChainService = std::make_shared<SideChainService>();
    const auto mixerService = std::make_shared<MixerService>();
    EditorService editorService { std::make_shared<SelectionService>(), std::make_shared<SettingsService>(), automationService, dataService };

    connect(&editorService, &EditorService::projectPathChanged, deviceService.get(), &DeviceService::setProjectPath);
    connect(&editorService, &EditorService::devicesDeserializationRequested, deviceService.get(), &DeviceService::deserializeFromXml);
    connect(&editorService, &EditorService::automationDeserializationRequested, automationService.get(), &AutomationService::deserializeFromXml);
    connect(&editorService, &EditorService::mixerDeserializationRequested, mixerService.get(), &MixerService::deserializeFromXml);
    connect(&editorService, &EditorService::sideChainDeserializationRequested, sideChainService.get(), &SideChainService::deserializeFromXml);

    editorService.load(QString::fromUtf8(songPath));
    const auto song = editorService.song();
    QVERIFY(song);

    const auto events = song->renderToEvents(automationService, sideChainService, 0);
    std::map<quint64, std::vector<Song::EventS>> eventMap;
    for (auto && event : events) {
        eventMap[event->tick()].push_back(event);
    }

    // Real-time playback runs the engine single-threaded in the callback unless the user turned
    // threading on, which is the case being investigated.
    audioEngine->setIsExclusive(false);
    audioEngine->setPlaybackThreadingEnabled(false);
    audioEngine->setPlaybackOversampleFactor(1);
    audioEngine->prepare(frames);

    const auto handleEvent = [&](const Event & event) {
        event.visit([&](auto && data) {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T, NoteData>) {
                if (auto && instrument = event.instrument(); instrument) {
                    if (const auto portName = instrument->midiAddress().portName(); deviceService->isInternalDevice(portName)) {
                        if (data.type() == NoteData::Type::NoteOff) {
                            deviceService->processMidiNoteOff(portName, *data.note());
                        } else if (data.type() == NoteData::Type::NoteOn && data.note().has_value()) {
                            if (mixerService->shouldColumnPlay(data.track(), data.column())) {
                                deviceService->processMidiNoteOn(portName, *data.note(), mixerService->effectiveVelocity(data.track(), data.column(), data.velocity()));
                            }
                        }
                    }
                }
            } else if constexpr (std::is_same_v<T, MidiCcData>) {
                if (auto && instrument = event.instrument(); instrument) {
                    if (const auto portName = instrument->midiAddress().portName(); deviceService->isInternalDevice(portName)) {
                        deviceService->processMidiCc(portName, data.controller(), data.value(), instrument->midiAddress().channel());
                    }
                }
            } else if constexpr (std::is_same_v<T, PitchBendData>) {
                if (auto && instrument = event.instrument(); instrument) {
                    if (const auto portName = instrument->midiAddress().portName(); deviceService->isInternalDevice(portName)) {
                        deviceService->processMidiPitchBend(portName, static_cast<uint16_t>((static_cast<uint16_t>(data.msb()) << 7) | data.lsb()), instrument->midiAddress().channel());
                    }
                }
            }
        });
    };

    const double ticksPerSecond = static_cast<double>(song->beatsPerMinute()) * song->linesPerBeat() * song->ticksPerLine() / 60.0;
    const double bufferSeconds = static_cast<double>(frames) / sampleRate;
    const auto bufferCount = static_cast<size_t>(seconds / bufferSeconds);

    std::vector<double> costMs;
    costMs.reserve(bufferCount);
    std::vector<double> buffer(static_cast<size_t>(frames) * 2, 0.0);
    quint64 nextTick = 0;

    for (size_t block = 0; block < bufferCount; block++) {
        const auto tickAtEnd = static_cast<quint64>((block + 1) * bufferSeconds * ticksPerSecond);
        for (auto it = eventMap.lower_bound(nextTick); it != eventMap.end() && it->first < tickAtEnd; ++it) {
            for (auto && event : it->second) {
                handleEvent(*event);
            }
        }
        nextTick = tickAtEnd;

        std::fill(buffer.begin(), buffer.end(), 0.0);
        AudioContext context { std::span(buffer.data(), buffer.size()), frames, sampleRate, static_cast<double>(song->beatsPerMinute()), {}, 1, false };
        const auto started = std::chrono::steady_clock::now();
        audioEngine->process(context);
        costMs.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count());
    }

    auto sorted = costMs;
    std::sort(sorted.begin(), sorted.end());
    const double deadlineMs = bufferSeconds * 1000.0;
    const double mean = std::accumulate(costMs.begin(), costMs.end(), 0.0) / costMs.size();
    const auto pct = [&](double p) { return sorted[std::min(sorted.size() - 1, static_cast<size_t>(sorted.size() * p))]; };

    // Bursty backends hand over several buffers back to back; what has to fit the deadline is then
    // the whole burst, not one buffer.
    const size_t burst = 4;
    std::vector<double> burstMs;
    for (size_t i = 0; i + burst <= costMs.size(); i += burst) {
        burstMs.push_back(std::accumulate(costMs.begin() + static_cast<long>(i), costMs.begin() + static_cast<long>(i + burst), 0.0));
    }
    std::sort(burstMs.begin(), burstMs.end());

    qInfo().noquote() << "";
    qInfo().noquote() << QString { "Song            : %1" }.arg(QString::fromUtf8(songPath));
    qInfo().noquote() << QString { "Devices         : %1" }.arg(deviceService->internalDeviceNames().size());
    qInfo().noquote() << QString { "Events          : %1 over %2 distinct ticks" }.arg(events.size()).arg(eventMap.size());
    qInfo().noquote() << QString { "Buffer          : %1 frames @ %2 Hz -> %3 ms deadline" }.arg(frames).arg(sampleRate).arg(deadlineMs, 0, 'f', 2);
    qInfo().noquote() << QString { "Simulated       : %1 buffers (%2 s of song)" }.arg(costMs.size()).arg(seconds);
    qInfo().noquote() << "";
    qInfo().noquote() << QString { "process() mean  : %1 ms  (%2 %% of deadline)" }.arg(mean, 0, 'f', 3).arg(100.0 * mean / deadlineMs, 0, 'f', 1);
    qInfo().noquote() << QString { "process() p50   : %1 ms  (%2 %%)" }.arg(pct(0.50), 0, 'f', 3).arg(100.0 * pct(0.50) / deadlineMs, 0, 'f', 1);
    qInfo().noquote() << QString { "process() p95   : %1 ms  (%2 %%)" }.arg(pct(0.95), 0, 'f', 3).arg(100.0 * pct(0.95) / deadlineMs, 0, 'f', 1);
    qInfo().noquote() << QString { "process() p99   : %1 ms  (%2 %%)" }.arg(pct(0.99), 0, 'f', 3).arg(100.0 * pct(0.99) / deadlineMs, 0, 'f', 1);
    qInfo().noquote() << QString { "process() max   : %1 ms  (%2 %%)" }.arg(sorted.back(), 0, 'f', 3).arg(100.0 * sorted.back() / deadlineMs, 0, 'f', 1);
    const auto over = static_cast<size_t>(std::count_if(costMs.begin(), costMs.end(), [&](double c) { return c > deadlineMs; }));
    qInfo().noquote() << QString { "over deadline   : %1 / %2 buffers (%3 %%)" }.arg(over).arg(costMs.size()).arg(100.0 * over / costMs.size(), 0, 'f', 2);
    if (!burstMs.empty()) {
        qInfo().noquote() << "";
        qInfo().noquote() << QString { "burst of %1 (%2 ms deadline):" }.arg(burst).arg(burst * deadlineMs, 0, 'f', 2);
        qInfo().noquote() << QString { "  p50 %1 ms  p95 %2 ms  max %3 ms (%4 %% of burst deadline)" }
                                 .arg(burstMs[burstMs.size() / 2], 0, 'f', 2)
                                 .arg(burstMs[static_cast<size_t>(burstMs.size() * 0.95)], 0, 'f', 2)
                                 .arg(burstMs.back(), 0, 'f', 2)
                                 .arg(100.0 * burstMs.back() / (burst * deadlineMs), 0, 'f', 1);
    }
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::ParallelRenderTest)
