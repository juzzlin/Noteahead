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

#include "piano_soundboard.hpp"

#include "modal_grand_string.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

constexpr double Pi = 3.14159265358979323846;

//! The soundboard's and the rim's lowest resonances, how long each rings and how strongly it speaks
struct BodyMode
{
    double hz;
    double t60;
    double level;
};

constexpr std::array<BodyMode, 12> Body { { { 58.0, 0.35, 1.0 }, { 92.0, 0.30, 0.9 }, { 138.0, 0.25, 0.85 }, { 186.0, 0.22, 0.7 }, { 245.0, 0.20, 0.6 }, { 330.0, 0.16, 0.5 }, { 440.0, 0.13, 0.4 }, { 600.0, 0.10, 0.32 }, { 820.0, 0.08, 0.25 }, { 1150.0, 0.06, 0.2 }, { 1600.0, 0.05, 0.15 }, { 2300.0, 0.04, 0.1 } } };

//! How much of the soundboard's motion reaches a free string, and how much of the strings is heard
constexpr double Coupling = 0.0015;
constexpr double ResonanceGain = 2.0;
constexpr double KnockGain = 0.6;
//! The free strings' loop low-pass: their upper partials die faster, as the struck strings' do
constexpr double CombLowpass = 0.7;
//! What the low-pass delays the loop by at low frequencies, in samples, taken off the line's length
constexpr double CombLowpassDelay = (1.0 - CombLowpass) / CombLowpass;
//! How fast a string dies when its damper comes down
constexpr double DampedSeconds = 0.1;
constexpr double Silence = 1.0e-12;

} // namespace

void PianoSoundboard::setSampleRate(double sampleRate)
{
    if (sampleRate == m_sampleRate || sampleRate <= 0.0) {
        return;
    }
    m_sampleRate = sampleRate;
    for (size_t i = 0; i < Body.size(); i++) {
        const double r = std::exp(-6.91 / (Body.at(i).t60 * sampleRate));
        m_bodyA1.at(i) = 2.0 * r * std::cos(2.0 * Pi * Body.at(i).hz / sampleRate);
        m_bodyA2.at(i) = -r * r;
    }
    tune();
    reset();
}

void PianoSoundboard::setTuning(double stretch, double decayScale, double panSpread)
{
    if (stretch != m_stretch || decayScale != m_decayScale || panSpread != m_panSpread) {
        m_stretch = stretch;
        m_decayScale = decayScale;
        m_panSpread = panSpread;
        tune();
    }
}

void PianoSoundboard::tune()
{
    if (m_sampleRate <= 0.0) {
        return;
    }
    for (int i = 0; i < StringCount; i++) {
        const int note = LowestNote + i;
        const double hz = 440.0 * std::exp2((note - 69) / 12.0 + ModalGrandString::stretchCents(note) * m_stretch / 1200.0);
        auto & comb = m_combs.at(i);
        comb.delay = std::max(2.0, m_sampleRate / hz - CombLowpassDelay);
        const auto length = static_cast<size_t>(std::ceil(comb.delay)) + 2;
        if (comb.line.size() != length) {
            comb.line.assign(length, 0.0);
            comb.write = 0;
        }
        const double seconds = ModalGrandString::slowDecaySeconds(hz) * m_decayScale;
        comb.openGain = std::pow(10.0, -3.0 * comb.delay / (m_sampleRate * seconds));
        comb.closedGain = std::pow(10.0, -3.0 * comb.delay / (m_sampleRate * DampedSeconds));
        comb.pan = ModalGrandString::keyPan(note, m_panSpread);
    }
}

void PianoSoundboard::knock(uint8_t note, double strength)
{
    if (m_sampleRate <= 0.0) {
        return;
    }
    const double noteHz = 440.0 * std::exp2((static_cast<int>(note) - 69) / 12.0);
    for (size_t i = 0; i < Body.size(); i++) {
        // A key nearer a resonance in pitch is struck nearer where that resonance moves the board
        const double distance = std::log2(Body.at(i).hz / noteHz) / 2.0;
        const double weight = Body.at(i).level * (0.3 + std::exp(-0.5 * distance * distance));
        const double omega = 2.0 * Pi * Body.at(i).hz / m_sampleRate;
        // Added to whatever is ringing: the board is one, however many keys strike it
        m_bodyY2.at(i) -= strength * weight * std::sin(omega) / -m_bodyA2.at(i);
    }
    m_bodyEnergy = std::max(m_bodyEnergy, strength * strength);
}

void PianoSoundboard::setOpenStrings(const std::bitset<128> & open)
{
    m_open = open;
    if (open.any()) {
        m_combsActive = true;
    }
}

void PianoSoundboard::process(double strings, double knockLevel, double resonance, double & left, double & right)
{
    if (m_bodyEnergy > Silence) {
        double body = 0.0;
        double side = 0.0;
        for (size_t i = 0; i < Body.size(); i++) {
            const double y = m_bodyA1.at(i) * m_bodyY1.at(i) + m_bodyA2.at(i) * m_bodyY2.at(i);
            m_bodyY2.at(i) = m_bodyY1.at(i);
            m_bodyY1.at(i) = y;
            body += y;
            side += i % 2 ? -y : y;
        }
        body *= KnockGain * knockLevel;
        side *= 0.3 * KnockGain * knockLevel;
        left += body + side;
        right += body - side;
        m_bodyEnergy += 0.001 * (body * body - m_bodyEnergy);
    }

    if (!m_combsActive) {
        return;
    }
    bool any = false;
    const double input = strings * Coupling;
    double l = 0.0;
    double r = 0.0;
    for (int i = 0; i < StringCount; i++) {
        auto & comb = m_combs.at(i);
        const bool open = m_open.test(static_cast<size_t>(LowestNote + i));
        if (!open && comb.energy < Silence) {
            continue;
        }
        any = true;
        const auto size = static_cast<int>(comb.line.size());
        double read = static_cast<double>(comb.write) - comb.delay;
        if (read < 0.0) {
            read += size;
        }
        const auto i0 = static_cast<int>(read);
        const double frac = read - i0;
        const double delayed = comb.line.at(i0) + frac * (comb.line.at((i0 + 1) % size) - comb.line.at(i0));
        comb.lowpass += CombLowpass * (delayed - comb.lowpass);
        const double y = (open ? input : 0.0) + (open ? comb.openGain : comb.closedGain) * comb.lowpass;
        comb.line.at(comb.write) = y;
        comb.write = (comb.write + 1) % size;
        comb.energy += 0.001 * (y * y - comb.energy);
        l += y * std::cos(0.5 * Pi * comb.pan);
        r += y * std::sin(0.5 * Pi * comb.pan);
    }
    left += l * ResonanceGain * resonance;
    right += r * ResonanceGain * resonance;
    m_combsActive = any;
}

bool PianoSoundboard::isActive() const
{
    return m_bodyEnergy > Silence || m_combsActive;
}

void PianoSoundboard::reset()
{
    m_bodyY1.fill(0.0);
    m_bodyY2.fill(0.0);
    m_bodyEnergy = 0.0;
    for (auto && comb : m_combs) {
        std::ranges::fill(comb.line, 0.0);
        comb.write = 0;
        comb.lowpass = 0.0;
        comb.energy = 0.0;
    }
    m_combsActive = m_open.any();
}

} // namespace noteahead
