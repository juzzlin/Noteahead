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

#include "analog_bus.hpp"
#include "analog_bus_presets.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/utils.hpp"
#include "../../../domain/dsp/audio_context.hpp"
#include "../../../domain/dsp/upsampler.hpp"

#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

constexpr double StageReferenceLevel { 0.32 };
constexpr double MeterReleaseMs { 100.0 };
constexpr double SilentDbThreshold { 0.01 };
//! Auto Gain lets the stages' filters settle on the noise for this long before it measures...
constexpr double CalibrationWarmUpSeconds { 0.02 };
//! ...and measures for this long, long enough for the bass to be heard through the N transformer.
constexpr double CalibrationSeconds { 0.17 };
//! NoiseSource plays every colour at the RMS of uniform white noise, 1/sqrt(3).
constexpr double NoiseSourceRms { 0.57735026918962576 };

std::string key(const QString & name)
{
    return name.toStdString();
}

double consoleCurveA(double u)
{
    // Discrete American op-amp + steel transformer: punchy odd-dominant saturation with subtle 2nd harmonic
    return std::tanh(u + 0.06 * u * u);
}

double consoleCurveN(double u)
{
    // Discrete British Class-A + iron core: richer asymmetric even-harmonic warmth
    return std::tanh(u + 0.14 * u * u);
}

} // namespace

struct AnalogBus::Oversampling
{
    Upsampler upsamplerL;
    Upsampler upsamplerR;
    Decimator decimatorL;
    Decimator decimatorR;

    //! Raw dry carried through an identical upsample + decimate chain so parallel Mix never combs,
    //! even when Tight HPF or Pre-EQ alters the wet path ahead of upsampling.
    Upsampler dryUpsamplerL;
    Upsampler dryUpsamplerR;
    Decimator dryDecimatorL;
    Decimator dryDecimatorR;
};

AnalogBus::AnalogBus()
  : AnalogBus { CalibrationTag {} }
{
    m_calibrator = std::make_unique<AnalogBus>(CalibrationTag {});
}

