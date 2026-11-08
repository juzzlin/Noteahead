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

#include "modal_bass_string.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! 60 dB of amplitude in nepers: a T60 of T is a decay rate of this over T
constexpr double T60Nepers = 6.907755278982137;

// Where the Jazz Bass's pickups sit, from the bridge, as the growlybass fit put them
constexpr double NeckPickupMm = 155.0;
constexpr double BridgePickupMm = 65.0;

// What the fits needed so that the combs' nulls are as shallow as the recordings': a finger is not
// a point and the string moves in two planes. As a share of a partial at its fullest. The finger's
// comb is filled so far that it only colours the partials by 3 dB; the pickups' is what carves.
constexpr double PluckFill = 1.0;
constexpr double PickupFill = 0.41;

//! The partials stop here, or at the top of the sample rate, whichever comes first
constexpr double MaxPartialHz = 10000.0;

//! A lifted finger stops the higher partials faster than the low ones
constexpr double MuteCornerHz = 300.0;
//! The finger landing for the next pluck silences what was left of the last one this fast
constexpr double FingerLandingSeconds = 0.006;
//! The pickups' weights follow a change of length over this long, so a hammer-on does not click
constexpr double WeightSlewSeconds = 0.004;
//! What a hammer-on or a pull-off adds of its own, against a pluck of the same strength
constexpr double LegatoExcitation = 0.35;

constexpr double SilenceThreshold = 1.0e-6;
constexpr int SilenceCheckInterval = 256;

} // namespace

void ModalBassString::setSampleRate(double sampleRate)
{
    if (sampleRate <= 0.0 || (sampleRate == m_sampleRate && m_tailDecay > 0.0)) {
        return;
    }
    m_sampleRate = sampleRate;
    m_tailDecay = std::exp(-T60Nepers / (FingerLandingSeconds * m_sampleRate));
    m_weightSlew = 1.0 - std::exp(-1.0 / (WeightSlewSeconds * m_sampleRate));
    updateRotations();
}

void ModalBassString::setSettings(const Settings & settings)
{
    const bool pickupsChanged = settings.neck != m_settings.neck || settings.bridge != m_settings.bridge;
    const bool decayChanged = settings.fundamentalT60 != m_settings.fundamentalT60 || settings.lossPerHz != m_settings.lossPerHz || settings.muteT60 != m_settings.muteT60;
    m_settings = settings;
    if (pickupsChanged) {
        updatePickupWeights(false);
    }
    if (decayChanged) {
        updateRotations();
    }
}

double ModalBassString::decayRate(double frequency) const
{
    // Measured on both basses: every partial below about 130 Hz rings for the same long time, and
    // above that the losses grow about in proportion to the frequency
    double rate = T60Nepers / std::max(0.1, m_settings.fundamentalT60) + m_settings.lossPerHz * frequency;
    if (!m_held) {
        rate = std::max(rate, T60Nepers / std::max(0.005, m_settings.muteT60) * (1.0 + frequency / MuteCornerHz));
    }
    return rate;
}

void ModalBassString::setLength(double frequency, double fret, double inharmonicity)
{
    m_lengthMm = ScaleMm * std::exp2(-fret / 12.0);
    const double maxHz = std::min(MaxPartialHz, 0.45 * m_sampleRate);
    const double norm = 1.0 / std::sqrt(1.0 + inharmonicity);
    int count = 0;
    for (int k = 1; k <= MaxPartials; k++) {
        const double fk = frequency * k * std::sqrt(1.0 + inharmonicity * k * k) * norm;
        if (fk > maxHz) {
            break;
        }
        m_partials[static_cast<size_t>(k - 1)].frequency = fk;
        count = k;
    }
    // The partials a shorter string can no longer carry fall silent
    for (int k = count; k < m_partialCount; k++) {
        m_partials[static_cast<size_t>(k)] = {};
    }
    m_partialCount = std::max(1, count);
}

void ModalBassString::excite(double amplitude, double tilt, double scale)
{
    const double length = m_lengthMm;
    for (int i = 0; i < m_partialCount; i++) {
        const double k = static_cast<double>(i + 1);
        const double comb = std::sin(k * std::numbers::pi * m_settings.pluckMm / length);
        const double pluck = std::sqrt(comb * comb + PluckFill * PluckFill) / std::pow(k, tilt);
        const double fingerWidth = k * std::numbers::pi * m_settings.fingerMm / length;
        const double finger = std::exp(-0.5 * fingerWidth * fingerWidth);
        // Released from rest: the velocity the pickups sense starts at zero, so the partial does
        m_partials[static_cast<size_t>(i)].re += amplitude * scale * pluck * finger;
    }
}

