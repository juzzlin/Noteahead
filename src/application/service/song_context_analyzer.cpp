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

#include "song_context_analyzer.hpp"

#include "../../common/constants.hpp"
#include "../../domain/devices/device.hpp"
#include "../../domain/effects/effect.hpp"
#include "../../domain/effects/effect_rack.hpp"
#include "../../domain/tracker/instrument.hpp"
#include "../../domain/tracker/song.hpp"
#include "device_service.hpp"
#include "side_chain_service.hpp"
#include "song_overview_builder.hpp"

#include <QString>

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <vector>

namespace noteahead {

namespace {

bool isSubOr808Device(const QString & typeName)
{
    return typeName == Constants::kick808DeviceName() || typeName == Constants::drumSynth808DeviceName() || typeName == Constants::bassSynthDeviceName() || typeName == Constants::electricBassDeviceName();
}

bool isKickOnlyDevice(const QString & typeName)
{
    return typeName == Constants::kick808DeviceName() || typeName == Constants::kick909DeviceName() || typeName == Constants::kickSynthDeviceName();
}

bool isDrumKitDevice(const QString & typeName)
{
    return typeName == Constants::drumSynthDeviceName() || typeName == Constants::drumSynthV2DeviceName() || typeName == Constants::drumSynth808DeviceName() || typeName == Constants::drumSynth909DeviceName() || typeName == Constants::fmDrumsDeviceName();
}

bool isPunchyKickDevice(const QString & typeName)
{
    return typeName == Constants::kick909DeviceName() || typeName == Constants::kickSynthDeviceName() || typeName == Constants::drumSynth909DeviceName() || typeName == Constants::drumSynthDeviceName() || typeName == Constants::drumSynthV2DeviceName() || typeName == Constants::fmDrumsDeviceName();
}

bool isBrightOrFmDevice(const QString & typeName)
{
    return typeName == Constants::fmSynthDeviceName() || typeName == Constants::wavetableSynthDeviceName() || typeName == Constants::noiseFxDeviceName() || typeName == Constants::drumSynth909DeviceName() || typeName == Constants::speechDeviceName() || typeName == Constants::speechV2DeviceName();
}

bool isStereoWideningEffect(const QString & effectType)
{
    return effectType == Constants::RackEffectType::stereoWidener() || effectType == Constants::RackEffectType::stereoEnhancer() || effectType == Constants::RackEffectType::dimension() || effectType == Constants::RackEffectType::doubler() || effectType == Constants::RackEffectType::chorus();
}

double midiToHz(int midiNote)
{
    return 440.0 * std::exp2((static_cast<double>(std::clamp(midiNote, 0, 127)) - 69.0) / 12.0);
}

bool hasEnabledStereoInsert(const Device & device)
{
    for (const auto & rackRef : const_cast<Device &>(device).effectRacks()) {
        for (const auto & effect : rackRef.get().effects()) {
            if (effect && effect->enabled() && isStereoWideningEffect(QString::fromStdString(effect->type()))) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

SongContext SongContextAnalyzer::analyze(const Song & song, const DeviceService & deviceService, const SideChainService * sideChainService)
{
    SongContext ctx {};
    ctx.hasSongData = true;
    ctx.bpm = std::clamp(static_cast<double>(song.beatsPerMinute()), 40.0, 300.0);

    const auto overview { SongOverview::build(song) };
    const size_t linesPerBeat { std::max<size_t>(overview.linesPerBeat, 1) };

    struct TrackInfo
    {
        std::shared_ptr<Instrument> instrument;
        std::shared_ptr<Device> device;
        QString deviceTypeName;
        bool isDrumTrack { false };
        bool isSubOr808 { false };
        bool isKickOnly { false };
        bool isDrumKit { false };
        int pitchShiftSemitones { 0 };
    };

    std::map<size_t, TrackInfo> trackInfos;
    double maxPanDeviation { 0.0 };
    double widthSum { 0.0 };
    size_t widthCount { 0 };
    bool hasStereoInserts { false };

    for (const auto & tr : overview.tracks) {
        TrackInfo info {};
        info.instrument = song.instrument(tr.index);
        if (info.instrument) {
            info.isDrumTrack = info.instrument->settings().drumTrack;
            info.pitchShiftSemitones += info.instrument->settings().transpose;
            const auto portName { info.instrument->midiAddress().portName() };
            if (deviceService.isInternalDevice(portName)) {
                info.device = deviceService.device(portName.toStdString());
            }
        }
        if (info.device) {
            info.deviceTypeName = QString::fromStdString(info.device->typeName());
            info.isSubOr808 = isSubOr808Device(info.deviceTypeName);
            info.isKickOnly = isKickOnlyDevice(info.deviceTypeName);
            info.isDrumKit = isDrumKitDevice(info.deviceTypeName);
            if (info.device->supportsOctave()) {
                info.pitchShiftSemitones += info.device->octave() * 12;
            }
            if (info.isSubOr808) {
                ctx.hasSubOr808 = true;
            }
            if (isPunchyKickDevice(info.deviceTypeName)) {
                ctx.hasPunchyKick = true;
            }
            if (isBrightOrFmDevice(info.deviceTypeName)) {
                ctx.hasBrightOrFmSynths = true;
            }
            maxPanDeviation = std::max(maxPanDeviation, std::abs(static_cast<double>(info.device->pan()) - 0.5) * 2.0);
            widthSum += static_cast<double>(info.device->width());
            widthCount++;
            if (hasEnabledStereoInsert(*info.device)) {
                hasStereoInserts = true;
            }
        }
        if (sideChainService && sideChainService->settings(tr.index).enabled) {
            ctx.hasSidechainDucking = true;
        }
        trackInfos.emplace(tr.index, std::move(info));
    }

    std::set<size_t> activeTracksWithNotes;
    std::set<size_t> tracksWithInstruments;
    std::optional<int> lowestPitchedMidi;
    size_t peakActiveTracksInPattern { 0 };
    size_t peakSimultaneousNotes { 0 };
    double maxPatternScore { 0.0 };
    double minNonEmptyPatternScore { 1.0e9 };
    size_t nonEmptyPatternCount { 0 };

    size_t totalKickBeatsWindow { 0 };
    size_t coveredFourOnFloorBeats { 0 };
    size_t totalKickStrikes { 0 };

    for (const auto & [idx, info] : trackInfos) {
        if (info.device || info.instrument) {
            tracksWithInstruments.insert(idx);
        }
    }

    for (const auto & row : overview.rows) {
        if (row.skipped) {
            continue;
        }
        const auto patIt { overview.patterns.find(row.pattern) };
        if (patIt == overview.patterns.end()) {
            continue;
        }
        const auto & pattern { patIt->second };
        const double patternBeats { static_cast<double>(pattern.lineCount) / static_cast<double>(linesPerBeat) };
        const size_t integerBeats { static_cast<size_t>(std::max(1.0, std::floor(patternBeats))) };

        std::set<size_t> patternActiveTracks;
        std::map<size_t, size_t> lineConcurrency;
        std::set<size_t> patternBeatAlignedKicks;
        bool patternHadKickSource { false };

        for (const auto & note : pattern.notes) {
            activeTracksWithNotes.insert(note.track);
            patternActiveTracks.insert(note.track);
            const size_t quantizedLine { static_cast<size_t>(std::floor(note.startLine)) };
            lineConcurrency[quantizedLine]++;

            const auto trIt { trackInfos.find(note.track) };
            const TrackInfo * info { trIt != trackInfos.end() ? &trIt->second : nullptr };

            const bool isDrumPadDevice { info && info->device && !info->device->supportsOctave() };
            const bool isKickCandidate { info && (info->isKickOnly || info->isDrumKit || info->isDrumTrack) };
            const bool isPitchedCandidate { !info || (!info->isDrumTrack && !info->isDrumKit && !isDrumPadDevice && (!info->isKickOnly || info->deviceTypeName == Constants::kick808DeviceName())) };

            if (isPitchedCandidate) {
                const int shiftedMidi { std::clamp(static_cast<int>(note.pitch) + (info ? info->pitchShiftSemitones : 0), 0, 127) };
                if (!lowestPitchedMidi.has_value() || shiftedMidi < *lowestPitchedMidi) {
                    lowestPitchedMidi = shiftedMidi;
                }
            }

            if (isKickCandidate) {
                bool countsAsKick { false };
                if (info->isKickOnly) {
                    countsAsKick = true;
                } else {
                    // Drum kits use GM kick 35 / 36; fall back to any note on a dedicated drum track if no GM kicks exist
                    countsAsKick = (note.pitch == 35 || note.pitch == 36);
                }
                if (countsAsKick) {
                    patternHadKickSource = true;
                    totalKickStrikes++;
                    const double beatPos { note.startLine / static_cast<double>(linesPerBeat) };
                    const double nearestBeat { std::round(beatPos) };
                    if (std::abs(beatPos - nearestBeat) <= 0.15) {
                        const size_t beatIdx { static_cast<size_t>(std::max(0.0, nearestBeat)) };
                        if (beatIdx < integerBeats) {
                            patternBeatAlignedKicks.insert(beatIdx);
                        }
                    }
                }
            }
        }

        if (patternHadKickSource) {
            totalKickBeatsWindow += integerBeats;
            coveredFourOnFloorBeats += patternBeatAlignedKicks.size();
        }

        peakActiveTracksInPattern = std::max(peakActiveTracksInPattern, patternActiveTracks.size());
        for (const auto & [line, count] : lineConcurrency) {
            peakSimultaneousNotes = std::max(peakSimultaneousNotes, count);
        }

        if (!pattern.notes.empty()) {
            nonEmptyPatternCount++;
            const double densityScore { static_cast<double>(patternActiveTracks.size()) + static_cast<double>(pattern.notes.size()) / std::max(1.0, patternBeats * 2.0) };
            maxPatternScore = std::max(maxPatternScore, densityScore);
            minNonEmptyPatternScore = std::min(minNonEmptyPatternScore, densityScore);
        }
    }

    ctx.activeInstrumentCount = !activeTracksWithNotes.empty() ? activeTracksWithNotes.size() : tracksWithInstruments.size();

    if (lowestPitchedMidi.has_value()) {
        ctx.lowestBassHz = std::clamp(midiToHz(*lowestPitchedMidi), 25.0, 120.0);
    }

    if (totalKickBeatsWindow > 0) {
        const double fourFloorFraction { static_cast<double>(coveredFourOnFloorBeats) / static_cast<double>(totalKickBeatsWindow) };
        ctx.isFourOnTheFloor = fourFloorFraction >= 0.50;
        ctx.kickDensityPerBeat = static_cast<double>(totalKickStrikes) / static_cast<double>(totalKickBeatsWindow);
    }

    if (peakActiveTracksInPattern > 0 || peakSimultaneousNotes > 0) {
        const double trackFactor { std::clamp(static_cast<double>(peakActiveTracksInPattern) / 8.0, 0.0, 1.0) };
        const double voiceFactor { std::clamp(static_cast<double>(peakSimultaneousNotes) / 10.0, 0.0, 1.0) };
        ctx.arrangementDensity = std::clamp(0.6 * trackFactor + 0.4 * voiceFactor, 0.1, 1.0);
    }

    if (nonEmptyPatternCount >= 2 && maxPatternScore > 0.0) {
        ctx.arrangementContrast = std::clamp((maxPatternScore - minNonEmptyPatternScore) / maxPatternScore, 0.0, 1.0);
    }

    if (widthCount > 0 || hasStereoInserts || maxPanDeviation > 0.0) {
        const double avgWidth { widthCount > 0 ? widthSum / static_cast<double>(widthCount) : 1.0 };
        const double baseSpread { 0.45 * avgWidth + 0.35 * maxPanDeviation + (hasStereoInserts ? 0.25 : 0.0) };
        ctx.stereoSpreadHint = std::clamp(baseSpread, 0.15, 1.0);
    }

    std::ostringstream summary;
    summary << static_cast<int>(std::lround(ctx.bpm)) << " BPM";
    if (ctx.isFourOnTheFloor) {
        summary << " \xC2\xB7 4-on-the-floor";
    }
    if (lowestPitchedMidi.has_value() || ctx.hasSubOr808) {
        summary << " \xC2\xB7 Bass " << static_cast<int>(std::lround(ctx.lowestBassHz)) << " Hz";
    }
    if (ctx.hasSubOr808) {
        summary << " \xC2\xB7 808/Sub";
    } else if (ctx.hasPunchyKick) {
        summary << " \xC2\xB7 Punchy Kick";
    }
    if (ctx.hasBrightOrFmSynths) {
        summary << " \xC2\xB7 FM/Bright";
    }
    if (ctx.hasSidechainDucking) {
        summary << " \xC2\xB7 SC Duck";
    }
    summary << " \xC2\xB7 " << ctx.activeInstrumentCount << (ctx.activeInstrumentCount == 1 ? " Track" : " Tracks");
    if (ctx.arrangementDensity >= 0.65) {
        summary << " \xC2\xB7 Dense";
    } else if (ctx.arrangementDensity <= 0.3) {
        summary << " \xC2\xB7 Sparse";
    }
    ctx.summaryText = summary.str();

    return ctx;
}

} // namespace noteahead