AnalogBus::AnalogBus(CalibrationTag)
  : m_oversampling { std::make_unique<Oversampling>() }
{
    namespace C = Constants::NahdXml;
    using Display = ParameterDisplay;

    addParameter(Parameter { key(C::xmlKeyLowCutMode()), 0.0f, 0, static_cast<int>(LowCutMode::Track50Hz), 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { key(C::xmlKeyRouting()), static_cast<float>(Routing::AToN), 0, static_cast<int>(Routing::NToA), static_cast<int>(Routing::AToN), 1, Parameter::Type::Discrete });
    addParameter(Parameter { key(C::xmlKeyDriveA()), 0.25f, 0, 3600, 900, 100, Parameter::Type::Continuous });
    addParameter(Parameter { key(C::xmlKeyDriveN()), 0.25f, 0, 3600, 900, 100, Parameter::Type::Continuous });
    addParameter(Parameter { key(C::xmlKeyColorMode()), 0.0f, 0, static_cast<int>(ColorMode::LoFi12), 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { key(C::xmlKeyColorDrive()), 0.35f, 0, 10000, 3500, 100, Parameter::Type::Continuous });
    addParameter(Parameter { key(C::xmlKeyColorMix()), 1.0f, 0, 10000, 10000, 100, Parameter::Type::Continuous });
    addParameter(Parameter { key(C::xmlKeyEqEnabled()), 1.0f, 0, 1, 1, 1, Parameter::Type::Boolean });
    addParameter(Parameter { key(C::xmlKeyEqPre()), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { key(C::xmlKeyLowGain()), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { key(C::xmlKeyLowFreq()), static_cast<float>(LowFreq::Sub2_45Hz), 0, static_cast<int>(LowFreq::Body_110Hz), static_cast<int>(LowFreq::Sub2_45Hz), 1, Parameter::Type::Discrete });
    addParameter(Parameter { key(C::xmlKeyHighGain()), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { key(C::xmlKeyHighFreq()), static_cast<float>(HighFreq::Brite_8kHz), 0, static_cast<int>(HighFreq::Sheen_14kHz), static_cast<int>(HighFreq::Brite_8kHz), 1, Parameter::Type::Discrete });
    addParameter(Parameter { key(C::xmlKeyAirGain()), 0.5f, -900, 900, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { key(C::xmlKeyVintage()), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { key(C::xmlKeyAnalog()), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addParameter(Parameter { key(C::xmlKeyAspect()), 0.0f, 0, 10000, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { key(C::xmlKeyGain()), 0.5f, -1200, 1200, 0, 100, Parameter::Type::Continuous });
    addParameter(Parameter { key(C::xmlKeyAutoGain()), 0.0f, 0, 1, 0, 1, Parameter::Type::Boolean });
    addMixParameter(1.0f, MixLaw::Internal);

    setParameterDisplay(key(C::xmlKeyDriveA()), { .mapping = Display::Mapping::Linear, .unit = "dB", .min = 0.0, .max = MaxConsoleDriveDb, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "A Drive") });
    setParameterDisplay(key(C::xmlKeyDriveN()), { .mapping = Display::Mapping::Linear, .unit = "dB", .min = 0.0, .max = MaxConsoleDriveDb, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "N Drive") });
    setParameterDisplay(key(C::xmlKeyColorDrive()), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Color Drive") });
    setParameterDisplay(key(C::xmlKeyColorMix()), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Color Mix") });
    setParameterDisplay(key(C::xmlKeyLowGain()), { .mapping = Display::Mapping::Linear, .unit = "dB", .min = -MaxEqGainDb, .max = MaxEqGainDb, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Low") });
    setParameterDisplay(key(C::xmlKeyHighGain()), { .mapping = Display::Mapping::Linear, .unit = "dB", .min = -MaxEqGainDb, .max = MaxEqGainDb, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "High") });
    setParameterDisplay(key(C::xmlKeyAirGain()), { .mapping = Display::Mapping::Linear, .unit = "dB", .min = -MaxAirGainDb, .max = MaxAirGainDb, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Air") });
    setParameterDisplay(key(C::xmlKeyAspect()), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Aspect") });
    setParameterDisplay(key(C::xmlKeyGain()), { .mapping = Display::Mapping::Linear, .unit = "dB", .min = -MaxOutputDb, .max = MaxOutputDb, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Output") });
    setParameterDisplay(key(C::xmlKeyMix()), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Mix") });

    m_driftL.initialize(0);
    m_driftR.initialize(1);

    m_tolDriveL = 1.0 + 0.04 * AnalogDrift::tolerance(0, 0);
    m_tolDriveR = 1.0 + 0.04 * AnalogDrift::tolerance(1, 0);
    m_tolGainL = 1.0 + 0.015 * AnalogDrift::tolerance(0, 1);
    m_tolGainR = 1.0 + 0.015 * AnalogDrift::tolerance(1, 1);

    syncParameters();
}

AnalogBus::~AnalogBus() = default;

std::string AnalogBus::typeIdString()
{
    return Constants::RackEffectType::analogBus().toStdString();
}

std::string AnalogBus::type() const
{
    return Constants::RackEffectType::analogBus().toStdString();
}

std::string AnalogBus::typeId() const
{
    return typeIdString();
}

double AnalogBus::driveDb(float position)
{
    return std::clamp(static_cast<double>(position), 0.0, 1.0) * MaxConsoleDriveDb;
}

float AnalogBus::drivePosition(double db)
{
    return static_cast<float>(std::clamp(db / MaxConsoleDriveDb, 0.0, 1.0));
}

double AnalogBus::eqGainDb(float position)
{
    return (std::clamp(static_cast<double>(position), 0.0, 1.0) * 2.0 - 1.0) * MaxEqGainDb;
}

float AnalogBus::eqGainPosition(double db)
{
    return static_cast<float>(std::clamp((db / MaxEqGainDb + 1.0) * 0.5, 0.0, 1.0));
}

double AnalogBus::airGainDb(float position)
{
    return (std::clamp(static_cast<double>(position), 0.0, 1.0) * 2.0 - 1.0) * MaxAirGainDb;
}

float AnalogBus::airGainPosition(double db)
{
    return static_cast<float>(std::clamp((db / MaxAirGainDb + 1.0) * 0.5, 0.0, 1.0));
}

double AnalogBus::outputGainDb(float position)
{
    return (std::clamp(static_cast<double>(position), 0.0, 1.0) * 2.0 - 1.0) * MaxOutputDb;
}

float AnalogBus::outputGainPosition(double db)
{
    return static_cast<float>(std::clamp((db / MaxOutputDb + 1.0) * 0.5, 0.0, 1.0));
}

std::string AnalogBus::routingLabel(Routing routing)
{
    switch (routing) {
    case Routing::Off:
        return "Off";
    case Routing::A:
        return "A";
    case Routing::N:
        return "N";
    case Routing::AToN:
        return "A->N";
    case Routing::NToA:
        return "N->A";
    }
    return "A->N";
}

void AnalogBus::syncParameters()
{
    namespace C = Constants::NahdXml;
    const auto valueOf = [this](const QString & name) {
        const auto p = parameter(key(name));
        return p ? p->get().value() : 0.0f;
    };

    const auto nextLowCut = static_cast<LowCutMode>(std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyLowCutMode()))), 0, static_cast<int>(LowCutMode::Track50Hz)));
    const auto nextColorMode = static_cast<ColorMode>(std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyColorMode()))), 0, static_cast<int>(ColorMode::LoFi12)));
    const auto nextLowFreq = static_cast<LowFreq>(std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyLowFreq()))), 0, static_cast<int>(LowFreq::Body_110Hz)));
    const auto nextHighFreq = static_cast<HighFreq>(std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyHighFreq()))), 0, static_cast<int>(HighFreq::Sheen_14kHz)));

    if (nextColorMode != m_colorMode) {
        m_colorLowL.reset();
        m_colorLowR.reset();
        m_colorMidL.reset();
        m_colorMidR.reset();
        m_vinylSideHp.reset();
        for (auto & lp : m_colorLpFilter) {
            lp.reset();
        }
        m_loFiHoldSample = { 0.0, 0.0 };
        m_loFiPhase = 0.0;
    }

    m_lowCutMode = nextLowCut;
    m_routing = static_cast<Routing>(std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyRouting()))), 0, static_cast<int>(Routing::NToA)));
    m_driveA = std::clamp(static_cast<double>(valueOf(C::xmlKeyDriveA())), 0.0, 1.0);
    m_driveN = std::clamp(static_cast<double>(valueOf(C::xmlKeyDriveN())), 0.0, 1.0);
    m_colorMode = nextColorMode;
    m_colorDrive = std::clamp(static_cast<double>(valueOf(C::xmlKeyColorDrive())), 0.0, 1.0);
    m_colorMix = std::clamp(static_cast<double>(valueOf(C::xmlKeyColorMix())), 0.0, 1.0);
    m_eqEnabled = valueOf(C::xmlKeyEqEnabled()) > 0.5f;
    m_eqPre = valueOf(C::xmlKeyEqPre()) > 0.5f;
    m_lowGainDb = eqGainDb(valueOf(C::xmlKeyLowGain()));
    m_lowFreq = nextLowFreq;
    m_highGainDb = eqGainDb(valueOf(C::xmlKeyHighGain()));
    m_highFreq = nextHighFreq;
    m_airGainDb = airGainDb(valueOf(C::xmlKeyAirGain()));
    m_vintage = valueOf(C::xmlKeyVintage()) > 0.5f;
    m_analog = valueOf(C::xmlKeyAnalog()) > 0.5f;
    const double nextAspect = std::clamp(static_cast<double>(valueOf(C::xmlKeyAspect())), 0.0, 1.0);
    if (nextAspect > 0.0 && m_aspect == 0.0) {
        m_aspectSideHp.reset();
        m_aspectSideShelf.reset();
    }
    m_aspect = nextAspect;
    m_outputDb = outputGainDb(valueOf(C::xmlKeyGain()));
    m_autoGainEnabled = m_calibrator && valueOf(C::xmlKeyAutoGain()) > 0.5f;

    // Back to the last measurement if it still holds, or to unity until the next one is done
    const double sampleRate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    const bool measured = m_measuredSettings == saturationSettings(sampleRate);
    const double targetDb = m_autoGainEnabled && measured ? m_measuredGainDb : 0.0;
    if (!m_autoGainEnabled) {
        m_calibrating = false;
    }
    m_autoGain.setTargetDb(targetDb, sampleRate);
    m_appliedAutoGainDb = targetDb;

    m_filtersDirty = true;
    updateCachedGains();
}

