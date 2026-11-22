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

#include "compressor_76.hpp"

#include "../../../common/constants.hpp"
#include "../../../common/parameter_mapper.hpp"
#include "../../../domain/dsp/upsampler.hpp"
#include "compressor_76_presets.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace noteahead {

namespace {

constexpr double MinGainDb { -30.0 };
constexpr double MaxGainDb { 30.0 };
constexpr double MinAttackMs { 0.02 };
constexpr double MaxAttackMs { 0.8 };
constexpr double MinReleaseMs { 50.0 };
constexpr double MaxReleaseMs { 1100.0 };
constexpr double MinLowCutHz { 20.0 };
constexpr double MaxLowCutHz { 400.0 };

constexpr double DefaultAttackMs { 0.2 };
constexpr double DefaultReleaseMs { 200.0 };

constexpr double BlueStripeAttackScale { 0.85 };
constexpr double BlueStripePresenceHz { 2000.0 };
constexpr double BlueStripePresenceQ { 0.65 };
constexpr double BlueStripePresenceDb { 1.2 };

std::string key(const QString & name)
{
    return name.toStdString();
}

} // namespace

struct Compressor76::Oversampling
{
    Upsampler upsamplerL;
    Upsampler upsamplerR;
    Decimator decimatorL;
    Decimator decimatorR;

    //! Latency-matched dry path so oversampled Mix blends in phase without comb filtering.
    Decimator dryDecimatorL;
    Decimator dryDecimatorR;
};

