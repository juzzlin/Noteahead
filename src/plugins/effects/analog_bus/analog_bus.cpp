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
    m_aspect = std::clamp(static_cast<double>(valueOf(C::xmlKeyAspect())), 0.0, 1.0);
    m_outputDb = outputGainDb(valueOf(C::xmlKeyGain()));

    m_filtersDirty = true;
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

    m_meterReleaseCoeff = std::exp(-1.0 / ((MeterReleaseMs / 1000.0) * baseSampleRate));
    m_lastBaseSampleRate = baseSampleRate;
    m_lastStageSampleRate = stageSampleRate;
    m_filtersDirty = false;
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

double AnalogBus::processStageA(double sample, double driveLin, double & peakPre, double & peakPost)
{
    const double u = sample * driveLin;
    const double shaped = consoleCurveA(u);
    peakPre = std::max(peakPre, std::abs(u));
    peakPost = std::max(peakPost, std::abs(shaped));

    const double comp = StageReferenceLevel / std::tanh(StageReferenceLevel * driveLin);
    return shaped * comp;
}

double AnalogBus::processStageN(double sample, double driveLin, size_t channel, double & peakPre, double & peakPost)
{
    // Split low frequencies to emulate core flux saturation in an iron output transformer
    m_stageNLfFilter[channel].process(sample);
    const double lf = m_stageNLfFilter[channel].lowPass();
    const double hf = m_stageNLfFilter[channel].highPass();

    const double lfDrive = driveLin * (1.0 + 0.65 * m_driveN);
    const double lfComp = StageReferenceLevel / std::tanh(StageReferenceLevel * lfDrive);
    const double lfU = lf * lfDrive;
    const double lfShaped = consoleCurveN(lfU) * lfComp;

    const double u = (hf + lfShaped * 0.35 + lf * 0.65) * driveLin;
    const double shaped = consoleCurveN(u);

    peakPre = std::max(peakPre, std::abs(u));
    peakPost = std::max(peakPost, std::abs(shaped));

    const double comp = StageReferenceLevel / std::tanh(StageReferenceLevel * driveLin);
    m_stageNSilkFilter[channel].process(shaped * comp);
    // Subtle 16 kHz top-end smoothing characteristic of discrete Class-A iron output stages
    return m_stageNSilkFilter[channel].lowPass() * 0.25 + shaped * comp * 0.75;
}

void AnalogBus::processColorFrame(double & left, double & right, double stageSampleRate, double & peakPre, double & peakPost)
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

        const double driveLin = 1.0 + 4.5 * m_colorDrive;
        const double comp = StageReferenceLevel / std::tanh(StageReferenceLevel * driveLin);
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

        const double driveLin = 1.0 + 6.0 * m_colorDrive;
        const double comp = StageReferenceLevel / std::tanh(StageReferenceLevel * driveLin);
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
        const double blendedSide = side * (1.0 - 0.65 * m_colorDrive) + sideHp * (0.65 * m_colorDrive);
        colL = m_colorMidL.process(m_colorLowL.process(mid + blendedSide));
        colR = m_colorMidR.process(m_colorLowR.process(mid - blendedSide));

        const double driveLin = 1.0 + 5.0 * m_colorDrive;
        const double comp = StageReferenceLevel / std::tanh(StageReferenceLevel * driveLin);
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

        const double driveLin = 1.0 + 4.0 * m_colorDrive;
        const double comp = StageReferenceLevel / std::tanh(StageReferenceLevel * driveLin);
        const double uL = colL * driveLin;
        const double uR = colR * driveLin;
        const double sL = std::tanh(uL);
        const double sR = std::tanh(uR);
        peakPre = std::max(peakPre, std::max(std::abs(uL), std::abs(uR)));
        peakPost = std::max(peakPost, std::max(std::abs(sL), std::abs(sR)));

        const double bits = 12.0 - 4.0 * m_colorDrive;
        const double step = std::pow(2.0, -bits);
        const double qL = std::round((sL * comp) / step) * step;
        const double qR = std::round((sR * comp) / step) * step;

        const double holdRateHz = 28000.0 - 20000.0 * m_colorDrive;
        m_loFiPhase += holdRateHz / stageSampleRate;
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

void AnalogBus::processStageFrame(double & left, double & right, double stageSampleRate, double driveScaleL, double driveScaleR, double & peakPre, double & peakPost)
{
    const double driveLinA = std::pow(10.0, (m_driveA * MaxConsoleDriveDb) / 20.0);
    const double driveLinN = std::pow(10.0, (m_driveN * MaxConsoleDriveDb) / 20.0);

    switch (m_routing) {
    case Routing::Off:
        break;
    case Routing::A:
        left = processStageA(left, driveLinA * driveScaleL, peakPre, peakPost);
        right = processStageA(right, driveLinA * driveScaleR, peakPre, peakPost);
        break;
    case Routing::N:
        left = processStageN(left, driveLinN * driveScaleL, 0, peakPre, peakPost);
        right = processStageN(right, driveLinN * driveScaleR, 1, peakPre, peakPost);
        break;
    case Routing::AToN:
        left = processStageA(left, driveLinA * driveScaleL, peakPre, peakPost);
        right = processStageA(right, driveLinA * driveScaleR, peakPre, peakPost);
        left = processStageN(left, driveLinN * driveScaleL, 0, peakPre, peakPost);
        right = processStageN(right, driveLinN * driveScaleR, 1, peakPre, peakPost);
        break;
    case Routing::NToA:
        left = processStageN(left, driveLinN * driveScaleL, 0, peakPre, peakPost);
        right = processStageN(right, driveLinN * driveScaleR, 1, peakPre, peakPost);
        left = processStageA(left, driveLinA * driveScaleL, peakPre, peakPost);
        right = processStageA(right, driveLinA * driveScaleR, peakPre, peakPost);
        break;
    }

    processColorFrame(left, right, stageSampleRate, peakPre, peakPost);
}