AnalogBus::SaturationSettings AnalogBus::saturationSettings(double sampleRate) const
{
    return { m_routing, m_driveA, m_driveN, m_colorMode, m_colorDrive, m_colorMix, sampleRate };
}

void AnalogBus::startCalibration(double sampleRate)
{
    m_calibratingSettings = saturationSettings(sampleRate);

    // Only the console and color stages, as they are set here, with everything around them flat
    auto && calibrator = *m_calibrator;
    calibrator.m_shouldSyncParameters = false;
    calibrator.m_lowCutMode = LowCutMode::Off;
    calibrator.m_routing = m_routing;
    calibrator.m_driveA = m_driveA;
    calibrator.m_driveN = m_driveN;
    calibrator.m_colorMode = m_colorMode;
    calibrator.m_colorDrive = m_colorDrive;
    calibrator.m_colorMix = m_colorMix;
    calibrator.m_eqEnabled = false;
    calibrator.m_eqPre = false;
    calibrator.m_vintage = false;
    calibrator.m_analog = false;
    calibrator.m_aspect = 0.0;
    calibrator.m_outputDb = 0.0;
    calibrator.m_autoGainEnabled = false;
    calibrator.setSampleRate(sampleRate);
    calibrator.reset();
    calibrator.m_filtersDirty = true;

    // The same noise every time, so that the same settings always measure the same
    for (auto && noise : m_calibrationNoise) {
        if (m_calibrationNoiseSampleRate != sampleRate) {
            noise.setSampleRate(sampleRate);
            noise.setColour(1.0 / 3.0);
        }
        noise.reset();
    }
    for (auto && dcBlocker : m_calibrationDcBlockers) {
        dcBlocker.setSampleRate(sampleRate);
        dcBlocker.reset();
    }
    m_calibrationNoiseSampleRate = sampleRate;

    m_calibrationWarmUpFrames = static_cast<size_t>(std::lround(CalibrationWarmUpSeconds * sampleRate));
    m_calibrationFrames = static_cast<size_t>(std::lround(CalibrationSeconds * sampleRate));
    m_calibrationFrame = 0;
    m_calibrationInPower = 0.0;
    m_calibrationOutPower = 0.0;
    m_calibrating = true;
}

bool AnalogBus::stepCalibration(size_t frames)
{
    const double scale = std::pow(10.0, CalibrationLevelDb / 20.0) / NoiseSourceRms;
    const size_t end = m_calibrationWarmUpFrames + m_calibrationFrames;
    for (size_t i = 0; i < frames && m_calibrationFrame < end; i++, m_calibrationFrame++) {
        const double inL = m_calibrationDcBlockers[0].process(m_calibrationNoise[0].nextSample() * scale);
        const double inR = m_calibrationDcBlockers[1].process(m_calibrationNoise[1].nextSample() * scale);
        double outL = inL;
        double outR = inR;
        m_calibrator->processSample(outL, outR);
        if (m_calibrationFrame >= m_calibrationWarmUpFrames) {
            m_calibrationInPower += inL * inL + inR * inR;
            m_calibrationOutPower += outL * outL + outR * outR;
        }
    }
    return m_calibrationFrame >= end;
}

void AnalogBus::finishCalibration(double sampleRate)
{
    const double gainDb = m_calibrationInPower > 0.0 && m_calibrationOutPower > 0.0 ? 10.0 * std::log10(m_calibrationInPower / m_calibrationOutPower) : 0.0;
    m_measuredGainDb = std::clamp(gainDb, -MaxAutoGainDb, MaxAutoGainDb);
    m_measuredSettings = m_calibratingSettings;
    m_calibrating = false;
    if (m_autoGainEnabled) {
        m_autoGain.setTargetDb(m_measuredGainDb, sampleRate);
        m_appliedAutoGainDb = m_measuredGainDb;
    }
}

