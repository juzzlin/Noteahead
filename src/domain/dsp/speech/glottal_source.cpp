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

#include "glottal_source.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! What the open quotient is allowed to span.
//!
//! Below the floor the pulse is a handful of samples at any sensible pitch and turns into a click;
//! above the ceiling there is no closed phase left at all, and a fold that never shuts has no snap
//! to it and so no harmonics above the second.
constexpr double MinOpenQuotient = 0.25;
constexpr double MaxOpenQuotient = 0.85;

//! The pitch the pulse is scaled against, and how far the scaling is allowed to travel either way.
//!
//! The tilt that follows the source is a high pass, so a partial below its corner is differentiated
//! and one above it is passed: how much of the pulse survives therefore depends on where its
//! harmonics sit, which is to say on the pitch. A sawtooth barely shows it because its spectrum is
//! shallow, but a glottal pulse is far darker and puts most of its energy in the two or three
//! partials nearest the fundamental -- so at the bottom of the keyboard nearly all of it lands below
//! the corner. Measured across three octaves the level fell 25 dB, against 8 for the sawtooth, which
//! is a bass note that cannot be heard rather than a voice with a low register.
//!
//! Cancelled by scaling the pulse against the pitch, which holds the flow's rate of fall -- and so
//! the excitation -- where it is however low the note. Clamped either side because the compensation
//! has nothing to say outside the range a voice sings in, and unclamped it would turn a stray
//! sub-audio frequency into an enormous pulse.
constexpr double ReferenceFrequency = 220.0;
constexpr double MinFrequencyScale = 0.25;
constexpr double MaxFrequencyScale = 4.0;

} // namespace

GlottalSource::GlottalSource()
{
    updateShapeNorms();
    updateFrequencyScale();
}

void GlottalSource::setSampleRate(double sampleRate)
{
    DspComponent::setSampleRate(sampleRate);
    m_saw.setSampleRate(sampleRate);
    updatePhaseStep();
}

void GlottalSource::setFrequency(double frequency)
{
    m_frequency = std::max(1.0, frequency);
    m_saw.setFrequency(m_frequency);
    updatePhaseStep();
    updateFrequencyScale();
}

void GlottalSource::setModel(Model model)
{
    m_model = model;
}

void GlottalSource::setOpenQuotient(double openQuotient)
{
    m_openQuotient = std::clamp(openQuotient, MinOpenQuotient, MaxOpenQuotient);
    updateShapeNorms();
}

void GlottalSource::setSpeedQuotient(double speedQuotient)
{
    m_speedQuotient = std::max(1.0, speedQuotient);
    updateShapeNorms();
}

void GlottalSource::updateFrequencyScale()
{
    m_frequencyScale = std::clamp(ReferenceFrequency / m_frequency, MinFrequencyScale, MaxFrequencyScale);
}

void GlottalSource::updateShapeNorms()
{
    // The mean integrates in closed form: the raised cosine over its rise averages a half and the
    // quarter cosine over its fall averages 2/pi, while the closed phase contributes nothing -- so
    // the mean falls with the open quotient, which is why openness() has to divide by it.
    const double rising = m_openQuotient * m_speedQuotient / (1.0 + m_speedQuotient);
    const double falling = m_openQuotient - rising;
    m_meanFlow = std::max(1e-6, 0.5 * rising + falling * 2.0 / std::numbers::pi);
    // The closing quarter-cosine reaches zero at a slope of pi/2 over its own length, so a pulse
    // that shuts twice as fast excites the tract twice as hard. Scaling by that length takes the
    // shape back out of the level; the reference it is relative to is folded into the caller's
    // makeup gain, which is the one place a level belongs.
    m_shapeScale = std::max(1e-6, falling);
}

void GlottalSource::setJitter(double jitter)
{
    m_jitter = std::clamp(jitter, 0.0, 0.5);
}

void GlottalSource::setShimmer(double shimmer)
{
    m_shimmer = std::clamp(shimmer, 0.0, 0.9);
}

void GlottalSource::updatePhaseStep()
{
    m_phaseStep = m_sampleRate > 0.0 ? m_frequency / m_sampleRate : 0.0;
}

void GlottalSource::beginPeriod()
{
    // Drawn per period rather than smoothed into one, because that is what jitter is: the folds do
    // not know what the last cycle did. A smoothed version of it is vibrato with a rough edge.
    m_periodScale = 1.0 + m_jitter * m_deviation(m_rng);
    m_amplitude = 1.0 + m_shimmer * m_deviation(m_rng);
}

double GlottalSource::rosenberg(double phase) const
{
    // The opening and closing stretches share the open quotient in the ratio the speed quotient
    // names, so that changing one does not silently change the other.
    const double open = m_openQuotient;
    const double rising = open * m_speedQuotient / (1.0 + m_speedQuotient);
    const double falling = open - rising;

    if (phase < rising) {
        // Blowing open: half a cosine, from shut to fully open.
        return 0.5 * (1.0 - std::cos(std::numbers::pi * phase / std::max(1e-9, rising)));
    }
    if (phase < open) {
        // Snapping shut: a quarter cosine, which reaches zero with a non-zero slope. That corner in
        // the derivative is the excitation -- a pulse that closed smoothly would have none.
        return std::cos(0.5 * std::numbers::pi * (phase - rising) / std::max(1e-9, falling));
    }
    return 0.0;
}

double GlottalSource::nextSample()
{
    if (m_model == Model::Saw) {
        // Flat, so a caller that modulates by this gets exactly the steady aspiration it used to.
        m_openness = 1.0;
        return m_saw.nextSample();
    }

    // The flow *is* how far open the folds are, so the two are one evaluation: the aspiration
    // wants it before the shimmer, which is a property of the pulse rather than of the gap.
    const double opening = rosenberg(m_phase);
    m_openness = opening / m_meanFlow;
    const double flow = opening * m_amplitude * m_shapeScale * m_frequencyScale;

    // A long period is a low-frequency one, so the step is divided by the scale rather than
    // multiplied: that is what makes the jitter a wander in pitch rather than in amplitude.
    m_phase += m_phaseStep / std::max(0.5, m_periodScale);
    if (m_phase >= 1.0) {
        m_phase -= std::floor(m_phase);
        beginPeriod();
    }

    return flow;
}

double GlottalSource::openness() const
{
    return m_openness;
}

void GlottalSource::reset()
{
    m_saw.reset();
    m_phase = 0.0;
    m_periodScale = 1.0;
    m_amplitude = 1.0;
    m_openness = 1.0;
    m_rng.seed(0x51EEC4);
}

} // namespace noteahead
