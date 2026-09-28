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

#include "bit_crusher.hpp"

#include "../../common/constants.hpp"
#include "../../common/parameter_mapper.hpp"
#include "../dsp/audio_context.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! What the rate control spans, in Hz. The bottom is low enough to be a texture rather than an
//! effect; the top is above anything the engine runs at, which is where holding does nothing.
constexpr double MinTargetRate = 100.0;
constexpr double MaxTargetRate = 48000.0;

} // namespace

BitCrusher::BitCrusher()
{
    // Both default to transparent, so an effect dropped into a rack is heard only once it is
    // turned to something.
    addParameter(Parameter { Constants::NahdXml::xmlKeyBitDepth().toStdString(), static_cast<float>(maxBits()), 1, maxBits(), maxBits(), 1, Parameter::Type::Discrete });
    addParameter(Parameter { Constants::NahdXml::xmlKeySampleRate().toStdString(), 1.0f, 0, 10000, 10000, 100 });
    addMixParameter(1.0f);
}

int BitCrusher::maxBits()
{
    return 16;
}

std::string BitCrusher::typeIdString()
{
    return "efe35634-88c2-492f-bed3-211ac0ef5c4c";
}

std::string BitCrusher::type() const
{
    return Constants::RackEffectType::bitCrusher().toStdString();
}

std::string BitCrusher::typeId() const
{
    return typeIdString();
}

double BitCrusher::quantize(double sample) const
{
    if (m_bits >= maxBits()) {
        return sample;
    }
    // Steps either side of zero rather than over the whole range, so that silence stays silence.
    // Quantising the full range instead puts a step boundary at zero, and a signal resting there
    // then dithers between the two nearest levels on its own noise.
    const double levels = std::pow(2.0, m_bits - 1);
    return std::round(std::clamp(sample, -1.0, 1.0) * levels) / levels;
}

void BitCrusher::processSample(double & left, double & right)
{
    // Wet only: Effect::process() keeps the dry signal and blends it back, so an effect that mixed
    // here would have the mix applied to it twice.
    //
    // Sample and hold. The position advances by one frame at a time and a new sample is taken
    // whenever it has covered a whole held frame, so a target rate that does not divide the real
    // one lands on the right average rather than on the nearest divisor.
    m_holdPosition += 1.0;
    if (m_holdPosition >= m_holdFrames) {
        m_holdPosition -= m_holdFrames;
        m_heldLeft = quantize(left);
        m_heldRight = quantize(right);
    }

    left = m_heldLeft;
    right = m_heldRight;
}

void BitCrusher::processBlock(AudioContext & context)
{
    if (m_sampleRate != context.sampleRate) {
        m_sampleRate = context.sampleRate;
        sync();
    }

    for (uint32_t i = 0; i < context.frameCount; i++) {
        processSample(context.buffer[i * 2], context.buffer[i * 2 + 1]);
    }
}

void BitCrusher::sync()
{
    if (const auto p = parameter(Constants::NahdXml::xmlKeyBitDepth().toStdString()); p) {
        m_bits = std::clamp(p->get().xmlValue(), 1, maxBits());
    }
    if (const auto p = parameter(Constants::NahdXml::xmlKeySampleRate().toStdString()); p) {
        m_targetRate = ParameterMapper::mapExponential(p->get().value(), MinTargetRate, MaxTargetRate);
    }

    // At or above the real rate nothing is held: one frame per sample is no reduction at all.
    const double rate = m_sampleRate > 0.0 ? m_sampleRate : Constants::defaultSampleRate();
    m_holdFrames = std::max(1.0, rate / std::max(1.0, m_targetRate));
}

void BitCrusher::reset()
{
    Effect::reset();
    m_holdPosition = 0.0;
    m_heldLeft = 0.0;
    m_heldRight = 0.0;
}

} // namespace noteahead
