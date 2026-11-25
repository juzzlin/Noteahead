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

#ifndef ANALOG_BUS_HPP
#define ANALOG_BUS_HPP

#include "../../../domain/dsp/analog_drift.hpp"
#include "../../../domain/dsp/auto_gain.hpp"
#include "../../../domain/dsp/dc_blocker.hpp"
#include "../../../domain/dsp/noise_source.hpp"
#include "../../../domain/dsp/one_pole_filter.hpp"
#include "../../../domain/dsp/svf_filter.hpp"
#include "../../../domain/effects/effect.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace noteahead {

//! Stereo console coloration, tone-shaping, and spatial enhancement processor.
//!
//! Models a discrete two-topology analog console bus:
//! - Tight high-pass filter (Off, 25 Hz resonant Mix bump, 50 Hz Butterworth Track cut)
//! - Two distinct discrete console topologies with selectable series ordering (Off, A, N, A -> N, N -> A):
//!   * A: Discrete American class-A op-amp + steel transformer punch (odd-dominant clarity and transient bite)
//!   * N: Discrete British class-A amplifier + iron-core LF transformer flux (even harmonics and low-mid weight)
//! - Expandable Color module slot (Off, Bus 4000, Tape, Vinyl, Lo-Fi 12) with dedicated Drive and parallel Color Mix
//! - Three-band musical Tone EQ (Low Baxandall shelf, High Baxandall shelf, and Air shelf) switchable Pre or Post saturation
//! - Vintage high-frequency transformer roll-off, deterministic Analog L/R component drift, and elliptical Mid/Side Aspect width
//!
//! Nonlinear console and color stages run oversampled when requested under MixLaw::Internal, using a
//! latency-matched dry path so parallel Mix remains phase-coherent at 1x, 2x, and 4x factors.
//!
//! Auto Gain measures what the console and color stages do to the level, by playing pink noise at
//! CalibrationLevelDb through a private copy of them whenever their settings change, and takes the
//! difference back out of the wet signal. The Tone EQ, Tight HPF, Vintage, Aspect and Output are
//! left to change the level as they are set to.
class AnalogBus : public Effect
{
public:
    //! Persisted by index: append only.
    enum class LowCutMode
    {
        Off,
        Mix25Hz,
        Track50Hz
    };

    //! Persisted by index: append only.
    enum class Routing
    {
        Off,
        A,
        N,
        AToN,
        NToA
    };

    //! Persisted by index: append only.
    enum class ColorMode
    {
        Off,
        Bus4000,
        Tape,
        Vinyl,
        LoFi12
    };

    //! Persisted by index: append only.
    enum class LowFreq
    {
        Sub1_30Hz,
        Sub2_45Hz,
        Bass_65Hz,
        Body_110Hz
    };

    //! Persisted by index: append only.
    enum class HighFreq
    {
        Mids_2k5Hz,
        Presence_4k5Hz,
        Brite_8kHz,
        Sheen_14kHz
    };

    static constexpr double MaxConsoleDriveDb { 36.0 };
    static constexpr double MaxEqGainDb { 12.0 };
    static constexpr double MaxAirGainDb { 9.0 };
    static constexpr double MaxOutputDb { 12.0 };
    //! The RMS level of the pink noise Auto Gain measures with, near what devices play at.
    static constexpr double CalibrationLevelDb { -20.0 };
    static constexpr double MaxAutoGainDb { 30.0 };
    static constexpr std::array<double, 4> LowFreqsHz { 30.0, 45.0, 65.0, 110.0 };
    static constexpr std::array<double, 4> HighFreqsHz { 2500.0, 4500.0, 8000.0, 14000.0 };

    AnalogBus();
    ~AnalogBus() override;

private:
    struct CalibrationTag
    {
    };

public:
    //! The private copy Auto Gain measures with: no copy of its own.
    explicit AnalogBus(CalibrationTag);

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;
    std::vector<std::string> modulationPriority() const override;

    void processSample(double & left, double & right) override;
    void reset() override;
    void sync() override;

    const EffectPresetList & factoryPresets() const override;

    //! How hard the nonlinear console/color stages are compressing, in dB (0 to negative).
    float saturationDb() const;
    //! The gain Auto Gain is applying, in dB; 0 when it is off.
    double autoGainDb() const;