void AnalogBus::updateFilters(double baseSampleRate, double stageSampleRate)
{
    // Tight HPF
    switch (m_lowCutMode) {
    case LowCutMode::Off:
        m_tightHpL.setBypass();
        m_tightHpR.setBypass();
        break;
    case LowCutMode::Mix25Hz:
        // Slight under-damped Q creates the subtle sub-bass contour bump characteristic of mix high-pass filters
        m_tightHpL.calculateLowCut(25.0, baseSampleRate, 0.88);
        m_tightHpR.calculateLowCut(25.0, baseSampleRate, 0.88);
        break;
    case LowCutMode::Track50Hz:
        m_tightHpL.calculateLowCut(50.0, baseSampleRate, std::numbers::sqrt2 * 0.5);
        m_tightHpR.calculateLowCut(50.0, baseSampleRate, std::numbers::sqrt2 * 0.5);
        break;
    }

    // Tone EQ (wide Baxandall Q = 0.55 for Low/High, soft 0.50 for Air)
    if (!m_eqEnabled || std::abs(m_lowGainDb) <= SilentDbThreshold) {
        m_eqLowL.setBypass();
        m_eqLowR.setBypass();
    } else {
        const double lowHz = LowFreqsHz.at(static_cast<size_t>(m_lowFreq));
        m_eqLowL.calculateLowShelf(lowHz, baseSampleRate, 0.55, m_lowGainDb);
        m_eqLowR.calculateLowShelf(lowHz, baseSampleRate, 0.55, m_lowGainDb);
    }

    if (!m_eqEnabled || std::abs(m_highGainDb) <= SilentDbThreshold) {
        m_eqHighL.setBypass();
        m_eqHighR.setBypass();
    } else {
        const double highHz = HighFreqsHz.at(static_cast<size_t>(m_highFreq));
        m_eqHighL.calculateHighShelf(highHz, baseSampleRate, 0.55, m_highGainDb);
        m_eqHighR.calculateHighShelf(highHz, baseSampleRate, 0.55, m_highGainDb);
    }

    if (!m_eqEnabled || std::abs(m_airGainDb) <= SilentDbThreshold) {
        m_eqAirL.setBypass();
        m_eqAirR.setBypass();
    } else {
        m_eqAirL.calculateHighShelf(12000.0, baseSampleRate, 0.45, m_airGainDb);
        m_eqAirR.calculateHighShelf(12000.0, baseSampleRate, 0.45, m_airGainDb);
    }

    // Vintage 6 dB/oct roll-off & Aspect elliptical M/S enhancement
    for (auto & vf : m_vintageFilter) {
        vf.calculate(15500.0, baseSampleRate);
    }
    m_aspectSideHp.calculateLowCut(130.0, baseSampleRate, std::numbers::sqrt2 * 0.5);
    m_aspectSideShelf.calculateHighShelf(2200.0, baseSampleRate, 0.65, 2.5);

    // Stage N transformer LF split + silky top filter at oversampled rate
    for (size_t ch = 0; ch < 2; ch++) {
        m_stageNLfFilter[ch].calculate(220.0, stageSampleRate);
        m_stageNSilkFilter[ch].calculate(16000.0, stageSampleRate);
    }

    // Color module filters at oversampled rate
    switch (m_colorMode) {
    case ColorMode::Off:
        m_colorLowL.setBypass();
        m_colorLowR.setBypass();
        m_colorMidL.setBypass();
        m_colorMidR.setBypass();
        m_vinylSideHp.setBypass();
        break;
    case ColorMode::Bus4000:
        m_colorLowL.calculateBell(95.0, stageSampleRate, 0.85, 0.8 * m_colorDrive);
        m_colorLowR.calculateBell(95.0, stageSampleRate, 0.85, 0.8 * m_colorDrive);
        m_colorMidL.calculateBell(3500.0, stageSampleRate, 0.70, 0.9 * m_colorDrive);
        m_colorMidR.calculateBell(3500.0, stageSampleRate, 0.70, 0.9 * m_colorDrive);
        m_vinylSideHp.setBypass();
        for (auto & lp : m_colorLpFilter) {
            lp.calculate(20000.0, stageSampleRate);
        }
        break;
    case ColorMode::Tape:
        m_colorLowL.calculateBell(75.0, stageSampleRate, 1.1, 1.8 * m_colorDrive);
        m_colorLowR.calculateBell(75.0, stageSampleRate, 1.1, 1.8 * m_colorDrive);
        m_colorMidL.setBypass();
        m_colorMidR.setBypass();
        m_vinylSideHp.setBypass();
        for (auto & lp : m_colorLpFilter) {
            lp.calculate(18000.0 - 7000.0 * m_colorDrive, stageSampleRate);
        }
        break;
    case ColorMode::Vinyl:
        m_colorLowL.calculateLowCut(28.0, stageSampleRate, 0.707);
        m_colorLowR.calculateLowCut(28.0, stageSampleRate, 0.707);
        m_colorMidL.calculateBell(450.0, stageSampleRate, 0.65, 0.8 * m_colorDrive);
        m_colorMidR.calculateBell(450.0, stageSampleRate, 0.65, 0.8 * m_colorDrive);
        m_vinylSideHp.calculateLowCut(120.0, stageSampleRate, std::numbers::sqrt2 * 0.5);
        for (auto & lp : m_colorLpFilter) {
            lp.calculate(14000.0 - 4500.0 * m_colorDrive, stageSampleRate);
        }
        break;
    case ColorMode::LoFi12:
        m_colorLowL.calculateBell(140.0, stageSampleRate, 0.75, 1.2 * m_colorDrive);
        m_colorLowR.calculateBell(140.0, stageSampleRate, 0.75, 1.2 * m_colorDrive);
        m_colorMidL.setBypass();
        m_colorMidR.setBypass();
        m_vinylSideHp.setBypass();
        for (auto & lp : m_colorLpFilter) {
            lp.calculate(12000.0 - 3000.0 * m_colorDrive, stageSampleRate);
        }
        break;
    }

    m_loFiPhaseInc = (28000.0 - 20000.0 * m_colorDrive) / stageSampleRate;
    m_dcBlockerL.setSampleRate(baseSampleRate);
    m_dcBlockerR.setSampleRate(baseSampleRate);
    m_meterReleaseCoeff = std::exp(-1.0 / ((MeterReleaseMs / 1000.0) * baseSampleRate));
    m_meterOneMinusReleaseCoeff = 1.0 - m_meterReleaseCoeff;
    m_lastBaseSampleRate = baseSampleRate;
    m_lastStageSampleRate = stageSampleRate;
    m_filtersDirty = false;
    updateCachedGains();
}

