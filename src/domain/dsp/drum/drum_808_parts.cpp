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

#include "drum_808_parts.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead::Drum808 {

namespace {

//! Kept clear of Nyquist, where the cookbook's filters fold over
double clampedOmega(double hz, double sampleRate)
{
    return 2.0 * std::numbers::pi * std::clamp(hz, 1.0, sampleRate * 0.45) / sampleRate;
}

} // namespace

void Biquad::setBandPass(double hz, double q, double sampleRate)
{
    const double omega = clampedOmega(hz, sampleRate);
    const double alpha = std::sin(omega) / (2.0 * q);
    const double a0 = 1.0 + alpha;
    m_b0 = alpha / a0;
    m_b1 = 0.0;
    m_b2 = -alpha / a0;
    m_a1 = -2.0 * std::cos(omega) / a0;
    m_a2 = (1.0 - alpha) / a0;
}

void Biquad::setHighPass(double hz, double q, double sampleRate)
{
    const double omega = clampedOmega(hz, sampleRate);
    const double alpha = std::sin(omega) / (2.0 * q);
    const double cosine = std::cos(omega);
    const double a0 = 1.0 + alpha;
    m_b0 = (1.0 + cosine) / 2.0 / a0;
    m_b1 = -(1.0 + cosine) / a0;
    m_b2 = m_b0;
    m_a1 = -2.0 * cosine / a0;
    m_a2 = (1.0 - alpha) / a0;
}

void Biquad::setLowPass(double hz, double q, double sampleRate)
{
    const double omega = clampedOmega(hz, sampleRate);
    const double alpha = std::sin(omega) / (2.0 * q);
    const double cosine = std::cos(omega);
    const double a0 = 1.0 + alpha;
    m_b0 = (1.0 - cosine) / 2.0 / a0;
    m_b1 = (1.0 - cosine) / a0;
    m_b2 = m_b0;
    m_a1 = -2.0 * cosine / a0;
    m_a2 = (1.0 - alpha) / a0;
}

void Biquad::setLowShelf(double hz, double gainDb, double q, double sampleRate)
{
    const double omega = clampedOmega(hz, sampleRate);
    const double a = std::pow(10.0, gainDb / 40.0);
    const double cosine = std::cos(omega);
    const double s = 2.0 * std::sqrt(a) * std::sin(omega) / (2.0 * q);
    const double a0 = (a + 1.0) + (a - 1.0) * cosine + s;
    m_b0 = a * ((a + 1.0) - (a - 1.0) * cosine + s) / a0;
    m_b1 = 2.0 * a * ((a - 1.0) - (a + 1.0) * cosine) / a0;
    m_b2 = a * ((a + 1.0) - (a - 1.0) * cosine - s) / a0;
    m_a1 = -2.0 * ((a - 1.0) + (a + 1.0) * cosine) / a0;
    m_a2 = ((a + 1.0) + (a - 1.0) * cosine - s) / a0;
}

double Biquad::process(double input)
{
    const double output = m_b0 * input + m_b1 * m_x1 + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;
    m_x2 = m_x1;
    m_x1 = input;
    m_y2 = m_y1;
    m_y1 = output;
    return output;
}

void Biquad::reset()
{
    m_x1 = m_x2 = m_y1 = m_y2 = 0.0;
}

Noise::Noise(uint32_t seed)
  : m_seed { seed }
  , m_state { seed }
{
}

void Noise::reseed()
{
    m_state = m_seed;
}

double Noise::next()
{
    // xorshift32, uniform -1..1 scaled to unit deviation as the fits' Gaussian noise
    m_state ^= m_state << 13;
    m_state ^= m_state >> 17;
    m_state ^= m_state << 5;
    return std::sqrt(3.0) * (static_cast<double>(m_state) / 2147483648.0 - 1.0);
}

void ModeOscillator::start(const Mode & mode, double sampleRate)
{
    // The imaginary part is the output, so a phasor at the phase gives sin(phase) at the start
    m_state = std::polar(mode.amplitude, mode.phase);
    m_step = std::polar(std::exp(-1.0 / (mode.seconds * sampleRate)), 2.0 * std::numbers::pi * mode.hz / sampleRate);
}

double ModeOscillator::next()
{
    const double output = m_state.imag();
    m_state *= m_step;
    return output;
}

void ModeOscillator::stop()
{
    m_state = {};
}

SquareBank::SquareBank(uint8_t mask)
  : m_mask { mask }
{
    for (size_t i = 0; i < FrequenciesHz.size(); i++) {
        if (m_mask & (1u << i)) {
            m_count += 1.0;
        }
    }
}

