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

#include "fm_operator.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! How hard the sine is driven to make the square. High enough to read as one, low enough that the
//! result stays a continuous curve: a literal square's vertical edges are an instant step, and no
//! amount of oversampling band-limits a step away.
constexpr double SquareDrive { 4.0 };

//! Share of the saw's cycle spent easing back up to the top, for the same reason. It costs a little
//! of the brightness a mathematical saw would have and buys back the top octave, which would
//! otherwise be aliases rather than harmonics.
constexpr double SawWrap { 0.05 };

double sawSample(double phase)
{
    if (phase < 1.0 - SawWrap) {
        return 1.0 - 2.0 * phase / (1.0 - SawWrap);
    }
    const double progress = (phase - (1.0 - SawWrap)) / SawWrap;
    return -1.0 + (1.0 - std::cos(std::numbers::pi * progress));
}

double waveformSample(FmOperator::Waveform waveform, double phase)
{
    const double angle = 2.0 * std::numbers::pi * phase;
    switch (waveform) {
    case FmOperator::Waveform::Sine:
        return std::sin(angle);
    case FmOperator::Waveform::HalfSine:
        return phase < 0.5 ? std::sin(angle) : 0.0;
    case FmOperator::Waveform::AbsSine:
        return std::abs(std::sin(angle));
    case FmOperator::Waveform::QuarterSine:
        return phase < 0.25 || (phase >= 0.5 && phase < 0.75) ? std::abs(std::sin(angle)) : 0.0;
    case FmOperator::Waveform::AlternatingSine:
        return phase < 0.5 ? std::sin(2.0 * angle) : 0.0;
    case FmOperator::Waveform::AlternatingAbsSine:
        return phase < 0.5 ? std::abs(std::sin(2.0 * angle)) : 0.0;
    case FmOperator::Waveform::Square:
        return std::tanh(SquareDrive * std::sin(angle)) / std::tanh(SquareDrive);
    case FmOperator::Waveform::Saw:
        return sawSample(phase);
    }
    return 0.0;
}

} // namespace

const std::vector<float> & FmOperator::tables()
{
    // Function-local static: built on the first operator constructed, and every operator after that
    // shares it. Devices build their voices off the audio thread, so the cost never lands on it.
    static const std::vector<float> built = [] {
        std::vector<float> result(WaveformCount * TableSize);
        for (size_t waveform = 0; waveform < WaveformCount; waveform++) {
            std::vector<double> cycle(TableSize);
            double mean = 0.0;
            for (size_t i = 0; i < TableSize; i++) {
                const double phase = static_cast<double>(i) / static_cast<double>(TableSize);
                cycle[i] = waveformSample(static_cast<Waveform>(waveform), phase);
                mean += cycle[i];
            }
            mean /= static_cast<double>(TableSize);

            // Every gated or rectified waveform spends its cycle mostly on one side of zero, so it
            // carries a large constant along with the sound -- two thirds of Abs Sine's amplitude
            // is offset rather than signal. The offset is taken out here, once, rather than being
            // left for a DC blocker downstream.
            //
            // It costs nothing either way an operator can be used. On a modulator a constant is
            // only a fixed rotation of the carrier's phase, which is inaudible; on a carrier it
            // would otherwise be multiplied by the amp envelope and heard as a thump at every note
            // on. A blocker can only fade that thump out over its own time constant -- some hundred
            // milliseconds at the few hertz a blocker has to sit at to leave the bass alone -- and
            // by then the note has started.
            for (size_t i = 0; i < TableSize; i++) {
                result[waveform * TableSize + i] = static_cast<float>(cycle[i] - mean);
            }
        }
        return result;
    }();
    return built;
}

std::vector<std::string> FmOperator::waveformNames()
{
    return { "Sine", "Half Sine", "Abs Sine", "Quarter Sine", "Alt Sine", "Alt Abs Sine", "Square", "Saw" };
}

FmOperator::FmOperator()
{
    tables();
    updatePhaseStep();
}

void FmOperator::setSampleRate(double sampleRate)
{
    if (std::abs(m_sampleRate - sampleRate) < 0.1) {
        return;
    }
    DspComponent::setSampleRate(sampleRate);
    m_envelope.setSampleRate(sampleRate);
    updatePhaseStep();
}

void FmOperator::setFrequency(double frequency)
{
    m_frequency = std::max(0.0, frequency);
    updatePhaseStep();
}

double FmOperator::frequency() const
{
    return m_frequency;
}

void FmOperator::setWaveform(Waveform waveform)
{
    m_waveform = waveform;
}

FmOperator::Waveform FmOperator::waveform() const
{
    return m_waveform;
}

void FmOperator::setLevel(double level)
{
    m_level = std::clamp(level, 0.0, 1.0);
}

double FmOperator::level() const
{
    return m_level;
}

void FmOperator::setFeedback(double cycles)
{
    m_feedback = std::max(0.0, cycles);
}

double FmOperator::feedback() const
{
    return m_feedback;
}

AdsrEnvelope & FmOperator::envelope()
{
    return m_envelope;
}

const AdsrEnvelope & FmOperator::envelope() const
{
    return m_envelope;
}

void FmOperator::trigger()
{
    m_envelope.trigger();
}

void FmOperator::release()
{
    m_envelope.release();
}

void FmOperator::reset()
{
    m_envelope.reset();
    m_phase = 0.0;
    m_value = 0.0;
    m_previous = 0.0;
    m_previous2 = 0.0;
}

void FmOperator::sync(double phase)
{
    m_phase = phase - std::floor(phase);
}

double FmOperator::lookup(double phase) const
{
    const double wrapped = phase - std::floor(phase);
    const double position = wrapped * static_cast<double>(TableSize);
    const size_t index = static_cast<size_t>(position) % TableSize;
    const double fraction = position - std::floor(position);
    const auto & table = tables();
    const size_t base = static_cast<size_t>(m_waveform) * TableSize;
    const double current = static_cast<double>(table[base + index]);
    const double next = static_cast<double>(table[base + (index + 1) % TableSize]);
    return current + (next - current) * fraction;
}

double FmOperator::nextSample(double phaseMod)
{
    // Self-feedback runs off the average of the last two outputs rather than off the last one
    // alone, which is the arrangement Yamaha arrived at on the DX7. Averaging puts a gentle
    // low-pass inside the loop, and that buys back most of the knob: measured here, the loop still
    // resolves to a steady waveform half way up, where fed off the last sample alone it has been
    // chaotic since a third of the way up.
    //
    // It does not stay steady all the way to the top, and is not meant to. Past roughly two thirds
    // the loop breaks into noise, which is where the DX-series gets its noise from and the only
    // noise source this synth has.
    const double feedbackMod = m_feedback > 0.0 ? m_feedback * (m_previous + m_previous2) * 0.5 : 0.0;
    const double output = lookup(m_phase + phaseMod + feedbackMod) * m_envelope.nextSample() * m_level;

    m_previous2 = m_previous;
    m_previous = output;
    m_value = output;

    m_phase += m_phaseStep;
    if (m_phase >= 1.0) {
        m_phase -= std::floor(m_phase);
    }

    return output;
}

double FmOperator::value() const
{
    return m_value;
}

double FmOperator::phase() const
{
    return m_phase;
}

bool FmOperator::isSilent() const
{
    return m_envelope.isSilent();
}

void FmOperator::updatePhaseStep()
{
    m_phaseStep = m_sampleRate > 0.0 ? m_frequency / m_sampleRate : 0.0;
}

} // namespace noteahead
