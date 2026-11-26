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

#ifndef MAGIC_MASTER_HPP
#define MAGIC_MASTER_HPP

#include "../../../domain/dsp/auto_gain.hpp"
#include "../../../domain/dsp/compressor_core.hpp"
#include "../../../domain/dsp/dc_blocker.hpp"
#include "../../../domain/dsp/linkwitz_riley_crossover.hpp"
#include "../../../domain/dsp/spectral_peak_detector.hpp"
#include "../../../domain/dsp/svf_filter.hpp"
#include "../../../domain/dsp/tracking_bell_bank.hpp"
#include "../../../domain/dsp/true_peak_limiter_core.hpp"
#include "../../../domain/dsp/upsampler.hpp"
#include "../../../domain/effects/effect.hpp"
#include "../../../domain/effects/effect_presets.hpp"
#include "../../../domain/effects/loudness_meter_resettable.hpp"
#include "../../../domain/effects/master_calibration.hpp"
#include "../../../domain/effects/song_context_consumer.hpp"
#include "../../../domain/utility/dbtp_meter.hpp"
#include "../../../domain/utility/loudness_meter.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <mutex>
#include <span>
#include <string>
#include <vector>

namespace noteahead {

//! Zero-fiddling, EDM-focused automatic 6-stage mastering rack effect.
class MagicMaster : public Effect,
                    public SongContextConsumer,
                    public LoudnessMeterResettable,
                    public MasterCalibrationConsumer
{
public:
    //! 4x Upsampler(119+43) + 1-sample 4x alignment delay + Decimator(43+119) == 276 4x-samples == 69 base frames.
    static constexpr size_t ClipperOversampleLatencyFrames { 69 };

    MagicMaster();
    ~MagicMaster() override;

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;
    std::vector<std::string> modulationPriority() const override;
    const EffectPresetList & factoryPresets() const override;

    void setSampleRate(double sampleRate) override;
    void setBpm(float bpm) override;
    size_t latencyFrames(double sampleRate) const override;
    bool isSettled() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;

    void setSongContext(const SongContext & context) override;
    SongContext songContext() const;

    void requestResetMeters() override;

    void setCalibration(const MasterCalibration & calibration) override;
    void clearCalibration() override;
    MasterCalibration calibration() const override;
    void setCalibrationBypass(bool bypass) override;
    double thresholdDb() const override;
    double ceilingDb() const override;
    MasterCalibration calibrateFromStereo(std::span<const float> interleavedStereo, uint32_t sampleRate) const override;

    // Telemetry getters for UI & tests
    float shortTermLufs() const;
    float integratedLufs() const;
    float truePeakDb() const;
    float truePeakHoldDb() const;
    float limiterBoostDb() const;
    bool isCalibrated() const;
    std::string songContextSummary() const;

    float subHpfHz() const;
    float monoBassHz() const;
    float autoEqBandDb(int band) const;
    float autoEqMaxMoveDb() const;
    float resonanceCutDb() const;
    float multibandReductionDb(int band) const;
    float stereoCorrelation() const;
    float stereoWidthPercent() const;
    float clipperReductionDb() const;
    float limiterReductionDb() const;

    static float thresholdPosition(double db);
    static float ceilingPosition(double db);

private:
    static constexpr size_t EqBandCount { 5 };
    static constexpr size_t MbBandCount { 3 };

    struct EqBandFilters
    {
        SvfFilter filterL;
        SvfFilter filterR;
        SvfFilter detector;
    };

    struct CorrelationTracker
    {
        double lr { 0.0 };
        double ll { 0.0 };
        double rr { 0.0 };
        double lastCorrelation { 1.0 };

        void update(double left, double right, double coeff);
        void reset();
    };

    void syncParameters();
    void pullPendingSnapshots();
    void updateStageConfigurations(bool force);
    void updateAutoEqCoefficients();
    void updateMultibandThresholds();
    void processSingleFrame(double & left, double & right);
    void processSubCleanStage(double & left, double & right);
    void processAutoEqStage(double & left, double & right);
    void processMultibandStage(double & left, double & right);
    void processStereoAirStage(double & left, double & right);
    void processClipperStage(double & left, double & right);
    double softClip4xSample(double x, double ceilingLin, double kneeLin) const;
    void updateClipperMeter(double peakPre, double peakPost);
    void publishTelemetry();

    // Parameters mirrored locally
    double m_thresholdDb { -6.0 };
    double m_ceilingDb { -1.2 };
    bool m_subCleanEnabled { true };
    bool m_eqEnabled { true };
    bool m_multibandEnabled { true };
    bool m_stereoEnabled { true };
    bool m_clipperEnabled { true };
    bool m_limiterEnabled { true };

    // Thread-safe context & calibration state (preserved across reset())
    mutable std::mutex m_stateMutex;
    SongContext m_pendingSongContext {};
    SongContext m_activeSongContext {};
    std::atomic<bool> m_contextDirty { false };

    MasterCalibration m_pendingCalibration {};
    MasterCalibration m_activeCalibration {};
    std::atomic<bool> m_calibrationDirty { false };
    std::atomic<bool> m_calibrationBypass { false };
    std::atomic<bool> m_resetMetersRequested { false };

    double m_configuredSampleRate { 0.0 };
    double m_configuredSubHpfHz { 0.0 };
    double m_configuredMonoBassHz { 0.0 };
    double m_configuredLowMidHz { 0.0 };
    double m_configuredMidHighHz { 0.0 };
    double m_configuredBpm { 0.0 };
    bool m_configuredBright { false };
    bool m_configuredSub808 { false };
    bool m_configuredSidechain { false };

    // Stage 1: Sub & Phase
    DcBlocker m_dcBlockerL;
    DcBlocker m_dcBlockerR;
    SvfFilter m_subHpf1L;
    SvfFilter m_subHpf2L;
    SvfFilter m_subHpf1R;
    SvfFilter m_subHpf2R;
    LinkwitzRileyCrossover m_monoBassL;
    LinkwitzRileyCrossover m_monoBassR;
    double m_subHpfHz { 28.0 };
    double m_monoBassHz { 125.0 };

    // Stage 2: Auto EQ & Dynamic Resonance Tamer
    std::array<EqBandFilters, EqBandCount> m_eqBands {};
    std::array<double, EqBandCount> m_eqGainDb {};
    std::array<double, EqBandCount> m_eqEnvPower {};
    std::array<double, EqBandCount> m_eqDetectorPinkNorm { 1.0, 1.0, 1.0, 1.0, 1.0 };
    double m_eqDetectorCoeff { 0.0 };
    double m_eqSmoothCoeff { 0.0 };
    double m_eqFastPower { 0.0 };
    double m_eqFastCoeff { 0.0 };
    size_t m_eqUpdateCounter { 0 };
    AutoGain m_eqAutoGain;
    SpectralPeakDetector m_peakDetector;
    TrackingBellBank m_resonanceBank;

    // Stage 3: Multiband Glue
    LinkwitzRileyCrossover m_mbLowSplitL;
    LinkwitzRileyCrossover m_mbHighSplitL;
    LinkwitzRileyCrossover m_mbLowAllPassL;
    LinkwitzRileyCrossover m_mbLowSplitR;
    LinkwitzRileyCrossover m_mbHighSplitR;
    LinkwitzRileyCrossover m_mbLowAllPassR;
    std::array<CompressorCore, MbBandCount> m_mbComps {};
    std::array<double, MbBandCount> m_mbBandPower {};
    std::array<double, MbBandCount> m_mbThresholdsDb { -18.0, -18.0, -20.0 };
    std::array<double, MbBandCount> m_mbMakeupsLin { 1.0, 1.0, 1.0 };
    double m_mbRmsCoeff { 0.0 };
    size_t m_mbThresholdCounter { 0 };

    // Stage 4: Stereo & Air
    LinkwitzRileyCrossover m_stLowSplitL;
    LinkwitzRileyCrossover m_stHighSplitL;
    LinkwitzRileyCrossover m_stLowAllPassL;
    LinkwitzRileyCrossover m_stLowSplitR;
    LinkwitzRileyCrossover m_stHighSplitR;
    LinkwitzRileyCrossover m_stLowAllPassR;
    CorrelationTracker m_corrMid;
    CorrelationTracker m_corrHigh;
    double m_corrCoeff { 0.0 };
    double m_currentMidWidth { 1.0 };
    double m_currentHighWidth { 1.0 };
    double m_widthSmoothCoeff { 0.0 };

    SvfFilter m_sheenPreHpfL;
    SvfFilter m_sheenPreHpfR;
    SvfFilter m_sheenPostHpfL;
    SvfFilter m_sheenPostHpfR;
    Upsampler2x m_sheenUpL;
    Upsampler2x m_sheenUpR;
    Decimator2x m_sheenDownL;
    Decimator2x m_sheenDownR;
    double m_sheenHfEnv { 0.0 };
    double m_sheenEnvCoeff { 0.0 };

    // Stage 6: Fixed 4x Soft Clipper (with 1-sample 4x alignment -> 69 base frames latency)
    Upsampler m_clipUpL;
    Upsampler m_clipUpR;
    Decimator m_clipDownL;
    Decimator m_clipDownR;
    float m_clipPrevL { 0.0f };
    float m_clipPrevR { 0.0f };
    double m_clipReductionDb { 0.0 };
    double m_clipMeterReleaseCoeff { 0.0 };

    // Stage 7: True-Peak Limiter Core
    TruePeakLimiterCore m_limiter;

    // Output metering & settle tracking
    LoudnessMeter m_outputLoudness;
    DbTpMeter m_outputTruePeak;
    size_t m_quietFrames { 0 };

    // Lock-free UI telemetry
    std::atomic<float> m_meterShortTermLufs { -70.0f };
    std::atomic<float> m_meterIntegratedLufs { -70.0f };
    std::atomic<float> m_meterTruePeakDb { -70.0f };
    std::atomic<float> m_meterTruePeakHoldDb { -70.0f };
    std::atomic<float> m_meterLimiterBoostDb { 4.8f };
    std::atomic<bool> m_meterIsCalibrated { false };

    std::atomic<float> m_meterSubHpfHz { 28.0f };
    std::atomic<float> m_meterMonoBassHz { 125.0f };
    std::atomic<float> m_meterAutoEqMaxMoveDb { 0.0f };
    std::array<std::atomic<float>, EqBandCount> m_meterAutoEqBandDb {};
    std::atomic<float> m_meterResonanceCutDb { 0.0f };
    std::array<std::atomic<float>, MbBandCount> m_meterMbReductionDb {};
    std::atomic<float> m_meterStereoCorr { 1.0f };
    std::atomic<float> m_meterStereoWidthPct { 100.0f };
    std::atomic<float> m_meterClipperReductionDb { 0.0f };
    std::atomic<float> m_meterLimiterReductionDb { 0.0f };
};

} // namespace noteahead

#endif // MAGIC_MASTER_HPP