void AnalogBus::updateCachedGains()
{
    m_driveLinA = std::pow(10.0, (m_driveA * MaxConsoleDriveDb) / 20.0);
    m_driveLinN = std::pow(10.0, (m_driveN * MaxConsoleDriveDb) / 20.0);
    m_compA = StageReferenceLevel / std::tanh(StageReferenceLevel * m_driveLinA);
    m_compN = StageReferenceLevel / std::tanh(StageReferenceLevel * m_driveLinN);
    m_lfDriveFactor = 1.0 + 0.65 * m_driveN;
    m_lfDriveN = m_driveLinN * m_lfDriveFactor;
    m_lfCompN = StageReferenceLevel / std::tanh(StageReferenceLevel * m_lfDriveN);

    m_stageGainsFlat = {
        m_driveLinA, m_driveLinA, m_compA, m_compA,
        m_driveLinN, m_driveLinN, m_compN, m_compN,
        m_lfDriveN, m_lfDriveN, m_lfCompN, m_lfCompN
    };

    switch (m_colorMode) {
    case ColorMode::Off:
        m_colorDriveLin = 1.0;
        m_colorComp = 1.0;
        break;
    case ColorMode::Bus4000:
        m_colorDriveLin = 1.0 + 4.5 * m_colorDrive;
        m_colorComp = StageReferenceLevel / std::tanh(StageReferenceLevel * m_colorDriveLin);
        break;
    case ColorMode::Tape:
        m_colorDriveLin = 1.0 + 6.0 * m_colorDrive;
        m_colorComp = StageReferenceLevel / std::tanh(StageReferenceLevel * m_colorDriveLin);
        break;
    case ColorMode::Vinyl:
        m_colorDriveLin = 1.0 + 5.0 * m_colorDrive;
        m_colorComp = StageReferenceLevel / std::tanh(StageReferenceLevel * m_colorDriveLin);
        m_vinylBlendFactor = 0.65 * m_colorDrive;
        break;
    case ColorMode::LoFi12:
        m_colorDriveLin = 1.0 + 4.0 * m_colorDrive;
        m_colorComp = StageReferenceLevel / std::tanh(StageReferenceLevel * m_colorDriveLin);
        m_colorLoFiStep = std::pow(2.0, -(12.0 - 4.0 * m_colorDrive));
        break;
    }

    m_outputGain = static_cast<double>(Utils::Dsp::dbToLinear(static_cast<float>(m_outputDb)));
}

void AnalogBus::prepareForAudio(double & baseSampleRate, uint8_t & factor, double & stageSampleRate)
{
    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }

    baseSampleRate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    factor = clampOversampleFactor(oversampleFactor());
    stageSampleRate = baseSampleRate * factor;

    if (m_filtersDirty || baseSampleRate != m_lastBaseSampleRate || stageSampleRate != m_lastStageSampleRate) {
        updateFilters(baseSampleRate, stageSampleRate);
    }
}

void AnalogBus::processToneEq(double & left, double & right)
{
    left = m_eqLowL.process(left);
    right = m_eqLowR.process(right);
    left = m_eqHighL.process(left);
    right = m_eqHighR.process(right);
    left = m_eqAirL.process(left);
    right = m_eqAirR.process(right);
}

double AnalogBus::processStageA(double sample, double driveLin, double comp, double & peakPre, double & peakPost)
{
    const double u = sample * driveLin;
    const double shaped = consoleCurveA(u);
    peakPre = std::max(peakPre, std::abs(u));
    peakPost = std::max(peakPost, std::abs(shaped));

    return shaped * comp;
}

double AnalogBus::processStageN(double sample, double driveLin, double comp, double lfDrive, double lfComp, size_t channel, double & peakPre, double & peakPost)
{
    // Split low frequencies to emulate core flux saturation in an iron output transformer
    m_stageNLfFilter[channel].process(sample);
    const double lf = m_stageNLfFilter[channel].lowPass();
    const double hf = m_stageNLfFilter[channel].highPass();

    const double lfU = lf * lfDrive;
    const double lfShaped = consoleCurveN(lfU) * lfComp;

    const double u = (hf + lfShaped * 0.35 + lf * 0.65) * driveLin;
    const double shaped = consoleCurveN(u);

    peakPre = std::max(peakPre, std::abs(u));
    peakPost = std::max(peakPost, std::abs(shaped));

    const double out = shaped * comp;
    m_stageNSilkFilter[channel].process(out);
    // Subtle 16 kHz top-end smoothing characteristic of discrete Class-A iron output stages
    return m_stageNSilkFilter[channel].lowPass() * 0.25 + out * 0.75;
}