void AnalogBus::processSample(double & left, double & right)
{
    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }

    const double baseSampleRate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    const uint8_t factor = clampOversampleFactor(oversampleFactor());
    const double stageSampleRate = baseSampleRate * factor;

    if (m_filtersDirty || baseSampleRate != m_lastBaseSampleRate || stageSampleRate != m_lastStageSampleRate) {
        updateFilters(baseSampleRate, stageSampleRate);
    }

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

    // 2. Deterministic L/R analog hardware component tolerance + slow drift
    double driveScaleL = 1.0;
    double driveScaleR = 1.0;
    double trimGainL = 1.0;
    double trimGainR = 1.0;
    if (m_analog) {
        const double driftL = m_driftL.next(baseSampleRate);
        const double driftR = m_driftR.next(baseSampleRate);
        const double tolDriveL = AnalogDrift::tolerance(0, 0);
        const double tolDriveR = AnalogDrift::tolerance(1, 0);
        const double tolGainL = AnalogDrift::tolerance(0, 1);
        const double tolGainR = AnalogDrift::tolerance(1, 1);
        driveScaleL = 1.0 + 0.04 * tolDriveL + 0.008 * driftL;
        driveScaleR = 1.0 + 0.04 * tolDriveR + 0.008 * driftR;
        trimGainL = 1.0 + 0.015 * tolGainL + 0.003 * driftL;
        trimGainR = 1.0 + 0.015 * tolGainR + 0.003 * driftR;
    }

    // 3. Oversampled nonlinear Console + Color stages
    double peakPre = 0.0;
    double peakPost = 0.0;
    if (factor == 1) {
        processStageFrame(wetL, wetR, stageSampleRate, driveScaleL, driveScaleR, peakPre, peakPost);
    } else {
        std::array<float, 4> dryHighL {};
        std::array<float, 4> dryHighR {};
        std::array<float, 4> wetHighL {};
        std::array<float, 4> wetHighR {};

        m_oversampling->dryUpsamplerL.process(static_cast<float>(rawDryL), dryHighL.data(), factor);
        m_oversampling->dryUpsamplerR.process(static_cast<float>(rawDryR), dryHighR.data(), factor);
        m_oversampling->upsamplerL.process(static_cast<float>(wetL), wetHighL.data(), factor);
        m_oversampling->upsamplerR.process(static_cast<float>(wetR), wetHighR.data(), factor);

        for (uint8_t k = 0; k < factor; k++) {
            double sL = static_cast<double>(wetHighL[k]);
            double sR = static_cast<double>(wetHighR[k]);
            processStageFrame(sL, sR, stageSampleRate, driveScaleL, driveScaleR, peakPre, peakPost);
            wetHighL[k] = static_cast<float>(sL);
            wetHighR[k] = static_cast<float>(sR);
        }

        wetL = static_cast<double>(m_oversampling->decimatorL.process(wetHighL.data(), factor));
        wetR = static_cast<double>(m_oversampling->decimatorR.process(wetHighR.data(), factor));
        alignedDryL = static_cast<double>(m_oversampling->dryDecimatorL.process(dryHighL.data(), factor));
        alignedDryR = static_cast<double>(m_oversampling->dryDecimatorR.process(dryHighR.data(), factor));
    }

    // 4. DC blocking + analog trim
    m_dcBlockerL.setSampleRate(baseSampleRate);
    m_dcBlockerR.setSampleRate(baseSampleRate);
    wetL = m_dcBlockerL.process(wetL) * trimGainL;
    wetR = m_dcBlockerR.process(wetR) * trimGainR;

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

    const double mid = 0.5 * (wetL + wetR);
    const double side = 0.5 * (wetL - wetR);
    const double sideFocused = m_aspectSideHp.process(side);
    const double sideEnhanced = m_aspectSideShelf.process(sideFocused);
    const double totalSide = side + m_aspect * (0.65 * sideEnhanced + 0.15 * side);
    wetL = mid + totalSide;
    wetR = mid - totalSide;

    // 6. Saturation meter update
    double instantSaturationDb = 0.0;
    if (peakPre > 1.0e-12 && peakPost < peakPre) {
        instantSaturationDb = std::min(0.0, static_cast<double>(Utils::Dsp::linearToDb(static_cast<float>(peakPost / peakPre))));
    }
    if (instantSaturationDb < m_saturationDb) {
        m_saturationDb = instantSaturationDb;
    } else {
        m_saturationDb = m_meterReleaseCoeff * m_saturationDb + (1.0 - m_meterReleaseCoeff) * instantSaturationDb;
    }
    if (std::abs(m_saturationDb) < 1.0e-15) {
        m_saturationDb = 0.0;
    }

    // 7. Parallel mix + master output gain
    const double outputGain = static_cast<double>(Utils::Dsp::dbToLinear(static_cast<float>(m_outputDb)));
    const double mixValue = static_cast<double>(mix());
    left = completeBlend(rawDryL, blendWetPart(alignedDryL, wetL, mixValue), outputGain);
    right = completeBlend(rawDryR, blendWetPart(alignedDryR, wetR, mixValue), outputGain);
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