void ModalBassString::updatePickupWeights(bool immediate)
{
    const double neck = m_settings.neck;
    const double bridge = m_settings.bridge;
    const double fill = PickupFill * std::sqrt(neck * neck + bridge * bridge);
    for (int i = 0; i < m_partialCount; i++) {
        auto & partial = m_partials[static_cast<size_t>(i)];
        const double k = static_cast<double>(i + 1);
        const double sensed = neck * std::sin(k * std::numbers::pi * NeckPickupMm / m_lengthMm) + bridge * std::sin(k * std::numbers::pi * BridgePickupMm / m_lengthMm);
        partial.targetWeight = std::sqrt(sensed * sensed + fill * fill);
        if (immediate) {
            partial.weight = partial.targetWeight;
        }
    }
}

void ModalBassString::updateRotations()
{
    for (int i = 0; i < m_partialCount; i++) {
        auto & partial = m_partials[static_cast<size_t>(i)];
        const double frequency = partial.frequency * m_pitchRatio;
        const double omega = 2.0 * std::numbers::pi * frequency / m_sampleRate;
        const double radius = std::exp(-decayRate(partial.frequency) / m_sampleRate);
        partial.rotRe = radius * std::cos(omega);
        partial.rotIm = radius * std::sin(omega);
    }
}

void ModalBassString::pluck(double frequency, double fret, double inharmonicity, double amplitude, double tilt)
{
    // The finger lands on the string first: what it was still doing goes into the tail and dies
    for (int i = 0; i < m_partialCount; i++) {
        auto & partial = m_partials[static_cast<size_t>(i)];
        partial.tailRe += partial.re * partial.weight;
        partial.tailIm += partial.im * partial.weight;
        partial.tailRotRe = partial.rotRe;
        partial.tailRotIm = partial.rotIm;
        partial.re = 0.0;
        partial.im = 0.0;
    }
    m_held = true;
    setLength(frequency, fret, inharmonicity);
    updatePickupWeights(true);
    updateRotations();
    excite(amplitude, tilt, 1.0);
    m_active = true;
    m_silenceCounter = 0;
}

void ModalBassString::legato(double frequency, double fret, double inharmonicity, double amplitude, double tilt)
{
    m_held = true;
    setLength(frequency, fret, inharmonicity);
    updatePickupWeights(false);
    updateRotations();
    excite(amplitude, tilt, LegatoExcitation);
    m_active = true;
    m_silenceCounter = 0;
}

void ModalBassString::mute()
{
    if (!m_held) {
        return;
    }
    m_held = false;
    updateRotations();
}

void ModalBassString::setPitchRatio(double ratio)
{
    if (std::abs(ratio - m_pitchRatio) < 1.0e-7) {
        return;
    }
    m_pitchRatio = ratio;
    updateRotations();
}

double ModalBassString::nextSample()
{
    if (!m_active) {
        return 0.0;
    }
    double out = 0.0;
    for (int i = 0; i < m_partialCount; i++) {
        auto & p = m_partials[static_cast<size_t>(i)];
        const double re = p.re;
        p.re = re * p.rotRe - p.im * p.rotIm;
        p.im = re * p.rotIm + p.im * p.rotRe;
        p.weight += (p.targetWeight - p.weight) * m_weightSlew;

        const double tre = p.tailRe;
        p.tailRe = m_tailDecay * (tre * p.tailRotRe - p.tailIm * p.tailRotIm);
        p.tailIm = m_tailDecay * (tre * p.tailRotIm + p.tailIm * p.tailRotRe);

        out += p.im * p.weight + p.tailIm;
    }

    if (++m_silenceCounter >= SilenceCheckInterval) {
        m_silenceCounter = 0;
        double energy = 0.0;
        for (int i = 0; i < m_partialCount; i++) {
            const auto & p = m_partials[static_cast<size_t>(i)];
            energy += (std::abs(p.re) + std::abs(p.im)) * p.weight + std::abs(p.tailRe) + std::abs(p.tailIm);
        }
        if (energy < SilenceThreshold) {
            reset();
        }
    }
    return out;
}

bool ModalBassString::isActive() const
{
    return m_active;
}

bool ModalBassString::isHeld() const
{
    return m_held && m_active;
}

void ModalBassString::reset()
{
    for (auto && partial : m_partials) {
        partial.re = 0.0;
        partial.im = 0.0;
        partial.tailRe = 0.0;
        partial.tailIm = 0.0;
    }
    m_held = false;
    m_active = false;
    m_silenceCounter = 0;
}

} // namespace noteahead