void AnalogBus::processColorFrame(double & left, double & right, double & peakPre, double & peakPost)
{
    if (m_colorMode == ColorMode::Off) {
        return;
    }

    const double preL = left;
    const double preR = right;
    double colL = left;
    double colR = right;

    switch (m_colorMode) {
    case ColorMode::Off:
        return;
    case ColorMode::Bus4000: {
        colL = m_colorLowL.process(colL);
        colR = m_colorLowR.process(colR);
        colL = m_colorMidL.process(colL);
        colR = m_colorMidR.process(colR);

        const double driveLin = m_colorDriveLin;
        const double comp = m_colorComp;
        const double uL = colL * driveLin;
        const double uR = colR * driveLin;
        // Tight VCA / 4000-style predominantly odd compression curve
        const double sL = std::tanh(uL + 0.02 * uL * uL);
        const double sR = std::tanh(uR + 0.02 * uR * uR);
        peakPre = std::max(peakPre, std::max(std::abs(uL), std::abs(uR)));
        peakPost = std::max(peakPost, std::max(std::abs(sL), std::abs(sR)));
        colL = sL * comp;
        colR = sR * comp;
        break;
    }
    case ColorMode::Tape: {
        colL = m_colorLowL.process(colL);
        colR = m_colorLowR.process(colR);

        const double driveLin = m_colorDriveLin;
        const double comp = m_colorComp;
        const double uL = colL * driveLin;
        const double uR = colR * driveLin;
        const double sL = std::tanh(uL + 0.05 * uL * uL);
        const double sR = std::tanh(uR + 0.05 * uR * uR);
        peakPre = std::max(peakPre, std::max(std::abs(uL), std::abs(uR)));
        peakPost = std::max(peakPost, std::max(std::abs(sL), std::abs(sR)));
        m_colorLpFilter[0].process(sL * comp);
        m_colorLpFilter[1].process(sR * comp);
        colL = m_colorLpFilter[0].lowPass();
        colR = m_colorLpFilter[1].lowPass();
        break;
    }
    case ColorMode::Vinyl: {
        // Elliptical mono-bass below 120 Hz on side channel, plus lathe warmth and HF smoothing
        const double mid = 0.5 * (colL + colR);
        const double side = 0.5 * (colL - colR);
        const double sideHp = m_vinylSideHp.process(side);
        const double blendedSide = side * (1.0 - m_vinylBlendFactor) + sideHp * m_vinylBlendFactor;
        colL = m_colorMidL.process(m_colorLowL.process(mid + blendedSide));
        colR = m_colorMidR.process(m_colorLowR.process(mid - blendedSide));

        const double driveLin = m_colorDriveLin;
        const double comp = m_colorComp;
        const double uL = colL * driveLin;
        const double uR = colR * driveLin;
        const double sL = std::tanh(uL + 0.12 * uL * uL);
        const double sR = std::tanh(uR + 0.12 * uR * uR);
        peakPre = std::max(peakPre, std::max(std::abs(uL), std::abs(uR)));
        peakPost = std::max(peakPost, std::max(std::abs(sL), std::abs(sR)));
        m_colorLpFilter[0].process(sL * comp);
        m_colorLpFilter[1].process(sR * comp);
        colL = m_colorLpFilter[0].lowPass();
        colR = m_colorLpFilter[1].lowPass();
        break;
    }
    case ColorMode::LoFi12: {
        colL = m_colorLowL.process(colL);
        colR = m_colorLowR.process(colR);

        const double driveLin = m_colorDriveLin;
        const double comp = m_colorComp;
        const double uL = colL * driveLin;
        const double uR = colR * driveLin;
        const double sL = std::tanh(uL);
        const double sR = std::tanh(uR);
        peakPre = std::max(peakPre, std::max(std::abs(uL), std::abs(uR)));
        peakPost = std::max(peakPost, std::max(std::abs(sL), std::abs(sR)));

        const double step = m_colorLoFiStep;
        const double qL = std::round((sL * comp) / step) * step;
        const double qR = std::round((sR * comp) / step) * step;

        m_loFiPhase += m_loFiPhaseInc;
        if (m_loFiPhase >= 1.0) {
            m_loFiPhase -= std::floor(m_loFiPhase);
            m_loFiHoldSample[0] = qL;
            m_loFiHoldSample[1] = qR;
        }
        m_colorLpFilter[0].process(m_loFiHoldSample[0]);
        m_colorLpFilter[1].process(m_loFiHoldSample[1]);
        colL = m_colorLpFilter[0].lowPass();
        colR = m_colorLpFilter[1].lowPass();
        break;
    }
    }

    left = preL + m_colorMix * (colL - preL);
    right = preR + m_colorMix * (colR - preR);
}

void AnalogBus::processStageFrame(double & left, double & right, const StageGains & gains, double & peakPre, double & peakPost)
{
    switch (m_routing) {
    case Routing::Off:
        break;
    case Routing::A:
        left = processStageA(left, gains.driveAL, gains.compAL, peakPre, peakPost);
        right = processStageA(right, gains.driveAR, gains.compAR, peakPre, peakPost);
        break;
    case Routing::N:
        left = processStageN(left, gains.driveNL, gains.compNL, gains.lfDriveNL, gains.lfCompNL, 0, peakPre, peakPost);
        right = processStageN(right, gains.driveNR, gains.compNR, gains.lfDriveNR, gains.lfCompNR, 1, peakPre, peakPost);
        break;
    case Routing::AToN:
        left = processStageA(left, gains.driveAL, gains.compAL, peakPre, peakPost);
        right = processStageA(right, gains.driveAR, gains.compAR, peakPre, peakPost);
        left = processStageN(left, gains.driveNL, gains.compNL, gains.lfDriveNL, gains.lfCompNL, 0, peakPre, peakPost);
        right = processStageN(right, gains.driveNR, gains.compNR, gains.lfDriveNR, gains.lfCompNR, 1, peakPre, peakPost);
        break;
    case Routing::NToA:
        left = processStageN(left, gains.driveNL, gains.compNL, gains.lfDriveNL, gains.lfCompNL, 0, peakPre, peakPost);
        right = processStageN(right, gains.driveNR, gains.compNR, gains.lfDriveNR, gains.lfCompNR, 1, peakPre, peakPost);
        left = processStageA(left, gains.driveAL, gains.compAL, peakPre, peakPost);
        right = processStageA(right, gains.driveAR, gains.compAR, peakPre, peakPost);
        break;
    }

    processColorFrame(left, right, peakPre, peakPost);
}

