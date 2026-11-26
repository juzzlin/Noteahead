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

#include "magic_master_calibrator.hpp"

#include "../../../domain/dsp/audio_context.hpp"
#include "../../../domain/dsp/linkwitz_riley_crossover.hpp"
#include "../../../domain/utility/loudness_analyzer.hpp"
#include "../../../domain/utility/spectrum_analyzer.hpp"
#include "magic_master.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace noteahead {

namespace {

constexpr double MinDriveDb { -12.0 };
constexpr double MaxDriveDb { 24.0 };
constexpr size_t BlockFrames { 1024 };

std::array<double, 5> computeAutoEqFromSpectrum(const SpectrumAnalyzer::Result & spec, const SongContext & ctx)
{
    std::array<double, 5> eqDb {};
    if (!spec.isValid) {
        return eqDb;
    }

    const std::array<double, 5> measured {
        0.5 * (static_cast<double>(spec.subDb) + static_cast<double>(spec.bassDb)),
        static_cast<double>(spec.lowMidDb),
        static_cast<double>(spec.midDb),
        static_cast<double>(spec.upperMidDb),
        0.5 * (static_cast<double>(spec.highDb) + static_cast<double>(spec.airDb)),
    };
    const std::array<double, 5> targetContour {
        ctx.hasSubOr808 ? 2.0 : 2.5,
        ctx.arrangementDensity >= 0.65 ? -1.0 : -0.5,
        0.0,
        -0.5,
        -1.5,
    };

    const double maxMove { ctx.arrangementDensity <= 0.3 ? 1.8 : 3.0 };
    for (size_t b = 0; b < 5; b++) {
        if (b == 2) {
            eqDb[b] = 0.0;
            continue;
        }
        const double rel { measured[b] - measured[2] };
        const double err { targetContour[b] - rel };
        if (std::abs(err) > 1.0) {
            const double excess { std::abs(err) - 1.0 };
            eqDb[b] = std::clamp((err >= 0.0 ? excess : -excess) * 0.65, -maxMove, maxMove);
        }
    }
    return eqDb;
}

std::array<double, 3> computeMultibandRefRmsDb(std::span<const float> interleavedStereo, double sr, const SongContext & ctx)
{
    const double bpm { ctx.hasSongData ? std::clamp(ctx.bpm, 40.0, 300.0) : 128.0 };
    const double lowMidHz { ctx.hasSubOr808 ? 125.0 : (bpm >= 145.0 ? 150.0 : 140.0) };
    const double midHighHz { ctx.hasBrightOrFmSynths ? 2800.0 : 3000.0 };

    LinkwitzRileyCrossover lowSplitL;
    LinkwitzRileyCrossover highSplitL;
    LinkwitzRileyCrossover lowAllPassL;
    LinkwitzRileyCrossover lowSplitR;
    LinkwitzRileyCrossover highSplitR;
    LinkwitzRileyCrossover lowAllPassR;
    for (auto * flt : { &lowSplitL, &highSplitL, &lowAllPassL, &lowSplitR, &highSplitR, &lowAllPassR }) {
        flt->setSampleRate(sr);
    }
    lowSplitL.setCutoff(lowMidHz);
    lowAllPassL.setCutoff(midHighHz);
    highSplitL.setCutoff(midHighHz);
    lowSplitR.setCutoff(lowMidHz);
    lowAllPassR.setCutoff(midHighHz);
    highSplitR.setCutoff(midHighHz);

    std::array<double, 3> powerSum {};
    std::array<size_t, 3> activeCount {};
    const size_t totalFrames { interleavedStereo.size() / 2 };

    for (size_t i = 0; i < totalFrames; i++) {
        const double l { static_cast<double>(interleavedStereo[i * 2]) };
        const double r { static_cast<double>(interleavedStereo[i * 2 + 1]) };

        double lowL { 0.0 };
        double midHighL { 0.0 };
        double midL { 0.0 };
        double highL { 0.0 };
        lowSplitL.process(l, lowL, midHighL);
        lowL = lowAllPassL.processAllPass(lowL);
        highSplitL.process(midHighL, midL, highL);

        double lowR { 0.0 };
        double midHighR { 0.0 };
        double midR { 0.0 };
        double highR { 0.0 };
        lowSplitR.process(r, lowR, midHighR);
        lowR = lowAllPassR.processAllPass(lowR);
        highSplitR.process(midHighR, midR, highR);

        const std::array<double, 3> e {
            0.5 * (lowL * lowL + lowR * lowR),
            0.5 * (midL * midL + midR * midR),
            0.5 * (highL * highL + highR * highR),
        };
        for (size_t b = 0; b < 3; b++) {
            if (e[b] > 3.16e-5) { // > -45 dBFS gated active RMS
                powerSum[b] += e[b];
                activeCount[b]++;
            }
        }
    }

    std::array<double, 3> refDb { -18.0, -18.0, -20.0 };
    for (size_t b = 0; b < 3; b++) {
        if (activeCount[b] > 0) {
            const double meanPow { powerSum[b] / static_cast<double>(activeCount[b]) };
            refDb[b] = std::clamp(10.0 * std::log10(std::max(1.0e-10, meanPow)), -36.0, -6.0);
        }
    }
    return refDb;
}

} // namespace