    static double driveDb(float position);
    static float drivePosition(double db);
    static double eqGainDb(float position);
    static float eqGainPosition(double db);
    static double airGainDb(float position);
    static float airGainPosition(double db);
    static double outputGainDb(float position);
    static float outputGainPosition(double db);
    static std::string routingLabel(Routing routing);

private:
    void syncParameters();
    void updateFilters(double baseSampleRate, double stageSampleRate);
    void processToneEq(double & left, double & right);
    void processStageFrame(double & left, double & right, double stageSampleRate, double driveScaleL, double driveScaleR, double & peakPre, double & peakPost);
    double processStageA(double sample, double driveLin, double & peakPre, double & peakPost);
    double processStageN(double sample, double driveLin, size_t channel, double & peakPre, double & peakPost);
    void processColorFrame(double & left, double & right, double stageSampleRate, double & peakPre, double & peakPost);

    void startCalibration(double sampleRate);
    //! Plays @p frames more of the noise through the calibrator. True once the measurement is done.
    bool stepCalibration(size_t frames);
    void finishCalibration(double sampleRate);

    LowCutMode m_lowCutMode { LowCutMode::Off };
    Routing m_routing { Routing::AToN };
    double m_driveA { 0.25 };
    double m_driveN { 0.25 };
    ColorMode m_colorMode { ColorMode::Off };
    double m_colorDrive { 0.35 };
    double m_colorMix { 1.0 };
    bool m_eqEnabled { true };
    bool m_eqPre { false };
    double m_lowGainDb { 0.0 };
    LowFreq m_lowFreq { LowFreq::Sub2_45Hz };
    double m_highGainDb { 0.0 };
    HighFreq m_highFreq { HighFreq::Brite_8kHz };
    double m_airGainDb { 0.0 };
    bool m_vintage { false };
    bool m_analog { false };
    double m_aspect { 0.0 };
    double m_outputDb { 0.0 };
    bool m_autoGainEnabled { false };

    SvfFilter m_tightHpL;
    SvfFilter m_tightHpR;

    SvfFilter m_eqLowL;
    SvfFilter m_eqLowR;
    SvfFilter m_eqHighL;
    SvfFilter m_eqHighR;
    SvfFilter m_eqAirL;
    SvfFilter m_eqAirR;

    std::array<OnePoleFilter, 2> m_stageNLfFilter;
    std::array<OnePoleFilter, 2> m_stageNSilkFilter;

    SvfFilter m_colorLowL;
    SvfFilter m_colorLowR;
    SvfFilter m_colorMidL;
    SvfFilter m_colorMidR;
    SvfFilter m_vinylSideHp;
    std::array<OnePoleFilter, 2> m_colorLpFilter;
    std::array<double, 2> m_loFiHoldSample { 0.0, 0.0 };
    double m_loFiPhase { 0.0 };

    std::array<OnePoleFilter, 2> m_vintageFilter;
    SvfFilter m_aspectSideHp;
    SvfFilter m_aspectSideShelf;

    DcBlocker m_dcBlockerL;
    DcBlocker m_dcBlockerR;

    AnalogDrift m_driftL;
    AnalogDrift m_driftR;

    double m_saturationDb { 0.0 };
    double m_meterReleaseCoeff { 0.0 };
    double m_lastBaseSampleRate { -1.0 };
    double m_lastStageSampleRate { -1.0 };
    bool m_filtersDirty { true };
    bool m_shouldSyncParameters { true };

    struct Oversampling;
    std::unique_ptr<Oversampling> m_oversampling;

    //! What the console and color stages were last measured at
    struct SaturationSettings
    {
        Routing routing { Routing::Off };
        double driveA { 0.0 };
        double driveN { 0.0 };
        ColorMode colorMode { ColorMode::Off };
        double colorDrive { 0.0 };
        double colorMix { 0.0 };
        double sampleRate { 0.0 };

        bool operator==(const SaturationSettings &) const = default;
    };

    SaturationSettings saturationSettings(double sampleRate) const;

    std::unique_ptr<AnalogBus> m_calibrator;
    std::array<NoiseSource, 2> m_calibrationNoise { NoiseSource { 1 }, NoiseSource { 2 } };
    //! The noise without what the effect's own DC blockers would take out, which is not saturation
    std::array<DcBlocker, 2> m_calibrationDcBlockers;
    std::optional<SaturationSettings> m_measuredSettings;
    SaturationSettings m_calibratingSettings;
    bool m_calibrating { false };
    size_t m_calibrationFrame { 0 };
    double m_calibrationNoiseSampleRate { 0.0 };
    size_t m_calibrationWarmUpFrames { 0 };
    size_t m_calibrationFrames { 0 };
    double m_calibrationInPower { 0.0 };
    double m_calibrationOutPower { 0.0 };
    double m_measuredGainDb { 0.0 };
    AutoGain m_autoGain;
    std::atomic<double> m_appliedAutoGainDb { 0.0 };
};

} // namespace noteahead

#endif // ANALOG_BUS_HPP