void AnalogBus::processFrame(double & left, double & right, double baseSampleRate, uint8_t factor, double stageSampleRate, double mixValue, bool needAlignedDry)
{
    (void)stageSampleRate;
    const double rawDryL = left;
    const double rawDryR = right;
    double alignedDryL = rawDryL;
    double alignedDryR = rawDryR;
    double wetL = rawDryL;
    double wetR = rawDryR;

    // 1. Pre-stage linear shaping
    wetL = m_tightHpL.process(wetL);
    wetR = m_tightHpR.process(wetR);
    if (m_eqPre) {
        processToneEq(wetL, wetR);
    }

    // 2. Deterministic L/R analog hardware component tolerance + slow drift (computed once per base frame)
    StageGains analogGains {};
    const StageGains * gains = &m_stageGainsFlat;
    double trimGainL = 1.0;
    double trimGainR = 1.0;
    if (m_analog) {
        const double driftL = m_driftL.next(baseSampleRate);
        const double driftR = m_driftR.next(baseSampleRate);
        const double driveScaleL = m_tolDriveL + 0.008 * driftL;
        const double driveScaleR = m_tolDriveR + 0.008 * driftR;
        trimGainL = m_tolGainL + 0.003 * driftL;
        trimGainR = m_tolGainR + 0.003 * driftR;

        if (m_routing == Routing::A || m_routing == Routing::AToN || m_routing == Routing::NToA) {
            analogGains.driveAL = m_driveLinA * driveScaleL;
            analogGains.driveAR = m_driveLinA * driveScaleR;
            analogGains.compAL = StageReferenceLevel / std::tanh(StageReferenceLevel * analogGains.driveAL);
            analogGains.compAR = StageReferenceLevel / std::tanh(StageReferenceLevel * analogGains.driveAR);
        }
        if (m_routing == Routing::N || m_routing == Routing::AToN || m_routing == Routing::NToA) {
            analogGains.driveNL = m_driveLinN * driveScaleL;
            analogGains.driveNR = m_driveLinN * driveScaleR;
            analogGains.compNL = StageReferenceLevel / std::tanh(StageReferenceLevel * analogGains.driveNL);
            analogGains.compNR = StageReferenceLevel / std::tanh(StageReferenceLevel * analogGains.driveNR);
            analogGains.lfDriveNL = analogGains.driveNL * m_lfDriveFactor;
            analogGains.lfDriveNR = analogGains.driveNR * m_lfDriveFactor;
            analogGains.lfCompNL = StageReferenceLevel / std::tanh(StageReferenceLevel * analogGains.lfDriveNL);
            analogGains.lfCompNR = StageReferenceLevel / std::tanh(StageReferenceLevel * analogGains.lfDriveNR);
        }
        gains = &analogGains;
    }

    // 3. Oversampled nonlinear Console + Color stages
    double peakPre = 0.0;
    double peakPost = 0.0;
    if (factor == 1) {
        processStageFrame(wetL, wetR, *gains, peakPre, peakPost);
    } else {
        std::array<float, 4> wetHighL {};
        std::array<float, 4> wetHighR {};

        m_oversampling->upsamplerL.process(static_cast<float>(wetL), wetHighL.data(), factor);
        m_oversampling->upsamplerR.process(static_cast<float>(wetR), wetHighR.data(), factor);

        for (uint8_t k = 0; k < factor; k++) {
            double sL = static_cast<double>(wetHighL[k]);
            double sR = static_cast<double>(wetHighR[k]);
            processStageFrame(sL, sR, *gains, peakPre, peakPost);
            wetHighL[k] = static_cast<float>(sL);
            wetHighR[k] = static_cast<float>(sR);
        }

        wetL = static_cast<double>(m_oversampling->decimatorL.process(wetHighL.data(), factor));
        wetR = static_cast<double>(m_oversampling->decimatorR.process(wetHighR.data(), factor));

        if (needAlignedDry) {
            if (!m_prevNeedAlignedDry) {
                m_oversampling->dryUpsamplerL.reset();
                m_oversampling->dryUpsamplerR.reset();
                m_oversampling->dryDecimatorL.reset();
                m_oversampling->dryDecimatorR.reset();
                m_prevNeedAlignedDry = true;
            }
            std::array<float, 4> dryHighL {};
            std::array<float, 4> dryHighR {};
            m_oversampling->dryUpsamplerL.process(static_cast<float>(rawDryL), dryHighL.data(), factor);
            m_oversampling->dryUpsamplerR.process(static_cast<float>(rawDryR), dryHighR.data(), factor);
            alignedDryL = static_cast<double>(m_oversampling->dryDecimatorL.process(dryHighL.data(), factor));
            alignedDryR = static_cast<double>(m_oversampling->dryDecimatorR.process(dryHighR.data(), factor));
        } else {
            m_prevNeedAlignedDry = false;
        }
    }

    // 4. DC blocking + analog trim
    const double autoGain = m_autoGain.next();
    wetL = m_dcBlockerL.process(wetL) * trimGainL * autoGain;
    wetR = m_dcBlockerR.process(wetR) * trimGainR * autoGain;

    // 5. Post-stage Tone EQ, Vintage HF roll-off, Aspect elliptical M/S enhancement
    if (!m_eqPre) {
        processToneEq(wetL, wetR);
    }
    if (m_vintage) {
        m_vintageFilter[0].process(wetL);
        m_vintageFilter[1].process(wetR);
        wetL = m_vintageFilter[0].lowPass();
        wetR = m_vintageFilter[1].lowPass();
    }

    if (m_aspect > 0.0) {
        const double mid = 0.5 * (wetL + wetR);
        const double side = 0.5 * (wetL - wetR);
        const double sideFocused = m_aspectSideHp.process(side);
        const double sideEnhanced = m_aspectSideShelf.process(sideFocused);
        const double totalSide = side + m_aspect * (0.65 * sideEnhanced + 0.15 * side);
        wetL = mid + totalSide;
        wetR = mid - totalSide;
    }

    // 6. Saturation meter update
    double instantSaturationDb = 0.0;
    if (peakPre > 1.0e-12 && peakPost < peakPre) {
        instantSaturationDb = std::min(0.0, static_cast<double>(Utils::Dsp::linearToDb(static_cast<float>(peakPost / peakPre))));
    }
    if (instantSaturationDb < m_saturationDb) {
        m_saturationDb = instantSaturationDb;
    } else if (m_saturationDb != 0.0 || instantSaturationDb != 0.0) {
        m_saturationDb = m_meterReleaseCoeff * m_saturationDb + m_meterOneMinusReleaseCoeff * instantSaturationDb;
    }
    if (std::abs(m_saturationDb) < 1.0e-15) {
        m_saturationDb = 0.0;
    }

    // 7. Parallel mix + master output gain
    left = completeBlend(rawDryL, blendWetPart(alignedDryL, wetL, mixValue), m_outputGain);
    right = completeBlend(rawDryR, blendWetPart(alignedDryR, wetR, mixValue), m_outputGain);
}

