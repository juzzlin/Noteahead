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

#include "magic_master.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/utils.hpp"
#include "../../../domain/dsp/audio_context.hpp"
#include "magic_master_calibrator.hpp"
#include "magic_master_presets.hpp"

#include <QCoreApplication>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>

namespace noteahead {

namespace {

constexpr double ButterworthQ1 { 0.54119610 };
constexpr double ButterworthQ2 { 1.30656296 };
constexpr size_t InitialQuietFrames { std::numeric_limits<size_t>::max() / 4 };
constexpr double SilenceFloor { 1.0e-9 };

constexpr double MinThresholdDb { -24.0 };
constexpr double MaxThresholdDb { 0.0 };
constexpr double MinCeilingDb { -6.0 };
constexpr double MaxCeilingDb { 0.0 };

constexpr std::array<double, 5> EqCenterHz { 65.0, 260.0, 900.0, 3200.0, 11000.0 };
constexpr std::array<double, 5> EqQ { 0.707, 1.0, 0.9, 1.1, 0.707 };
constexpr std::array<double, 5> EqDetectorQ { 1.2, 1.2, 1.2, 1.2, 1.2 };

} // namespace

void MagicMaster::CorrelationTracker::update(double left, double right, double coeff)
{
    lr = coeff * lr + (1.0 - coeff) * (left * right);
    ll = coeff * ll + (1.0 - coeff) * (left * left);
    rr = coeff * rr + (1.0 - coeff) * (right * right);
    const double denom { std::sqrt(std::max(0.0, ll * rr)) };
    if (denom > 1.0e-12) {
        lastCorrelation = std::clamp(lr / denom, -1.0, 1.0);
    }
}

void MagicMaster::CorrelationTracker::reset()
{
    lr = 0.0;
    ll = 0.0;
    rr = 0.0;
    lastCorrelation = 1.0;
}

MagicMaster::MagicMaster()
{
    namespace C = Constants::NahdXml;

    addParameter(Parameter { C::xmlKeyThreshold().toStdString(), 0.75f, -2400, 0, -600, 100 });
    addParameter(Parameter { C::xmlKeyCeiling().toStdString(), 0.8f, -600, 0, -120, 100 });
    addParameter(Parameter { C::xmlKeySubCleanEnabled().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyEqEnabled().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyMultibandEnabled().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyStereoEnabled().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyClipperEnabled().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { C::xmlKeyLimiterEnabled().toStdString(), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });

    using Display = ParameterDisplay;
    setParameterDisplay(C::xmlKeyThreshold().toStdString(), { .mapping = Display::Mapping::Value, .unit = "dB", .min = MinThresholdDb, .max = MaxThresholdDb, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Threshold") });
    setParameterDisplay(C::xmlKeyCeiling().toStdString(), { .mapping = Display::Mapping::Value, .unit = "dBTP", .min = MinCeilingDb, .max = MaxCeilingDb, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Ceiling") });

    for (auto & meter : m_meterAutoEqBandDb) {
        meter.store(0.0f, std::memory_order_relaxed);
    }
    for (auto & meter : m_meterMbReductionDb) {
        meter.store(0.0f, std::memory_order_relaxed);
    }

    syncParameters();
    m_limiter.setLookaheadMs(4.0);
    m_limiter.setAutoRelease(true);
    m_limiter.setBoost(true);

    updateStageConfigurations(true);
    m_quietFrames = InitialQuietFrames;
}

MagicMaster::~MagicMaster() = default;

std::string MagicMaster::typeIdString()
{
    return "e9efb3ca-f61b-46fb-8095-979d318903db";
}

std::string MagicMaster::type() const
{
    return Constants::RackEffectType::magicMaster().toStdString();
}

std::string MagicMaster::typeId() const
{
    return typeIdString();
}

std::vector<std::string> MagicMaster::modulationPriority() const
{
    namespace C = Constants::NahdXml;
    return {
        C::xmlKeyThreshold().toStdString(),
        C::xmlKeyCeiling().toStdString(),
    };
}

const EffectPresetList & MagicMaster::factoryPresets() const
{
    return MagicMasterPresets::presets();
}

void MagicMaster::setSampleRate(double sampleRate)
{
    Effect::setSampleRate(sampleRate);
    m_dcBlockerL.setSampleRate(sampleRate);
    m_dcBlockerR.setSampleRate(sampleRate);
    m_monoBassL.setSampleRate(sampleRate);
    m_monoBassR.setSampleRate(sampleRate);
    m_mbLowSplitL.setSampleRate(sampleRate);
    m_mbHighSplitL.setSampleRate(sampleRate);
    m_mbLowAllPassL.setSampleRate(sampleRate);
    m_mbLowSplitR.setSampleRate(sampleRate);
    m_mbHighSplitR.setSampleRate(sampleRate);
    m_mbLowAllPassR.setSampleRate(sampleRate);
    m_stLowSplitL.setSampleRate(sampleRate);
    m_stHighSplitL.setSampleRate(sampleRate);
    m_stLowAllPassL.setSampleRate(sampleRate);
    m_stLowSplitR.setSampleRate(sampleRate);
    m_stHighSplitR.setSampleRate(sampleRate);
    m_stLowAllPassR.setSampleRate(sampleRate);
    m_limiter.setSampleRate(sampleRate);
    m_outputLoudness.setSampleRate(static_cast<uint32_t>(std::max(1.0, sampleRate)));
    m_outputTruePeak.setSampleRate(sampleRate);
    m_peakDetector.configure(sampleRate);
    m_resonanceBank.setSampleRate(sampleRate);

    updateStageConfigurations(true);
}

void MagicMaster::setBpm(float bpm)
{
    Effect::setBpm(bpm);
    const double activeBpm { m_activeSongContext.hasSongData ? std::clamp(m_activeSongContext.bpm, 40.0, 300.0) : static_cast<double>(bpm) };
    m_limiter.setBpm(activeBpm);
}

size_t MagicMaster::latencyFrames(double sampleRate) const
{
    if (m_calibrationBypass.load(std::memory_order_relaxed)) {
        return 0;
    }

    namespace C = Constants::NahdXml;
    size_t total { 0 };
    if (const auto clipperParam { parameter(C::xmlKeyClipperEnabled().toStdString()) }; !clipperParam || clipperParam->get().value() >= 0.5f) {
        total += ClipperOversampleLatencyFrames;
    }
    if (const auto limiterParam { parameter(C::xmlKeyLimiterEnabled().toStdString()) }; !limiterParam || limiterParam->get().value() >= 0.5f) {
        total += TruePeakLimiterCore::latencyFramesFor(sampleRate, 4.0);
    }
    return total;
}

bool MagicMaster::isSettled() const
{
    if (m_calibrationBypass.load(std::memory_order_relaxed)) {
        return true;
    }
    const size_t currentLatency { latencyFrames(sampleRate()) };
    return (!m_limiterEnabled || m_limiter.isSettled()) && (!m_eqEnabled || m_resonanceBank.isSettled()) && (m_quietFrames >= currentLatency);
}

void MagicMaster::setSongContext(const SongContext & context)
{
    {
        std::lock_guard<std::mutex> lock { m_stateMutex };
        m_pendingSongContext = context;
    }
    m_contextDirty.store(true, std::memory_order_release);
}

SongContext MagicMaster::songContext() const
{
    std::lock_guard<std::mutex> lock { m_stateMutex };
    return m_pendingSongContext;
}

void MagicMaster::requestResetMeters()
{
    m_outputLoudness.requestReset();
    m_resetMetersRequested.store(true, std::memory_order_relaxed);
    m_meterShortTermLufs.store(LoudnessMeter::MinimumLufs, std::memory_order_relaxed);
    m_meterIntegratedLufs.store(LoudnessMeter::MinimumLufs, std::memory_order_relaxed);
    m_meterTruePeakDb.store(-70.0f, std::memory_order_relaxed);
    m_meterTruePeakHoldDb.store(-70.0f, std::memory_order_relaxed);
}

void MagicMaster::setCalibration(const MasterCalibration & calibration)
{
    {
        std::lock_guard<std::mutex> lock { m_stateMutex };
        m_pendingCalibration = calibration;
    }
    m_calibrationDirty.store(true, std::memory_order_release);
    m_meterIsCalibrated.store(calibration.valid, std::memory_order_relaxed);
}

void MagicMaster::clearCalibration()
{
    setCalibration(MasterCalibration {});
}

MasterCalibration MagicMaster::calibration() const
{
    std::lock_guard<std::mutex> lock { m_stateMutex };
    return m_pendingCalibration;
}

void MagicMaster::setCalibrationBypass(bool bypass)
{
    m_calibrationBypass.store(bypass, std::memory_order_relaxed);
}

double MagicMaster::thresholdDb() const
{
    return m_thresholdDb;
}

double MagicMaster::ceilingDb() const
{
    return m_ceilingDb;
}

MasterCalibration MagicMaster::calibrateFromStereo(std::span<const float> interleavedStereo, uint32_t sampleRate) const
{
    return MagicMasterCalibrator::calibrate(*this, interleavedStereo, sampleRate);
}

void MagicMaster::pullPendingSnapshots()
{
    if (m_contextDirty.load(std::memory_order_acquire)) {
        std::unique_lock<std::mutex> lock { m_stateMutex, std::try_to_lock };
        if (lock.owns_lock()) {
            m_activeSongContext = m_pendingSongContext;
            m_contextDirty.store(false, std::memory_order_relaxed);
            if (m_activeSongContext.hasSongData && m_activeSongContext.bpm > 0.0) {
                setBpm(static_cast<float>(m_activeSongContext.bpm));
            }
        }
    }

    if (m_calibrationDirty.load(std::memory_order_acquire)) {
        std::unique_lock<std::mutex> lock { m_stateMutex, std::try_to_lock };
        if (lock.owns_lock()) {
            m_activeCalibration = m_pendingCalibration;
            m_calibrationDirty.store(false, std::memory_order_relaxed);
            m_meterIsCalibrated.store(m_activeCalibration.valid, std::memory_order_relaxed);
            if (m_activeCalibration.valid) {
                m_eqGainDb = m_activeCalibration.autoEqBandDb;
                updateAutoEqCoefficients();
            }
            updateMultibandThresholds();
        }
    }
}

void MagicMaster::syncParameters()
{
    namespace C = Constants::NahdXml;

    const double prevThresholdDb { m_thresholdDb };
    const double prevCeilingDb { m_ceilingDb };
    if (const auto p { parameter(C::xmlKeyThreshold().toStdString()) }; p) {
        m_thresholdDb = MinThresholdDb + static_cast<double>(p->get().value()) * (MaxThresholdDb - MinThresholdDb);
    }
    if (const auto p { parameter(C::xmlKeyCeiling().toStdString()) }; p) {
        m_ceilingDb = MinCeilingDb + static_cast<double>(p->get().value()) * (MaxCeilingDb - MinCeilingDb);
    }
    if (const auto p { parameter(C::xmlKeySubCleanEnabled().toStdString()) }; p) {
        m_subCleanEnabled = p->get().value() > 0.5f;
    }
    if (const auto p { parameter(C::xmlKeyEqEnabled().toStdString()) }; p) {
        m_eqEnabled = p->get().value() > 0.5f;
    }
    if (const auto p { parameter(C::xmlKeyMultibandEnabled().toStdString()) }; p) {
        m_multibandEnabled = p->get().value() > 0.5f;
    }
    if (const auto p { parameter(C::xmlKeyStereoEnabled().toStdString()) }; p) {
        m_stereoEnabled = p->get().value() > 0.5f;
    }
    if (const auto p { parameter(C::xmlKeyClipperEnabled().toStdString()) }; p) {
        m_clipperEnabled = p->get().value() > 0.5f;
    }
    if (const auto p { parameter(C::xmlKeyLimiterEnabled().toStdString()) }; p) {
        m_limiterEnabled = p->get().value() > 0.5f;
    }

    if (std::abs(m_thresholdDb - prevThresholdDb) > 1.0e-9 || std::abs(m_ceilingDb - prevCeilingDb) > 1.0e-9) {
        updateMultibandThresholds();
    }

    m_limiter.setThresholdDb(m_thresholdDb);
    m_limiter.setCeilingDb(m_ceilingDb);
    m_limiter.setLookaheadMs(4.0);
    m_limiter.setAutoRelease(true);
    m_limiter.setBoost(true);
    m_meterLimiterBoostDb.store(m_limiterEnabled ? static_cast<float>(std::max(0.0, m_ceilingDb - m_thresholdDb)) : 0.0f, std::memory_order_relaxed);
}

void MagicMaster::updateAutoEqCoefficients()
{
    const double sr { sampleRate() > 0.0 ? sampleRate() : 48000.0 };

    m_eqBands[0].filterL.calculateLowShelf(EqCenterHz[0], sr, EqQ[0], m_eqGainDb[0]);
    m_eqBands[0].filterR.calculateLowShelf(EqCenterHz[0], sr, EqQ[0], m_eqGainDb[0]);
    m_eqBands[1].filterL.calculateBell(EqCenterHz[1], sr, EqQ[1], m_eqGainDb[1]);
    m_eqBands[1].filterR.calculateBell(EqCenterHz[1], sr, EqQ[1], m_eqGainDb[1]);
    m_eqBands[2].filterL.calculateBell(EqCenterHz[2], sr, EqQ[2], m_eqGainDb[2]);
    m_eqBands[2].filterR.calculateBell(EqCenterHz[2], sr, EqQ[2], m_eqGainDb[2]);
    m_eqBands[3].filterL.calculateBell(EqCenterHz[3], sr, EqQ[3], m_eqGainDb[3]);
    m_eqBands[3].filterR.calculateBell(EqCenterHz[3], sr, EqQ[3], m_eqGainDb[3]);
    m_eqBands[4].filterL.calculateHighShelf(EqCenterHz[4], sr, EqQ[4], m_eqGainDb[4]);
    m_eqBands[4].filterR.calculateHighShelf(EqCenterHz[4], sr, EqQ[4], m_eqGainDb[4]);

    const auto gridFreqs { AutoGain::gridFrequencies(sr) };
    AutoGain::Grid magnitudesDb {};
    for (size_t gi = 0; gi < AutoGain::GridSize; gi++) {
        const double freq { gridFreqs[gi] };
        double mag { 1.0 };
        for (size_t b = 0; b < EqBandCount; b++) {
            mag *= m_eqBands[b].filterL.magnitudeAt(freq, sr);
        }
        magnitudesDb[gi] = 20.0 * std::log10(std::max(1.0e-6, mag));
    }
    const double compDb { AutoGain::compensationDb(magnitudesDb) };
    m_eqAutoGain.setTargetDb(compDb, sr);
}

void MagicMaster::updateMultibandThresholds()
{
    const double boostDb { std::max(0.0, m_ceilingDb - m_thresholdDb) };
    const double aggressiveness { std::clamp(boostDb / 12.0, 0.0, 1.0) };
    const double pushDb { 1.5 + 3.0 * aggressiveness };
    const double makeupDb { std::clamp(0.5 * (pushDb * 0.45), 0.0, 1.5) };
    const double makeupLin { static_cast<double>(Utils::Dsp::dbToLinear(static_cast<float>(makeupDb))) };
    for (size_t b = 0; b < MbBandCount; b++) {
        const double refRmsDb { m_activeCalibration.valid
                                  ? m_activeCalibration.multibandRefRmsDb[b]
                                  : (m_mbBandPower[b] > 1.0e-9 ? 10.0 * std::log10(m_mbBandPower[b]) : -24.0) };
        const double thresholdDb { std::clamp(refRmsDb - pushDb, -28.0, -6.0) };
        m_mbThresholdsDb[b] = thresholdDb;
        m_mbComps[b].setThresholdDb(thresholdDb);
        m_mbMakeupsLin[b] = makeupLin;
    }
}

void MagicMaster::updateStageConfigurations(bool force)
{
    const double sr { sampleRate() > 0.0 ? sampleRate() : 48000.0 };
    const double activeBpm { m_activeSongContext.hasSongData ? std::clamp(m_activeSongContext.bpm, 40.0, 300.0) : static_cast<double>(bpm()) };
    const double activeSubHpfHz { m_activeSongContext.hasSongData
                                    ? std::clamp(m_activeSongContext.lowestBassHz * 0.72, 22.0, m_activeSongContext.hasSubOr808 ? 26.0 : 36.0)
                                    : 28.0 };
    const double activeMonoBassHz { (m_activeSongContext.hasSubOr808 || activeBpm < 118.0) ? 115.0 : (activeBpm >= 145.0 ? 135.0 : 125.0) };
    const double activeLowMidHz { m_activeSongContext.hasSubOr808 ? 125.0 : (activeBpm >= 145.0 ? 150.0 : 140.0) };
    const double activeMidHighHz { m_activeSongContext.hasBrightOrFmSynths ? 2800.0 : 3000.0 };

    const bool srChanged { force || sr != m_configuredSampleRate };
    if (srChanged) {
        m_dcBlockerL.setSampleRate(sr);
        m_dcBlockerR.setSampleRate(sr);
        m_monoBassL.setSampleRate(sr);
        m_monoBassR.setSampleRate(sr);
        m_mbLowSplitL.setSampleRate(sr);
        m_mbHighSplitL.setSampleRate(sr);
        m_mbLowAllPassL.setSampleRate(sr);
        m_mbLowSplitR.setSampleRate(sr);
        m_mbHighSplitR.setSampleRate(sr);
        m_mbLowAllPassR.setSampleRate(sr);
        m_stLowSplitL.setSampleRate(sr);
        m_stHighSplitL.setSampleRate(sr);
        m_stLowAllPassL.setSampleRate(sr);
        m_stLowSplitR.setSampleRate(sr);
        m_stHighSplitR.setSampleRate(sr);
        m_stLowAllPassR.setSampleRate(sr);

        constexpr size_t NormBins { 256 };
        const double fMin { 20.0 };
        const double fMax { std::max(fMin * 2.0, 0.48 * sr) };
        const double logMin { std::log(fMin) };
        const double logSpan { std::log(fMax) - logMin };
        for (size_t b = 0; b < EqBandCount; b++) {
            m_eqBands[b].detector.calculateBandPass(EqCenterHz[b], sr, EqDetectorQ[b]);
            double pinkIntegral { 0.0 };
            for (size_t k = 0; k < NormBins; k++) {
                const double f { std::exp(logMin + logSpan * (static_cast<double>(k) / static_cast<double>(NormBins - 1))) };
                const double mag { m_eqBands[b].detector.magnitudeAt(f, sr) };
                pinkIntegral += mag * mag;
            }
            m_eqDetectorPinkNorm[b] = std::max(1.0e-12, pinkIntegral / static_cast<double>(NormBins));
        }
        m_eqDetectorCoeff = std::exp(-1.0 / (1.2 * sr));
        m_eqSmoothCoeff = std::exp(-1.0 / (0.25 * sr / 64.0));
        m_eqFastCoeff = std::exp(-1.0 / (0.020 * sr));
        m_mbRmsCoeff = std::exp(-1.0 / (1.5 * sr));
        m_corrCoeff = std::exp(-1.0 / (0.15 * sr));
        m_widthSmoothCoeff = std::exp(-1.0 / (0.04 * sr));
        m_sheenEnvCoeff = std::exp(-1.0 / (0.025 * sr));
        m_clipMeterReleaseCoeff = std::exp(-1.0 / (0.100 * sr));

        m_sheenPreHpfL.calculateHighCut(4500.0, sr, 0.707);
        m_sheenPreHpfR.calculateHighCut(4500.0, sr, 0.707);
        m_sheenPostHpfL.calculateHighCut(4500.0, sr, 0.707);
        m_sheenPostHpfR.calculateHighCut(4500.0, sr, 0.707);

        m_peakDetector.setChannelCount(2);
        m_peakDetector.configure(sr);
        m_resonanceBank.setSampleRate(sr);
        m_resonanceBank.setBellCount(6);
        m_resonanceBank.setQ(4.5);
        m_resonanceBank.setAttackMs(12.0);
        m_resonanceBank.setReleaseMs(140.0);

        for (auto & comp : m_mbComps) {
            comp.setSampleRate(sr);
        }
        m_limiter.setSampleRate(sr);
        m_outputLoudness.setSampleRate(static_cast<uint32_t>(std::max(1.0, sr)));
        m_outputTruePeak.setSampleRate(sr);

        m_configuredSampleRate = sr;
    }

    if (srChanged || std::abs(activeSubHpfHz - m_configuredSubHpfHz) > 0.05) {
        m_subHpf1L.calculateLowCut(activeSubHpfHz, sr, ButterworthQ1);
        m_subHpf2L.calculateLowCut(activeSubHpfHz, sr, ButterworthQ2);
        m_subHpf1R.calculateLowCut(activeSubHpfHz, sr, ButterworthQ1);
        m_subHpf2R.calculateLowCut(activeSubHpfHz, sr, ButterworthQ2);
        m_subHpfHz = activeSubHpfHz;
        m_configuredSubHpfHz = activeSubHpfHz;
        m_meterSubHpfHz.store(static_cast<float>(activeSubHpfHz), std::memory_order_relaxed);
    }

    if (srChanged || std::abs(activeMonoBassHz - m_configuredMonoBassHz) > 0.05) {
        m_monoBassL.setCutoff(activeMonoBassHz);
        m_monoBassR.setCutoff(activeMonoBassHz);
        m_stLowSplitL.setCutoff(activeMonoBassHz);
        m_stLowAllPassL.setCutoff(2500.0);
        m_stLowSplitR.setCutoff(activeMonoBassHz);
        m_stLowAllPassR.setCutoff(2500.0);
        m_stHighSplitL.setCutoff(2500.0);
        m_stHighSplitR.setCutoff(2500.0);
        m_monoBassHz = activeMonoBassHz;
        m_configuredMonoBassHz = activeMonoBassHz;
        m_meterMonoBassHz.store(static_cast<float>(activeMonoBassHz), std::memory_order_relaxed);
    }

    if (srChanged || std::abs(activeLowMidHz - m_configuredLowMidHz) > 0.05 || std::abs(activeMidHighHz - m_configuredMidHighHz) > 0.05) {
        m_mbLowSplitL.setCutoff(activeLowMidHz);
        m_mbLowAllPassL.setCutoff(activeMidHighHz);
        m_mbHighSplitL.setCutoff(activeMidHighHz);
        m_mbLowSplitR.setCutoff(activeLowMidHz);
        m_mbLowAllPassR.setCutoff(activeMidHighHz);
        m_mbHighSplitR.setCutoff(activeMidHighHz);
        m_configuredLowMidHz = activeLowMidHz;
        m_configuredMidHighHz = activeMidHighHz;
    }

    if (srChanged || m_activeSongContext.hasBrightOrFmSynths != m_configuredBright) {
        SpectralPeakDetector::Settings peakSettings {};
        peakSettings.lowHz = 200.0;
        peakSettings.highHz = 12000.0;
        peakSettings.focus = 0.2;
        peakSettings.selectivityDb = m_activeSongContext.hasBrightOrFmSynths ? 5.5 : 6.5;
        peakSettings.depthDb = m_activeSongContext.hasBrightOrFmSynths ? 4.0 : 3.2;
        m_peakDetector.setSettings(peakSettings);
        m_configuredBright = m_activeSongContext.hasBrightOrFmSynths;
    }

    if (srChanged || std::abs(activeBpm - m_configuredBpm) > 0.05 || m_activeSongContext.hasSubOr808 != m_configuredSub808 || m_activeSongContext.hasSidechainDucking != m_configuredSidechain) {
        const double beatMs { 60000.0 / activeBpm };

        m_mbComps[0].setDetectorMode(CompressorCore::DetectorMode::Rms);
        m_mbComps[0].setAttackMs(20.0);
        m_mbComps[0].setReleaseMs(std::clamp(0.35 * beatMs, 60.0, 180.0));
        m_mbComps[0].setRatio(m_activeSongContext.hasSidechainDucking ? 2.0 : (m_activeSongContext.hasSubOr808 ? 2.2 : 2.6));
        m_mbComps[0].setKneeDb(6.0);

        m_mbComps[1].setDetectorMode(CompressorCore::DetectorMode::Rms);
        m_mbComps[1].setAttackMs(16.0);
        m_mbComps[1].setReleaseMs(std::clamp(0.50 * beatMs, 80.0, 220.0));
        m_mbComps[1].setRatio(m_activeSongContext.hasSidechainDucking ? 1.7 : 2.1);
        m_mbComps[1].setKneeDb(6.0);

        m_mbComps[2].setDetectorMode(CompressorCore::DetectorMode::Peak);
        m_mbComps[2].setAttackMs(6.0);
        m_mbComps[2].setReleaseMs(std::clamp(0.18 * beatMs, 30.0, 90.0));
        m_mbComps[2].setRatio(2.4);
        m_mbComps[2].setKneeDb(4.0);

        m_limiter.setBpm(activeBpm);

        m_configuredBpm = activeBpm;
        m_configuredSub808 = m_activeSongContext.hasSubOr808;
        m_configuredSidechain = m_activeSongContext.hasSidechainDucking;
    }

    if (srChanged) {
        updateAutoEqCoefficients();
        updateMultibandThresholds();
    }
}

void MagicMaster::processSubCleanStage(double & left, double & right)
{
    left = m_dcBlockerL.process(left);
    right = m_dcBlockerR.process(right);

    left = m_subHpf2L.process(m_subHpf1L.process(left));
    right = m_subHpf2R.process(m_subHpf1R.process(right));

    double lowL { 0.0 };
    double highL { 0.0 };
    double lowR { 0.0 };
    double highR { 0.0 };
    m_monoBassL.process(left, lowL, highL);
    m_monoBassR.process(right, lowR, highR);

    const double monoLow { 0.5 * (lowL + lowR) };
    left = monoLow + highL;
    right = monoLow + highR;
}

void MagicMaster::processAutoEqStage(double & left, double & right)
{
    const double mono { 0.5 * (left + right) };
    m_eqFastPower = m_eqFastCoeff * m_eqFastPower + (1.0 - m_eqFastCoeff) * (mono * mono);
    if (!m_activeCalibration.valid) {
        const bool activeSignal { m_eqFastPower > 1.0e-5 };
        for (size_t b = 0; b < EqBandCount; b++) {
            const double d { m_eqBands[b].detector.process(mono) };
            if (activeSignal) {
                m_eqEnvPower[b] = m_eqDetectorCoeff * m_eqEnvPower[b] + (1.0 - m_eqDetectorCoeff) * (d * d);
            }
        }
        if (activeSignal) {
            m_eqUpdateCounter++;
            if (m_eqUpdateCounter >= 64) {
                m_eqUpdateCounter = 0;
                const std::array<double, EqBandCount> contour {
                    m_activeSongContext.hasSubOr808 ? 1.5 : 2.0,
                    m_activeSongContext.arrangementDensity >= 0.65 ? -0.5 : 0.0,
                    0.0,
                    0.5,
                    -1.0
                };
                const double refPower { std::max(1.0e-12, m_eqEnvPower[2] / m_eqDetectorPinkNorm[2]) };
                const double maxMove { m_activeSongContext.arrangementDensity <= 0.3 ? 3.0 : 4.0 };

                for (size_t b = 0; b < EqBandCount; b++) {
                    if (b == 2) {
                        m_eqGainDb[b] = 0.0;
                        continue;
                    }
                    const double normPower { std::max(1.0e-12, m_eqEnvPower[b] / m_eqDetectorPinkNorm[b]) };
                    const double relDb { 10.0 * std::log10(normPower / refPower) };
                    const double err { contour[b] - relDb };
                    double targetMove { 0.0 };
                    if (std::abs(err) > 0.5) {
                        const double excess { std::abs(err) - 0.5 };
                        targetMove = std::clamp((err >= 0.0 ? excess : -excess) * 0.65, -maxMove, maxMove);
                    }
                    m_eqGainDb[b] = m_eqSmoothCoeff * m_eqGainDb[b] + (1.0 - m_eqSmoothCoeff) * targetMove;
                }
                updateAutoEqCoefficients();
            }
        }
    } else {
        m_eqGainDb = m_activeCalibration.autoEqBandDb;
    }

    for (size_t b = 0; b < EqBandCount; b++) {
        left = m_eqBands[b].filterL.process(left);
        right = m_eqBands[b].filterR.process(right);
    }

    const double gainComp { m_eqAutoGain.next() };
    left *= gainComp;
    right *= gainComp;

    const std::array<double, 2> stereoFrame { left, right };
    if (m_peakDetector.push(stereoFrame.data())) {
        m_resonanceBank.track(m_peakDetector.peaks());
    }
    m_resonanceBank.advance();
    left = m_resonanceBank.process(0, left);
    right = m_resonanceBank.process(1, right);
}

void MagicMaster::processMultibandStage(double & left, double & right)
{
    double lowL { 0.0 };
    double midHighL { 0.0 };
    double midL { 0.0 };
    double highL { 0.0 };
    m_mbLowSplitL.process(left, lowL, midHighL);
    lowL = m_mbLowAllPassL.processAllPass(lowL);
    m_mbHighSplitL.process(midHighL, midL, highL);

    double lowR { 0.0 };
    double midHighR { 0.0 };
    double midR { 0.0 };
    double highR { 0.0 };
    m_mbLowSplitR.process(right, lowR, midHighR);
    lowR = m_mbLowAllPassR.processAllPass(lowR);
    m_mbHighSplitR.process(midHighR, midR, highR);

    const std::array<double, MbBandCount> bandEnergies {
        0.5 * (lowL * lowL + lowR * lowR),
        0.5 * (midL * midL + midR * midR),
        0.5 * (highL * highL + highR * highR)
    };

    if (!m_activeCalibration.valid) {
        for (size_t b = 0; b < MbBandCount; b++) {
            if (bandEnergies[b] > 1.0e-5) { // > -50 dBFS
                m_mbBandPower[b] = m_mbRmsCoeff * m_mbBandPower[b] + (1.0 - m_mbRmsCoeff) * bandEnergies[b];
            }
        }
    }

    m_mbThresholdCounter++;
    if (m_mbThresholdCounter >= 64) {
        m_mbThresholdCounter = 0;
        updateMultibandThresholds();
    }

    const auto dbToLin = [](double db) {
        return db == 0.0 ? 1.0 : std::pow(10.0, db * 0.05);
    };
    const double gainLow { dbToLin(m_mbComps[0].processGainDb(lowL, lowR)) * m_mbMakeupsLin[0] };
    const double gainMid { dbToLin(m_mbComps[1].processGainDb(midL, midR)) * m_mbMakeupsLin[1] };
    const double gainHigh { dbToLin(m_mbComps[2].processGainDb(highL, highR)) * m_mbMakeupsLin[2] };

    left = lowL * gainLow + midL * gainMid + highL * gainHigh;
    right = lowR * gainLow + midR * gainMid + highR * gainHigh;
}

void MagicMaster::processStereoAirStage(double & left, double & right)
{
    double lowL { 0.0 };
    double midHighL { 0.0 };
    double midL { 0.0 };
    double highL { 0.0 };
    m_stLowSplitL.process(left, lowL, midHighL);
    lowL = m_stLowAllPassL.processAllPass(lowL);
    m_stHighSplitL.process(midHighL, midL, highL);

    double lowR { 0.0 };
    double midHighR { 0.0 };
    double midR { 0.0 };
    double highR { 0.0 };
    m_stLowSplitR.process(right, lowR, midHighR);
    lowR = m_stLowAllPassR.processAllPass(lowR);
    m_stHighSplitR.process(midHighR, midR, highR);

    m_corrMid.update(midL, midR, m_corrCoeff);
    m_corrHigh.update(highL, highR, m_corrCoeff);

    const double spreadHint { m_activeSongContext.hasSongData ? m_activeSongContext.stereoSpreadHint : 0.5 };
    const double desiredMid { 1.00 + (1.0 - spreadHint) * 0.12 };
    const double desiredHigh { 1.05 + (1.0 - spreadHint) * 0.19 };

    const auto guardWidth = [](double desired, double corr) {
        if (corr < 0.0) {
            return std::clamp(1.0 + 0.20 * corr, 0.80, 1.0);
        }
        if (corr < 0.35) {
            return 1.0 + (desired - 1.0) * (corr / 0.35);
        }
        return desired;
    };

    const double targetMidW { guardWidth(desiredMid, m_corrMid.lastCorrelation) };
    const double targetHighW { guardWidth(desiredHigh, m_corrHigh.lastCorrelation) };

    m_currentMidWidth = m_widthSmoothCoeff * m_currentMidWidth + (1.0 - m_widthSmoothCoeff) * targetMidW;
    m_currentHighWidth = m_widthSmoothCoeff * m_currentHighWidth + (1.0 - m_widthSmoothCoeff) * targetHighW;

    const auto widenBand = [](double & bL, double & bR, double width) {
        const double m { 0.5 * (bL + bR) };
        const double s { 0.5 * (bL - bR) * width };
        bL = m + s;
        bR = m - s;
    };

    widenBand(midL, midR, m_currentMidWidth);
    widenBand(highL, highR, m_currentHighWidth);

    double mainL { lowL + midL + highL };
    double mainR { lowR + midR + highR };

    // Zero-main-path-latency 2x oversampled pure HF harmonic sheen (> 4.5 kHz)
    const double sideL { mainL - m_sheenPreHpfL.process(mainL) };
    const double sideR { mainR - m_sheenPreHpfR.process(mainR) };
    const double instHf { std::max(std::abs(sideL), std::abs(sideR)) };
    m_sheenHfEnv = std::max(instHf, m_sheenEnvCoeff * m_sheenHfEnv + (1.0 - m_sheenEnvCoeff) * instHf);

    if (m_sheenHfEnv > 1.0e-5) {
        const double normInv { 1.0 / std::max(m_sheenHfEnv, 0.02) };
        std::array<float, 2> upL {};
        std::array<float, 2> upR {};
        m_sheenUpL.process(static_cast<float>(std::clamp(sideL * normInv, -1.0, 1.0)), upL[0], upL[1]);
        m_sheenUpR.process(static_cast<float>(std::clamp(sideR * normInv, -1.0, 1.0)), upR[0], upR[1]);

        for (size_t k = 0; k < 2; k++) {
            const auto shapeHarmonic = [](float s) {
                const float c { std::clamp(s, -1.0f, 1.0f) };
                // Pure generated harmonics: fundamental cancelled cleanly
                return 0.7f * (std::tanh(2.5f * c) - 2.5f * c) + 0.12f * (c * c - 0.25f);
            };
            upL[k] = shapeHarmonic(upL[k]);
            upR[k] = shapeHarmonic(upR[k]);
        }

        const double hL { static_cast<double>(m_sheenDownL.process(upL[0], upL[1])) };
        const double hR { static_cast<double>(m_sheenDownR.process(upR[0], upR[1])) };
        const double airL { hL - m_sheenPostHpfL.process(hL) };
        const double airR { hR - m_sheenPostHpfR.process(hR) };
        const double sheenMix { m_activeSongContext.hasBrightOrFmSynths
                                  ? static_cast<double>(Utils::Dsp::dbToLinear(-24.0f))
                                  : static_cast<double>(Utils::Dsp::dbToLinear(-20.0f)) };
        mainL += airL * m_sheenHfEnv * sheenMix;
        mainR += airR * m_sheenHfEnv * sheenMix;
    }

    left = mainL;
    right = mainR;
}

double MagicMaster::softClip4xSample(double x, double ceilingLin, double kneeLin) const
{
    const double ax { std::abs(x) };
    if (ax <= kneeLin || ceilingLin <= kneeLin) {
        return x;
    }
    const double headroom { ceilingLin - kneeLin };
    const double shaped { kneeLin + headroom * std::tanh((ax - kneeLin) / headroom) };
    return x >= 0.0 ? shaped : -shaped;
}

void MagicMaster::updateClipperMeter(double peakPre, double peakPost)
{
    double reductionDb { 0.0 };
    if (peakPre > 1.0e-10 && peakPost < peakPre) {
        reductionDb = static_cast<double>(Utils::Dsp::linearToDb(static_cast<float>(peakPost / peakPre)));
    }
    if (reductionDb == 0.0 && m_clipReductionDb == 0.0) {
        return;
    }
    if (reductionDb < m_clipReductionDb) {
        m_clipReductionDb = reductionDb;
    } else {
        m_clipReductionDb = m_clipMeterReleaseCoeff * m_clipReductionDb + (1.0 - m_clipMeterReleaseCoeff) * reductionDb;
    }
    if (std::abs(m_clipReductionDb) < 1.0e-15) {
        m_clipReductionDb = 0.0;
    }
}

void MagicMaster::processClipperStage(double & left, double & right)
{
    const double clipCeilingDb { m_limiterEnabled
                                   ? std::min(0.0, std::min(m_thresholdDb, m_ceilingDb) + 2.0)
                                   : m_ceilingDb };
    const double ceilingLin { std::max(1.0e-5, static_cast<double>(Utils::Dsp::dbToLinear(static_cast<float>(clipCeilingDb)))) };
    const double kneeDb { m_limiterEnabled ? 3.0 : 2.5 };
    const double kneeLin { ceilingLin * static_cast<double>(Utils::Dsp::dbToLinear(static_cast<float>(-kneeDb))) };

    std::array<float, 4> highL {};
    std::array<float, 4> highR {};
    m_clipUpL.process(static_cast<float>(left), highL.data(), 4);
    m_clipUpR.process(static_cast<float>(right), highR.data(), 4);

    double peakPre { 0.0 };
    double peakPost { 0.0 };
    for (size_t k = 0; k < 4; k++) {
        const double preAbs { std::max(std::abs(static_cast<double>(highL[k])), std::abs(static_cast<double>(highR[k]))) };
        peakPre = std::max(peakPre, preAbs);
        highL[k] = static_cast<float>(softClip4xSample(static_cast<double>(highL[k]), ceilingLin, kneeLin));
        highR[k] = static_cast<float>(softClip4xSample(static_cast<double>(highR[k]), ceilingLin, kneeLin));
        const double postAbs { std::max(std::abs(static_cast<double>(highL[k])), std::abs(static_cast<double>(highR[k]))) };
        peakPost = std::max(peakPost, postAbs);
    }
    updateClipperMeter(peakPre, peakPost);

    // Insert exact 1-sample delay at 4x rate -> total group delay = 276 4x-samples = exact 69 base frames
    const std::array<float, 4> alignedL { m_clipPrevL, highL[0], highL[1], highL[2] };
    const std::array<float, 4> alignedR { m_clipPrevR, highR[0], highR[1], highR[2] };
    m_clipPrevL = highL[3];
    m_clipPrevR = highR[3];

    left = static_cast<double>(m_clipDownL.process(alignedL.data(), 4));
    right = static_cast<double>(m_clipDownR.process(alignedR.data(), 4));
}

void MagicMaster::processSingleFrame(double & left, double & right)
{
    if (std::max(std::abs(left), std::abs(right)) < SilenceFloor) {
        if (m_quietFrames < InitialQuietFrames) {
            m_quietFrames++;
        }
    } else {
        m_quietFrames = 0;
    }

    if (m_subCleanEnabled) {
        processSubCleanStage(left, right);
    }

    if (m_eqEnabled) {
        processAutoEqStage(left, right);
    }

    if (m_multibandEnabled) {
        processMultibandStage(left, right);
    }

    if (m_stereoEnabled) {
        processStereoAirStage(left, right);
    }

    if (m_clipperEnabled) {
        processClipperStage(left, right);
    } else {
        m_clipReductionDb = 0.0;
    }

    if (m_limiterEnabled) {
        m_limiter.processSample(left, right);
    }

    m_outputLoudness.processSample(left, right);
    m_outputTruePeak.processSample(left, right);
}

void MagicMaster::publishTelemetry()
{
    m_meterShortTermLufs.store(m_outputLoudness.shortTermLufs(), std::memory_order_relaxed);
    m_meterIntegratedLufs.store(m_outputLoudness.integratedLufs(), std::memory_order_relaxed);
    m_meterTruePeakDb.store(std::max(m_outputTruePeak.truePeakL(), m_outputTruePeak.truePeakR()), std::memory_order_relaxed);
    m_meterTruePeakHoldDb.store(std::max(m_outputTruePeak.truePeakHoldL(), m_outputTruePeak.truePeakHoldR()), std::memory_order_relaxed);
    m_meterLimiterBoostDb.store(m_limiterEnabled ? static_cast<float>(std::max(0.0, m_ceilingDb - m_thresholdDb)) : 0.0f, std::memory_order_relaxed);
    m_meterIsCalibrated.store(m_activeCalibration.valid, std::memory_order_relaxed);

    double maxEqMove { 0.0 };
    for (size_t b = 0; b < EqBandCount; b++) {
        const float bandDb { m_eqEnabled ? static_cast<float>(m_eqGainDb[b]) : 0.0f };
        m_meterAutoEqBandDb[b].store(bandDb, std::memory_order_relaxed);
        if (m_eqEnabled && std::abs(m_eqGainDb[b]) > std::abs(maxEqMove)) {
            maxEqMove = m_eqGainDb[b];
        }
    }
    m_meterAutoEqMaxMoveDb.store(static_cast<float>(maxEqMove), std::memory_order_relaxed);

    double maxCut { 0.0 };
    if (m_eqEnabled) {
        std::array<TrackingBellBank::Bell, TrackingBellBank::MaxBells> bells {};
        const size_t activeCount { m_resonanceBank.activeBells(bells) };
        for (size_t i = 0; i < activeCount; i++) {
            maxCut = std::min(maxCut, bells[i].gainDb);
        }
    }
    m_meterResonanceCutDb.store(static_cast<float>(maxCut), std::memory_order_relaxed);

    for (size_t b = 0; b < MbBandCount; b++) {
        const double red { m_multibandEnabled ? m_mbComps[b].reductionDb() : 0.0 };
        m_meterMbReductionDb[b].store(static_cast<float>(red), std::memory_order_relaxed);
    }

    if (m_stereoEnabled) {
        m_meterStereoCorr.store(static_cast<float>(0.5 * (m_corrMid.lastCorrelation + m_corrHigh.lastCorrelation)), std::memory_order_relaxed);
        m_meterStereoWidthPct.store(static_cast<float>(100.0 * 0.5 * (m_currentMidWidth + m_currentHighWidth)), std::memory_order_relaxed);
    } else {
        m_meterStereoCorr.store(1.0f, std::memory_order_relaxed);
        m_meterStereoWidthPct.store(100.0f, std::memory_order_relaxed);
    }

    m_meterClipperReductionDb.store(m_clipperEnabled ? static_cast<float>(m_clipReductionDb) : 0.0f, std::memory_order_relaxed);
    m_meterLimiterReductionDb.store(m_limiterEnabled ? m_limiter.reductionDb() : 0.0f, std::memory_order_relaxed);
}

void MagicMaster::processSample(double & left, double & right)
{
    if (m_calibrationBypass.load(std::memory_order_relaxed)) {
        return;
    }
    if (m_resetMetersRequested.exchange(false, std::memory_order_relaxed)) {
        m_outputTruePeak.reset();
        m_outputLoudness.reset();
    }

    syncParameters();
    pullPendingSnapshots();
    updateStageConfigurations(false);

    processSingleFrame(left, right);
    publishTelemetry();
}

void MagicMaster::processBlock(AudioContext & context)
{
    if (m_calibrationBypass.load(std::memory_order_relaxed)) {
        return;
    }
    if (m_resetMetersRequested.exchange(false, std::memory_order_relaxed)) {
        m_outputTruePeak.reset();
        m_outputLoudness.reset();
    }

    syncParameters();
    pullPendingSnapshots();
    updateStageConfigurations(false);

    const uint32_t frames { context.frameCount };
    for (uint32_t i = 0; i < frames; i++) {
        processSingleFrame(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
    publishTelemetry();
}

void MagicMaster::reset()
{
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
    m_subHpf1L.reset();
    m_subHpf2L.reset();
    m_subHpf1R.reset();
    m_subHpf2R.reset();
    m_monoBassL.reset();
    m_monoBassR.reset();

    for (auto & band : m_eqBands) {
        band.filterL.reset();
        band.filterR.reset();
        band.detector.reset();
    }
    std::fill(m_eqEnvPower.begin(), m_eqEnvPower.end(), 0.0);
    m_eqFastPower = 0.0;
    if (!m_activeCalibration.valid) {
        std::fill(m_eqGainDb.begin(), m_eqGainDb.end(), 0.0);
    } else {
        m_eqGainDb = m_activeCalibration.autoEqBandDb;
    }
    m_eqUpdateCounter = 0;
    m_peakDetector.reset();
    m_resonanceBank.reset();

    m_mbLowSplitL.reset();
    m_mbHighSplitL.reset();
    m_mbLowAllPassL.reset();
    m_mbLowSplitR.reset();
    m_mbHighSplitR.reset();
    m_mbLowAllPassR.reset();
    for (auto & comp : m_mbComps) {
        comp.reset();
    }
    std::fill(m_mbBandPower.begin(), m_mbBandPower.end(), 0.0);
    m_mbThresholdCounter = 0;

    m_stLowSplitL.reset();
    m_stHighSplitL.reset();
    m_stLowAllPassL.reset();
    m_stLowSplitR.reset();
    m_stHighSplitR.reset();
    m_stLowAllPassR.reset();
    m_corrMid.reset();
    m_corrHigh.reset();
    m_currentMidWidth = 1.0;
    m_currentHighWidth = 1.0;

    m_sheenPreHpfL.reset();
    m_sheenPreHpfR.reset();
    m_sheenPostHpfL.reset();
    m_sheenPostHpfR.reset();
    m_sheenUpL.reset();
    m_sheenUpR.reset();
    m_sheenDownL.reset();
    m_sheenDownR.reset();
    m_sheenHfEnv = 0.0;

    m_clipUpL.reset();
    m_clipUpR.reset();
    m_clipDownL.reset();
    m_clipDownR.reset();
    m_clipPrevL = 0.0f;
    m_clipPrevR = 0.0f;
    m_clipReductionDb = 0.0;

    m_limiter.reset();
    m_outputLoudness.reset();
    m_outputTruePeak.reset();
    m_quietFrames = InitialQuietFrames;

    // Preserve m_pendingSongContext / m_activeSongContext and m_pendingCalibration / m_activeCalibration across reset()
    updateStageConfigurations(true);
    m_eqAutoGain.reset();

    m_meterShortTermLufs.store(LoudnessMeter::MinimumLufs, std::memory_order_relaxed);
    m_meterIntegratedLufs.store(LoudnessMeter::MinimumLufs, std::memory_order_relaxed);
    m_meterTruePeakDb.store(-70.0f, std::memory_order_relaxed);
    m_meterTruePeakHoldDb.store(-70.0f, std::memory_order_relaxed);
    m_meterLimiterBoostDb.store(m_limiterEnabled ? static_cast<float>(std::max(0.0, m_ceilingDb - m_thresholdDb)) : 0.0f, std::memory_order_relaxed);
    m_meterAutoEqMaxMoveDb.store(0.0f, std::memory_order_relaxed);
    for (auto & meter : m_meterAutoEqBandDb) {
        meter.store(0.0f, std::memory_order_relaxed);
    }
    m_meterResonanceCutDb.store(0.0f, std::memory_order_relaxed);
    for (auto & meter : m_meterMbReductionDb) {
        meter.store(0.0f, std::memory_order_relaxed);
    }
    m_meterStereoCorr.store(1.0f, std::memory_order_relaxed);
    m_meterStereoWidthPct.store(100.0f, std::memory_order_relaxed);
    m_meterClipperReductionDb.store(0.0f, std::memory_order_relaxed);
    m_meterLimiterReductionDb.store(0.0f, std::memory_order_relaxed);
}

void MagicMaster::sync()
{
    syncParameters();
    pullPendingSnapshots();
    updateStageConfigurations(false);
}

float MagicMaster::shortTermLufs() const
{
    return m_meterShortTermLufs.load(std::memory_order_relaxed);
}

float MagicMaster::integratedLufs() const
{
    return m_meterIntegratedLufs.load(std::memory_order_relaxed);
}

float MagicMaster::truePeakDb() const
{
    return m_meterTruePeakDb.load(std::memory_order_relaxed);
}

float MagicMaster::truePeakHoldDb() const
{
    return m_meterTruePeakHoldDb.load(std::memory_order_relaxed);
}

float MagicMaster::limiterBoostDb() const
{
    return m_meterLimiterBoostDb.load(std::memory_order_relaxed);
}

bool MagicMaster::isCalibrated() const
{
    return m_meterIsCalibrated.load(std::memory_order_relaxed);
}

std::string MagicMaster::songContextSummary() const
{
    std::lock_guard<std::mutex> lock { m_stateMutex };
    return m_pendingSongContext.summaryText;
}

float MagicMaster::subHpfHz() const
{
    return m_meterSubHpfHz.load(std::memory_order_relaxed);
}

float MagicMaster::monoBassHz() const
{
    return m_meterMonoBassHz.load(std::memory_order_relaxed);
}

float MagicMaster::autoEqMaxMoveDb() const
{
    return m_meterAutoEqMaxMoveDb.load(std::memory_order_relaxed);
}

float MagicMaster::autoEqBandDb(int band) const
{
    if (band < 0 || static_cast<size_t>(band) >= EqBandCount) {
        return 0.0f;
    }
    return m_meterAutoEqBandDb[static_cast<size_t>(band)].load(std::memory_order_relaxed);
}

float MagicMaster::resonanceCutDb() const
{
    return m_meterResonanceCutDb.load(std::memory_order_relaxed);
}

float MagicMaster::multibandReductionDb(int band) const
{
    if (band < 0 || static_cast<size_t>(band) >= MbBandCount) {
        return 0.0f;
    }
    return m_meterMbReductionDb[static_cast<size_t>(band)].load(std::memory_order_relaxed);
}

float MagicMaster::stereoCorrelation() const
{
    return m_meterStereoCorr.load(std::memory_order_relaxed);
}

float MagicMaster::stereoWidthPercent() const
{
    return m_meterStereoWidthPct.load(std::memory_order_relaxed);
}

float MagicMaster::clipperReductionDb() const
{
    return m_meterClipperReductionDb.load(std::memory_order_relaxed);
}

float MagicMaster::limiterReductionDb() const
{
    return m_meterLimiterReductionDb.load(std::memory_order_relaxed);
}

float MagicMaster::thresholdPosition(double db)
{
    return static_cast<float>(std::clamp((db - MinThresholdDb) / (MaxThresholdDb - MinThresholdDb), 0.0, 1.0));
}

float MagicMaster::ceilingPosition(double db)
{
    return static_cast<float>(std::clamp((db - MinCeilingDb) / (MaxCeilingDb - MinCeilingDb), 0.0, 1.0));
}

} // namespace noteahead