Compressor76::Compressor76()
  : m_oversampling { std::make_unique<Oversampling>() }
{
    namespace C = Constants::NahdXml;
    using Display = ParameterDisplay;

    const float defaultAttack = attackPosition(DefaultAttackMs);
    const float defaultRelease = releasePosition(DefaultReleaseMs);

    addParameter(Parameter { key(C::xmlKeyInputGain()), inputGainPosition(0.0), static_cast<int>(MinGainDb * 100.0), static_cast<int>(MaxGainDb * 100.0), 0, 100 });
    addParameter(Parameter { key(C::xmlKeyGain()), outputGainPosition(0.0), static_cast<int>(MinGainDb * 100.0), static_cast<int>(MaxGainDb * 100.0), 0, 100 });
    addParameter(Parameter { key(C::xmlKeyAttack()), defaultAttack, 0, 10000, Parameter::internalToXmlValue(defaultAttack, 0, 10000), 100 });
    addParameter(Parameter { key(C::xmlKeyRelease()), defaultRelease, 0, 10000, Parameter::internalToXmlValue(defaultRelease, 0, 10000), 100 });
    addParameter(Parameter { key(C::xmlKeyRatio()), 0.0f, 0, static_cast<int>(Ratio::All), 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { key(C::xmlKeyMode()), 0.0f, 0, static_cast<int>(Character::BlueStripe), 0, 1, Parameter::Type::Discrete });
    addParameter(Parameter { key(C::xmlKeyHpfCutoff()), 0.0f, 0, 10000, 0, 100 });
    addMixParameter(1.0f, MixLaw::Internal, 0, 10000, 100);

    setParameterDisplay(key(C::xmlKeyInputGain()), { .mapping = Display::Mapping::Value, .unit = "dB", .min = MinGainDb, .max = MaxGainDb, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Input") });
    setParameterDisplay(key(C::xmlKeyGain()), { .mapping = Display::Mapping::Value, .unit = "dB", .min = MinGainDb, .max = MaxGainDb, .rest = Display::Rest::Centre, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Output") });
    setParameterDisplay(key(C::xmlKeyAttack()), { .mapping = Display::Mapping::Exponential, .unit = "ms", .min = MinAttackMs, .max = MaxAttackMs, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Attack") });
    setParameterDisplay(key(C::xmlKeyRelease()), { .mapping = Display::Mapping::Exponential, .unit = "ms", .min = MinReleaseMs, .max = MaxReleaseMs, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Release") });
    setParameterDisplay(key(C::xmlKeyHpfCutoff()), { .mapping = Display::Mapping::LogFrequencyHighPass, .unit = "Hz", .min = MinLowCutHz, .max = MaxLowCutHz, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Low Cut") });
    setParameterDisplay(key(C::xmlKeyMix()), { .mapping = Display::Mapping::Linear, .unit = "%", .min = 0.0, .max = 100.0, .label = QT_TRANSLATE_NOOP("ParameterDisplay", "Mix") });

    syncParameters();
}

Compressor76::~Compressor76() = default;

std::string Compressor76::typeIdString()
{
    return "46294dfd-52e6-4b2b-8d3f-8cdf919d4a2d";
}

std::string Compressor76::type() const
{
    return Constants::RackEffectType::compressor76().toStdString();
}

std::string Compressor76::typeId() const
{
    return typeIdString();
}

double Compressor76::inputGainDb(float position)
{
    return MinGainDb + std::clamp(static_cast<double>(position), 0.0, 1.0) * (MaxGainDb - MinGainDb);
}

float Compressor76::inputGainPosition(double db)
{
    return static_cast<float>(std::clamp((db - MinGainDb) / (MaxGainDb - MinGainDb), 0.0, 1.0));
}

double Compressor76::outputGainDb(float position)
{
    return inputGainDb(position);
}

float Compressor76::outputGainPosition(double db)
{
    return inputGainPosition(db);
}

double Compressor76::attackMs(float position)
{
    return ParameterMapper::mapExponential(std::clamp(static_cast<double>(position), 0.0, 1.0), MinAttackMs, MaxAttackMs);
}

float Compressor76::attackPosition(double ms)
{
    return static_cast<float>(std::clamp(ParameterMapper::unmapExponential(ms, MinAttackMs, MaxAttackMs), 0.0, 1.0));
}

double Compressor76::releaseMs(float position)
{
    return ParameterMapper::mapExponential(std::clamp(static_cast<double>(position), 0.0, 1.0), MinReleaseMs, MaxReleaseMs);
}

float Compressor76::releasePosition(double ms)
{
    return static_cast<float>(std::clamp(ParameterMapper::unmapExponential(ms, MinReleaseMs, MaxReleaseMs), 0.0, 1.0));
}

double Compressor76::lowCutHz(float position)
{
    if (position <= 0.0001f) {
        return 0.0;
    }
    return ParameterMapper::mapLogFrequency(std::clamp(static_cast<double>(position), 0.0, 1.0), MinLowCutHz, MaxLowCutHz);
}

float Compressor76::lowCutPosition(double hz)
{
    if (hz <= 0.0) {
        return 0.0f;
    }
    return static_cast<float>(std::clamp(ParameterMapper::unmapLogFrequency(hz, MinLowCutHz, MaxLowCutHz), 0.0, 1.0));
}

std::string Compressor76::ratioLabel(Ratio ratio)
{
    switch (ratio) {
    case Ratio::FourToOne:
        return "4:1";
    case Ratio::EightToOne:
        return "8:1";
    case Ratio::TwelveToOne:
        return "12:1";
    case Ratio::TwentyToOne:
        return "20:1";
    case Ratio::All:
        return "All";
    }
    return "4:1";
}

double Compressor76::fetShape(double value, double grNorm, bool allMode, bool blueStripe)
{
    const double evenCoeff = (blueStripe ? 0.12 : 0.06) * grNorm + (allMode ? (blueStripe ? 0.55 : 0.40) : 0.0);
    const double oddDrive = 1.0 + (blueStripe ? 0.6 : 0.25) * grNorm + (allMode ? (blueStripe ? 5.5 : 4.5) : 0.0);
    const double u = std::tanh(oddDrive * value) / oddDrive;
    return u + evenCoeff * u * u;
}

double Compressor76::outputShape(double value, bool blueStripe)
{
    const double drive = blueStripe ? 1.4 : 1.0;
    const double even = blueStripe ? 0.16 : 0.08;
    const double u = std::tanh(drive * value) / drive;
    return u + even * u * u;
}

void Compressor76::syncParameters()
{
    namespace C = Constants::NahdXml;
    const auto valueOf = [this](const QString & name) {
        const auto p = parameter(key(name));
        return p ? p->get().value() : 0.0f;
    };

    m_inputGain = std::pow(10.0, inputGainDb(valueOf(C::xmlKeyInputGain())) / 20.0);
    m_outputGain = std::pow(10.0, outputGainDb(valueOf(C::xmlKeyGain())) / 20.0);

    m_ratio = static_cast<Ratio>(std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyRatio()))), 0, static_cast<int>(Ratio::All)));
    m_character = static_cast<Character>(std::clamp(static_cast<int>(std::lround(valueOf(C::xmlKeyMode()))), 0, static_cast<int>(Character::BlueStripe)));

    m_core.setRatio(m_ratio);
    m_core.setAttackSeconds(attackMs(valueOf(C::xmlKeyAttack())) / 1000.0);
    m_core.setReleaseSeconds(releaseMs(valueOf(C::xmlKeyRelease())) / 1000.0);
    m_core.setSideChainHpfHz(lowCutHz(valueOf(C::xmlKeyHpfCutoff())));
    m_core.setAttackScale(m_character == Character::BlueStripe ? BlueStripeAttackScale : 1.0);
}

void Compressor76::processSample(double & left, double & right)
{
    if (m_shouldSyncParameters) {
        syncParameters();
        m_shouldSyncParameters = false;
    }

    const double baseSampleRate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    const uint8_t factor = clampOversampleFactor(oversampleFactor());
    const double stageSampleRate = baseSampleRate * factor;

    m_core.setSampleRate(stageSampleRate);
    if (stageSampleRate != m_tunedStageSampleRate || m_character != m_tunedCharacter) {
        if (m_character == Character::BlueStripe) {
            m_presenceL.calculateBell(BlueStripePresenceHz, stageSampleRate, BlueStripePresenceQ, BlueStripePresenceDb);
            m_presenceR.calculateBell(BlueStripePresenceHz, stageSampleRate, BlueStripePresenceQ, BlueStripePresenceDb);
        } else {
            m_presenceL.setBypass();
            m_presenceR.setBypass();
            m_presenceL.reset();
            m_presenceR.reset();
        }
        m_tunedStageSampleRate = stageSampleRate;
        m_tunedCharacter = m_character;
    }

    const bool isAll = m_ratio == Ratio::All;
    const bool blueStripe = m_character == Character::BlueStripe;
    const auto processFrame = [&](double & frameL, double & frameR) {
        const double inL = frameL * m_inputGain;
        const double inR = frameR * m_inputGain;
        const double grDb = m_core.processGainDb(inL, inR);
        const double grLin = std::pow(10.0, grDb / 20.0);
        const double grNorm = std::clamp(-grDb / 20.0, 0.0, 1.0);

        double wetFrameL = fetShape(inL * grLin, grNorm, isAll, blueStripe);
        double wetFrameR = fetShape(inR * grLin, grNorm, isAll, blueStripe);

        wetFrameL = m_presenceL.process(wetFrameL);
        wetFrameR = m_presenceR.process(wetFrameR);

        frameL = outputShape(wetFrameL * m_outputGain, blueStripe);
        frameR = outputShape(wetFrameR * m_outputGain, blueStripe);
    };

    const double rawDryL = left;
    const double rawDryR = right;
    double dryL = rawDryL;
    double dryR = rawDryR;
    double wetL = rawDryL;
    double wetR = rawDryR;

    if (factor == 1) {
        processFrame(wetL, wetR);
    } else {
        std::array<float, 4> highL {};
        std::array<float, 4> highR {};
        m_oversampling->upsamplerL.process(static_cast<float>(rawDryL), highL.data(), factor);
        m_oversampling->upsamplerR.process(static_cast<float>(rawDryR), highR.data(), factor);
        const std::array<float, 4> dryHighL = highL;
        const std::array<float, 4> dryHighR = highR;
        for (uint8_t k = 0; k < factor; k++) {
            double sL = static_cast<double>(highL[k]);
            double sR = static_cast<double>(highR[k]);
            processFrame(sL, sR);
            highL[k] = static_cast<float>(sL);
            highR[k] = static_cast<float>(sR);
        }
        wetL = static_cast<double>(m_oversampling->decimatorL.process(highL.data(), factor));
        wetR = static_cast<double>(m_oversampling->decimatorR.process(highR.data(), factor));
        dryL = static_cast<double>(m_oversampling->dryDecimatorL.process(dryHighL.data(), factor));
        dryR = static_cast<double>(m_oversampling->dryDecimatorR.process(dryHighR.data(), factor));
    }

    m_dcBlockerL.setSampleRate(baseSampleRate);
    m_dcBlockerR.setSampleRate(baseSampleRate);
    wetL = m_dcBlockerL.process(wetL);
    wetR = m_dcBlockerR.process(wetR);

    const double mixValue = static_cast<double>(mix());
    left = completeBlend(rawDryL, blendWetPart(dryL, wetL, mixValue), 1.0);
    right = completeBlend(rawDryR, blendWetPart(dryR, wetR, mixValue), 1.0);
}

void Compressor76::reset()
{
    m_core.reset();
    m_presenceL.reset();
    m_presenceR.reset();
    m_dcBlockerL.reset();
    m_dcBlockerR.reset();
    m_oversampling->upsamplerL.reset();
    m_oversampling->upsamplerR.reset();
    m_oversampling->decimatorL.reset();
    m_oversampling->decimatorR.reset();
    m_oversampling->dryDecimatorL.reset();
    m_oversampling->dryDecimatorR.reset();
}

void Compressor76::sync()
{
    m_shouldSyncParameters = true;
}

bool Compressor76::isSettled() const
{
    return m_core.isSettled();
}

float Compressor76::reductionDb() const
{
    return static_cast<float>(m_core.reductionDb());
}

const EffectPresetList & Compressor76::factoryPresets() const
{
    return Compressor76Presets::presets();
}

std::vector<std::string> Compressor76::modulationPriority() const
{
    namespace C = Constants::NahdXml;
    return {
        key(C::xmlKeyInputGain()),
        key(C::xmlKeyGain()),
        key(C::xmlKeyAttack()),
        key(C::xmlKeyRelease()),
        key(C::xmlKeyMix())
    };
}

} // namespace noteahead