void AnalogBus::processSample(double & left, double & right)
{
    double baseSampleRate = 0.0;
    uint8_t factor = 1;
    double stageSampleRate = 0.0;
    prepareForAudio(baseSampleRate, factor, stageSampleRate);

    // A frame of the measurement per frame played, so that a change costs no more than a second
    // copy of the stages for a moment
    if (m_autoGainEnabled) {
        if (!m_calibrating && m_measuredSettings != saturationSettings(baseSampleRate)) {
            startCalibration(baseSampleRate);
        }
        if (m_calibrating && stepCalibration(1)) {
            finishCalibration(baseSampleRate);
        }
    }

    const double mixValue = static_cast<double>(mix());
    const bool needAlignedDry = !sendMode() && (mixValue < 1.0);
    processFrame(left, right, baseSampleRate, factor, stageSampleRate, mixValue, needAlignedDry);
}

void AnalogBus::processBlock(AudioContext & context)
{
    if (context.frameCount == 0) {
        return;
    }

    double baseSampleRate = 0.0;
    uint8_t factor = 1;
    double stageSampleRate = 0.0;
    prepareForAudio(baseSampleRate, factor, stageSampleRate);

    bool stepCalibrationPerFrame = false;
    if (m_autoGainEnabled) {
        if (!m_calibrating && m_measuredSettings != saturationSettings(baseSampleRate)) {
            startCalibration(baseSampleRate);
        }
        if (m_calibrating) {
            const size_t calEnd = m_calibrationWarmUpFrames + m_calibrationFrames;
            if (m_calibrationFrame + context.frameCount < calEnd) {
                stepCalibration(context.frameCount);
            } else {
                stepCalibrationPerFrame = true;
            }
        }
    }

    const double mixValue = static_cast<double>(mix());
    const bool needAlignedDry = !sendMode() && (mixValue < 1.0);

    for (uint32_t i = 0; i < context.frameCount; i++) {
        if (stepCalibrationPerFrame && m_calibrating && stepCalibration(1)) {
            finishCalibration(baseSampleRate);
        }
        processFrame(context.buffer[i * 2], context.buffer[i * 2 + 1], baseSampleRate, factor, stageSampleRate, mixValue, needAlignedDry);
    }
}

void AnalogBus::reset()
{
    m_tightHpL.reset();
    m_tightHpR.reset();
    m_eqLowL.reset();
    m_eqLowR.reset();
    m_eqHighL.reset();
    m_eqHighR.reset();
    m_eqAirL.reset();
    m_eqAirR.reset();
    for (auto & f : m_stageNLfFilter) {
        f.reset();
    }
    for (auto & f : m_stageNSilkFilter) {
        f.reset();
    }
    m_colorLowL.reset();
    m_colorLowR.reset();
    m_colorMidL.reset();
    m_colorMidR.reset();
    m_vinylSideHp.reset();
    for (auto & f : m_colorLpFilter) {
        f.reset();
    }
    for (auto & f : m_vintageFilter) {
        f.reset();
    }
    m_aspectSideHp.reset();
    m_aspectSideShelf.reset();
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
    m_driftL.initialize(0);
    m_driftR.initialize(1);

    m_oversampling->upsamplerL.reset();
    m_oversampling->upsamplerR.reset();
    m_oversampling->decimatorL.reset();
    m_oversampling->decimatorR.reset();
    m_oversampling->dryUpsamplerL.reset();
    m_oversampling->dryUpsamplerR.reset();
    m_oversampling->dryDecimatorL.reset();
    m_oversampling->dryDecimatorR.reset();

    m_loFiHoldSample = { 0.0, 0.0 };
    m_loFiPhase = 0.0;
    m_saturationDb = 0.0;
    m_prevNeedAlignedDry = false;

    // A render starts at the right level: what is still to be measured is measured now
    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }
    if (m_autoGainEnabled) {
        const double sampleRate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
        if (m_calibrating || m_measuredSettings != saturationSettings(sampleRate)) {
            startCalibration(sampleRate);
            while (!stepCalibration(m_calibrationFrames)) {
            }
            finishCalibration(sampleRate);
        }
    }
    m_autoGain.reset();
}

void AnalogBus::sync()
{
    m_shouldSyncParameters = true;
}

const EffectPresetList & AnalogBus::factoryPresets() const
{
    return AnalogBusPresets::presets();
}

float AnalogBus::saturationDb() const
{
    return static_cast<float>(m_saturationDb);
}

double AnalogBus::autoGainDb() const
{
    return m_appliedAutoGainDb;
}

std::vector<std::string> AnalogBus::modulationPriority() const
{
    namespace C = Constants::NahdXml;
    return {
        key(C::xmlKeyDriveA()),
        key(C::xmlKeyDriveN()),
        key(C::xmlKeyColorDrive()),
        key(C::xmlKeyColorMix()),
        key(C::xmlKeyLowGain()),
        key(C::xmlKeyHighGain()),
        key(C::xmlKeyAirGain()),
        key(C::xmlKeyAspect()),
        key(C::xmlKeyMix()),
        key(C::xmlKeyGain())
    };
}

} // namespace noteahead