void SquareBank::setSampleRate(double sampleRate)
{
    for (size_t i = 0; i < FrequenciesHz.size(); i++) {
        m_step.at(i) = FrequenciesHz.at(i) / sampleRate;
    }
}

void SquareBank::scramble(uint32_t seed)
{
    for (auto && phase : m_phase) {
        seed = seed * 1664525u + 1013904223u;
        phase = static_cast<double>(seed >> 8) / 16777216.0;
    }
}

double SquareBank::next()
{
    // A naive square smoothed at both edges by PolyBLEP, which leaves the fits' band-limited ones
    const auto blep = [](double t, double dt) {
        if (t < dt) {
            t /= dt;
            return t + t - t * t - 1.0;
        }
        if (t > 1.0 - dt) {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        }
        return 0.0;
    };
    double sum = 0.0;
    for (size_t i = 0; i < FrequenciesHz.size(); i++) {
        if (!(m_mask & (1u << i))) {
            continue;
        }
        double & phase = m_phase.at(i);
        const double dt = m_step.at(i);
        double value = phase < 0.5 ? 0.5 : -0.5;
        value += 0.5 * blep(phase, dt);
        value -= 0.5 * blep(std::fmod(phase + 0.5, 1.0), dt);
        sum += value;
        phase += dt;
        if (phase >= 1.0) {
            phase -= 1.0;
        }
    }
    return m_count > 0.0 ? sum / m_count : 0.0;
}

double impulseDeviation(std::initializer_list<const Biquad *> filters)
{
    // Copied into a fixed array: this runs when a drum is struck, on the audio thread
    std::array<Biquad, 4> copies {};
    size_t count = 0;
    for (auto && filter : filters) {
        if (count < copies.size()) {
            copies.at(count) = *filter;
            copies.at(count).reset();
            count++;
        }
    }
    double energy = 0.0;
    for (int i = 0; i < 16384; i++) {
        double y = i == 0 ? 1.0 : 0.0;
        for (size_t j = 0; j < count; j++) {
            y = copies.at(j).process(y);
        }
        energy += y * y;
    }
    return std::sqrt(std::max(energy, 1.0e-30));
}

namespace {

//! Fritsch and Carlson's monotone cubic: the slopes at the points are what keeps it from overshooting
double monotoneCubic(const std::array<double, 5> & x, const std::array<double, 5> & y, double at)
{
    if (at <= x.front()) {
        return y.front();
    }
    if (at >= x.back()) {
        return y.back();
    }
    constexpr size_t n = 5;
    std::array<double, n - 1> secant {};
    for (size_t i = 0; i + 1 < n; i++) {
        secant.at(i) = (y.at(i + 1) - y.at(i)) / (x.at(i + 1) - x.at(i));
    }
    std::array<double, n> slope {};
    slope.front() = secant.front();
    slope.back() = secant.back();
    for (size_t i = 1; i + 1 < n; i++) {
        const double left = secant.at(i - 1);
        const double right = secant.at(i);
        if (left * right <= 0.0) {
            slope.at(i) = 0.0;
        } else {
            const double leftWidth = x.at(i) - x.at(i - 1);
            const double rightWidth = x.at(i + 1) - x.at(i);
            const double w1 = 2.0 * rightWidth + leftWidth;
            const double w2 = rightWidth + 2.0 * leftWidth;
            slope.at(i) = (w1 + w2) / (w1 / left + w2 / right);
        }
    }
    size_t i = 0;
    while (at > x.at(i + 1)) {
        i++;
    }
    const double h = x.at(i + 1) - x.at(i);
    const double t = (at - x.at(i)) / h;
    const double t2 = t * t;
    const double t3 = t2 * t;
    return (2.0 * t3 - 3.0 * t2 + 1.0) * y.at(i) + (t3 - 2.0 * t2 + t) * h * slope.at(i) + (-2.0 * t3 + 3.0 * t2) * y.at(i + 1) + (t3 - t2) * h * slope.at(i + 1);
}

} // namespace

double KnobCurve::at(float knob) const
{
    return monotoneCubic(knobs, values, static_cast<double>(knob));
}

double KnobCurve::ratioAt(float knob) const
{
    std::array<double, 5> logs {};
    for (size_t i = 0; i < values.size(); i++) {
        logs.at(i) = std::log(values.at(i));
    }
    return std::exp(monotoneCubic(knobs, logs, static_cast<double>(knob)));
}

} // namespace noteahead::Drum808