MasterCalibration MagicMasterCalibrator::calibrate(const MagicMaster & source, std::span<const float> interleavedStereo, uint32_t sampleRate)
{
    MasterCalibration result {};
    const size_t totalFrames { interleavedStereo.size() / 2 };
    if (totalFrames == 0 || sampleRate == 0) {
        return result;
    }

    const double sr { static_cast<double>(sampleRate) };
    LoudnessAnalyzer inputLoudness { sr };
    inputLoudness.process(interleavedStereo.data(), totalFrames * 2);
    const auto inputStats { inputLoudness.calculate() };
    result.measuredInputLufs = static_cast<double>(inputStats.integratedLoudness);

    if (result.measuredInputLufs <= -60.0) {
        // Silence or near-silence: leave calibration invalid so live mode stays active safely
        return result;
    }

    SpectrumAnalyzer spectrum { sr, 2 };
    spectrum.process(interleavedStereo.data(), totalFrames * 2);
    const auto specResult { spectrum.calculate() };

    const SongContext ctx { source.songContext() };
    result.autoEqBandDb = computeAutoEqFromSpectrum(specResult, ctx);
    result.multibandRefRmsDb = computeMultibandRefRmsDb(interleavedStereo, sr, ctx);
    result.calibratedForTargetLufs = source.targetLufs();

    const double targetLufs { source.targetLufs() };
    if (!source.autoLoudnessEnabled()) {
        result.valid = true;
        result.driveDb = 0.0;
        result.predictedOutputLufs = result.measuredInputLufs;
        return result;
    }

    const auto evaluateDrive = [&](double trialDriveDb, double & outLufs) {
        MasterCalibration trialCal { result };
        trialCal.valid = true;
        trialCal.driveDb = trialDriveDb;
        trialCal.calibratedForTargetLufs = targetLufs;

        const auto clone { source.cloneConfiguredForCalibration(trialCal) };
        clone->setSampleRate(sr);
        clone->reset();

        const size_t latency { clone->latencyFrames(sr) };
        std::vector<double> doubleBlock(BlockFrames * 2, 0.0);
        std::vector<float> outFloatBlock(BlockFrames * 2, 0.0f);
        LoudnessAnalyzer outAnalyzer { sr };

        size_t latencyToDrop { latency };
        const auto flushOut = [&](size_t frameCount) {
            const size_t drop { std::min(latencyToDrop, frameCount) };
            latencyToDrop -= drop;
            const size_t emitFrames { frameCount - drop };
            if (emitFrames > 0) {
                for (size_t i = 0; i < emitFrames * 2; i++) {
                    outFloatBlock[i] = static_cast<float>(doubleBlock[drop * 2 + i]);
                }
                outAnalyzer.process(outFloatBlock.data(), emitFrames * 2);
            }
        };

        size_t pos { 0 };
        while (pos < totalFrames) {
            const size_t chunk { std::min(BlockFrames, totalFrames - pos) };
            for (size_t i = 0; i < chunk * 2; i++) {
                doubleBlock[i] = static_cast<double>(interleavedStereo[pos * 2 + i]);
            }
            AudioContext audioCtx { std::span<double> { doubleBlock.data(), chunk * 2 }, static_cast<uint32_t>(chunk), sampleRate };
            audioCtx.offline = true;
            audioCtx.oversampleFactor = 4;
            clone->process(audioCtx);
            flushOut(chunk);
            pos += chunk;
        }

        size_t tailOwed { latency - latencyToDrop };
        latencyToDrop = 0;
        while (tailOwed > 0) {
            const size_t chunk { std::min(BlockFrames, tailOwed) };
            std::fill(doubleBlock.begin(), doubleBlock.begin() + chunk * 2, 0.0);
            AudioContext audioCtx { std::span<double> { doubleBlock.data(), chunk * 2 }, static_cast<uint32_t>(chunk), sampleRate };
            audioCtx.offline = true;
            audioCtx.oversampleFactor = 4;
            clone->process(audioCtx);
            flushOut(chunk);
            tailOwed -= chunk;
        }

        outLufs = static_cast<double>(outAnalyzer.calculate().integratedLoudness);
    };

    double bestDrive { std::clamp(targetLufs - result.measuredInputLufs, MinDriveDb, MaxDriveDb) };
    double bestOutLufs { -70.0 };
    evaluateDrive(bestDrive, bestOutLufs);

    double lo { MinDriveDb };
    double hi { MaxDriveDb };
    if (bestOutLufs < targetLufs) {
        lo = bestDrive;
    } else {
        hi = bestDrive;
    }

    for (int iter = 0; iter < 6; iter++) {
        if (std::abs(bestOutLufs - targetLufs) <= 0.15) {
            break;
        }
        const double err { targetLufs - bestOutLufs };
        double nextDrive { 0.5 * (lo + hi) };
        if (iter == 0) {
            nextDrive = std::clamp(bestDrive + err, lo, hi);
        }
        double trialLufs { -70.0 };
        evaluateDrive(nextDrive, trialLufs);
        if (std::abs(trialLufs - targetLufs) < std::abs(bestOutLufs - targetLufs)) {
            bestOutLufs = trialLufs;
            bestDrive = nextDrive;
        }
        if (trialLufs < targetLufs) {
            lo = std::max(lo, nextDrive);
        } else {
            hi = std::min(hi, nextDrive);
        }
    }

    result.valid = true;
    result.driveDb = std::clamp(bestDrive, MinDriveDb, MaxDriveDb);
    result.predictedOutputLufs = bestOutLufs;
    return result;
}

} // namespace noteahead
