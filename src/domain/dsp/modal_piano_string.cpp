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

#include "modal_piano_string.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

// Natural logarithm of 1000, i.e. what a T60 is quoted against.
constexpr double SixtyDecibels = 6.907755278982137;

// Where the hammer's own spectrum turns over at the reference pitch, struck at the
// reference velocity. Everything above it falls away at the hammer slope, which is what
// makes a soft strike dull and a hard one bright without touching the string itself.
constexpr double HammerCornerBase = 380.0;
// How the corner rises towards the treble, where the hammers are smaller and harder.
constexpr double HammerCornerPitchExponent = 0.4;
// Velocity the corner is quoted at, and how hard it follows the strike. Fitted so that
// the spectral centroid tracks the reference's across the keyboard at both velocities.
constexpr double ReferenceVelocity = 0.65;
constexpr double VelocityBrightnessExponent = 1.25;

// Amplitudes of the two components a struck unison splits into. The in-phase one is
// louder and dies quickly, the out-of-phase one is what is left ringing afterwards. The
// pair is what produces the reference's two-stage decay, and the slight detune between
// them is what makes it beat.
constexpr double FastComponentAmplitude = 0.80;
constexpr double SlowComponentAmplitude = 0.42;

// Roughly how much of the fundamental's decay time each partial keeps, as a function of
// its index. The reference's eighth partial is gone in well under half the time its
// fundamental takes, and the falloff goes as the square of the index. This is the value
// fitted to the reference, which Brightness then multiplies either way.
constexpr double NeutralPartialDamping = 0.025;
// How far either side of that Brightness reaches, as a factor.
constexpr double PartialDampingSpan = 4.0;
// How hard Brightness tilts the strike itself, in powers of the partial's pitch. At full
// travel this is about nine decibels either way by the eighth partial, which is what makes
// the control audible on the note rather than only on its tail.
constexpr double BrightnessTiltDepth = 0.5;

// The damper bites harder the higher the partial, and never more than this many times
// faster than it does in the bass.
constexpr double DamperPitchCorner = 300.0;
constexpr double DamperMaxSpeedup = 4.0;

// Offset from equal temperament, in cents, that stretched octaves produce, sampled across
// the keyboard. Measured off the reference at the octaves and smoothed through the bass,
// where a single octave's reading is inside the tuning's own scatter.
struct RailsbackPoint
{
    double note;
    double cents;
};

constexpr std::array<RailsbackPoint, 8> RailsbackCurve { {
  { 21.0, -8.0 },
  { 36.0, -6.0 },
  { 48.0, -2.0 },
  { 60.0, 0.0 },
  { 72.0, 3.0 },
  { 84.0, 5.0 },
  { 96.0, 13.0 },
  { 108.0, 33.0 },
} };

} // namespace

double ModalPianoString::midiNoteToFreq(uint8_t note)
{
    return 440.0 * std::exp2((note - 69) / 12.0);
}

double ModalPianoString::inharmonicityCoefficient(uint8_t note)
{
    // Fitted to the reference an octave at a time. The wound bass strings all land near
    // the same value, and from the middle of the keyboard up the coefficient doubles
    // every eleven or so semitones as the strings get shorter and stiffer relative to
    // their length.
    constexpr double BassCoefficient = 2.3e-4;
    constexpr double DoublingSemitones = 11.7;
    constexpr double TrebleStart = 48.0;
    if (note <= TrebleStart) {
        return BassCoefficient;
    }
    return BassCoefficient * std::exp2((static_cast<double>(note) - TrebleStart) / DoublingSemitones);
}

double ModalPianoString::railsbackCents(uint8_t note)
{
    const double n = static_cast<double>(note);
    if (n <= RailsbackCurve.front().note) {
        return RailsbackCurve.front().cents;
    }
    if (n >= RailsbackCurve.back().note) {
        return RailsbackCurve.back().cents;
    }
    for (size_t i = 1; i < RailsbackCurve.size(); i++) {
        if (n <= RailsbackCurve[i].note) {
            const auto & lower = RailsbackCurve[i - 1];
            const auto & upper = RailsbackCurve[i];
            const double t = (n - lower.note) / (upper.note - lower.note);
            return lower.cents + t * (upper.cents - lower.cents);
        }
    }
    return 0.0;
}

double ModalPianoString::strikePosition(uint8_t note)
{
    // A seventh of the way along in the bass, closing on a twelfth at the top, which is
    // where the reference's missing partials sit: at middle C its eighth partial is more
    // than forty decibels down, and a hammer landing on an eighth of the string cannot
    // excite it at all.
    constexpr double BassDivisor = 7.0;
    constexpr double TrebleDivisor = 12.0;
    const double t = std::clamp((static_cast<double>(note) - 21.0) / 87.0, 0.0, 1.0);
    return 1.0 / (BassDivisor + t * (TrebleDivisor - BassDivisor));
}

void ModalPianoString::setSampleRate(double sampleRate)
{
    const double previous = m_sampleRate;

    DspComponent::setSampleRate(sampleRate);

    if (previous == sampleRate || previous <= 0.0 || sampleRate <= 0.0 || m_modeCount == 0) {
        return;
    }

    // A note struck before the backend reported its rate was given poles derived against
    // the old one. Both what a mode's pole angle means in hertz and what its radius means
    // in seconds are per sample, so moving them across is a matter of the ratio of the
    // rates, and the note keeps the pitch and the decay time it was struck with rather
    // than jumping when a buffer arrives at a different rate.
    const double ratio = previous / sampleRate;
    for (int i = 0; i < m_modeCount; i++) {
        Mode & mode = m_modes[i];
        mode.omega *= ratio;
        mode.radius = std::pow(mode.radius, ratio);
        mode.a1 = 2.0 * mode.radius * std::cos(mode.omega);
        mode.a2 = -mode.radius * mode.radius;
    }
    m_attackStep *= ratio;
}

void ModalPianoString::setModePole(Mode & mode, double frequency, double decayTime) const
{
    const double omega = 2.0 * std::numbers::pi * frequency / m_sampleRate;
    const double radius = std::exp(-SixtyDecibels / (std::max(decayTime, MinDecayTime) * m_sampleRate));
    mode.omega = omega;
    mode.radius = radius;
    mode.a1 = 2.0 * radius * std::cos(omega);
    mode.a2 = -radius * radius;
}

void ModalPianoString::trigger(uint8_t note, float velocity, const Settings & settings)
{
    reset();

    m_note = note;

    const double vel = std::clamp(static_cast<double>(velocity), 0.0, 1.0);
    const double stretch = std::exp2(static_cast<double>(settings.stretch) * railsbackCents(note) / 1200.0);
    const double f0 = midiNoteToFreq(note) * stretch;
    const double B = inharmonicityCoefficient(note) * (0.25 + static_cast<double>(settings.inharmonicity) * 1.5);
    const double p = strikePosition(note);

    // Everything above this would fold back, and the topmost partials are inaudible in any
    // case, so the bank stops short of Nyquist rather than aliasing into the note.
    const double ceiling = m_sampleRate * 0.47;
    const int wantedPartials = 8 + static_cast<int>(std::lround(static_cast<double>(settings.richness) * (MaxPartials - 8)));

    // The hammer's spectrum is what velocity acts on: a harder strike shortens the
    // contact and pushes the corner up, which is why the reference's spectral centroid
    // climbs by half again from velocity 64 to 100 without the note itself changing.
    const double hammerCorner = HammerCornerBase
      * std::pow(f0 / ReferenceFrequency, HammerCornerPitchExponent)
      * std::pow(std::max(vel, 0.05) / ReferenceVelocity, VelocityBrightnessExponent)
      * std::exp2((static_cast<double>(settings.hardness) - 0.5) * 2.0);

    // How long the fundamental takes to fall sixty decibels at this pitch, and how much of
    // that each partial keeps. Both come straight off the reference.
    const double decayScale = 0.35 + static_cast<double>(settings.decay) * 1.3;
    const double fundamentalDecay = std::clamp(
      decayScale * ReferenceDecayTime * std::pow(ReferenceFrequency / f0, DecayPitchExponent), MinDecayTime, MaxDecayTime);
    // Brightness is neutral in the middle, so that at the default the bank is exactly what
    // was fitted to the reference and the control tilts away from it in both directions.
    // It acts twice: on how much of the strike lands in the upper partials, and on how long
    // they hold on afterwards. Only the second of those used to be wired up, which left the
    // control doing nothing at all to the note as it was struck — where brightness is heard.
    const double brightnessTilt = (static_cast<double>(settings.brightness) - 0.5) * 2.0;
    const double partialDamping = NeutralPartialDamping * std::pow(PartialDampingSpan, -brightnessTilt);

    const double detuneRatio = std::exp2(static_cast<double>(settings.detune) * 2.0 / 1200.0);
    const double fastDecayScale = 1.0 / (1.0 + static_cast<double>(settings.doubleDecay) * 3.0);

    // Stiffness pushes every partial sharp, the first one included, so a bank built
    // straight from the series sounds sharp of the note asked for — nearly seven cents of
    // it on the top key, where B is largest. A tuner listens to the first partial and
    // tunes that to pitch, so the whole series is brought down by what stiffness does to
    // it. Where the partials sit relative to each other, which is what is heard as the
    // piano's colour, is untouched.
    const double fundamentalStretch = std::sqrt(1.0 + B);

    // Modes are laid down in ascending pitch, so that pruning the ones that have fallen
    // silent is a matter of shortening the bank rather than compacting it.
    double sumOfSquares = 0.0;
    for (int k = 1; k <= wantedPartials && m_modeCount + 2 <= MaxModes; k++) {
        const double kk = static_cast<double>(k);
        const double frequency = f0 * kk * std::sqrt(1.0 + B * kk * kk) / fundamentalStretch;
        if (frequency >= ceiling) {
            break;
        }

        // What the strike puts into this partial: the hammer's own spectrum, the comb the
        // strike position cuts into it, and how well the soundboard radiates it. The last
        // is what leaves the bottom octave's fundamental quieter than its second partial.
        const double hammer = 1.0 / (1.0 + std::pow(frequency / hammerCorner, HammerSlope));
        const double comb = std::abs(std::sin(kk * std::numbers::pi * p));
        const double radiated = frequency / std::hypot(frequency, RadiationCorner);
        const double tilt = std::pow(frequency / f0, BrightnessTiltDepth * brightnessTilt);
        const double amplitude = hammer * comb * radiated * radiated * tilt;

        const double decayTime = std::clamp(fundamentalDecay / (1.0 + partialDamping * (kk * kk - 1.0)), MinDecayTime, MaxDecayTime);

        if (k <= UnisonPartials && note >= LowestDoubleStrungNote) {
            // Struck together, the strings of a unison start in phase and lean on the
            // bridge as one, which drains them quickly; what is left is the out-of-phase
            // motion the bridge barely sees, and it rings on. Two modes with different
            // decay times and a hair of detune between them is that, and it is where the
            // reference's knee and its slow beating both come from.
            const double fastAmplitude = amplitude * FastComponentAmplitude;
            const double slowAmplitude = amplitude * SlowComponentAmplitude;
            setModePole(m_modes[m_modeCount], frequency * detuneRatio, decayTime * fastDecayScale);
            m_modes[m_modeCount].y1 = fastAmplitude;
            m_modeCount++;
            setModePole(m_modes[m_modeCount], frequency / detuneRatio, decayTime);
            m_modes[m_modeCount].y1 = slowAmplitude;
            m_modeCount++;
            sumOfSquares += fastAmplitude * fastAmplitude + slowAmplitude * slowAmplitude;
        } else {
            setModePole(m_modes[m_modeCount], frequency, decayTime);
            m_modes[m_modeCount].y1 = amplitude;
            m_modeCount++;
            sumOfSquares += amplitude * amplitude;
        }
    }

    // The two samples ahead of the strike, so that each mode opens from silence on its own
    // rising quarter cycle instead of stepping straight to its peak. Written from the
    // amplitude parked in y1 above.
    for (int i = 0; i < m_modeCount; i++) {
        Mode & mode = m_modes[i];
        const double amplitude = mode.y1;
        mode.y1 = -amplitude * std::sin(mode.omega) / mode.radius;
        mode.y2 = -amplitude * std::sin(2.0 * mode.omega) / (mode.radius * mode.radius);
    }

    // Normalised on the energy the strike put in, so that a note keeps the same level
    // whether it is carrying fifty partials or two, which is how the reference behaves
    // across all but its topmost key. Velocity then squares, matching the seven to eight
    // decibels the reference gains between velocity 64 and 100.
    const double norm = sumOfSquares > 0.0 ? 1.0 / std::sqrt(sumOfSquares) : 0.0;
    m_gain = norm * vel * vel;

    // A short ramp over the strike, long in the bass and brief at the top, so that the
    // modes do not all land on one sample. It is what an attack time is measured as.
    const double attackSeconds = (0.0005 + 0.0035 * std::pow(ReferenceFrequency / std::max(f0, 20.0), 0.6)) * (1.3 - 0.5 * vel);
    m_attackPhase = 0.0;
    m_attackStep = 1.0 / std::max(1.0, attackSeconds * m_sampleRate);

    m_releasing = false;
    m_pruneCounter = 0;
    m_energy = 1.0;
}

void ModalPianoString::release(float releaseTime)
{
    // A grand has no dampers over the top octave and a half, so the key going up there
    // leaves the note ringing exactly as it was.
    if (m_note >= LowestUndampedNote || m_releasing) {
        return;
    }

    m_releasing = true;

    const double damperDecay = std::max(0.03 + static_cast<double>(releaseTime) * 1.5, MinDecayTime);
    for (int i = 0; i < m_modeCount; i++) {
        Mode & mode = m_modes[i];
        // Felt takes the top of a note off first, so the damper is quicker the higher the
        // partial sits, up to a limit.
        const double frequency = mode.omega * m_sampleRate / (2.0 * std::numbers::pi);
        const double speedup = std::clamp(frequency / DamperPitchCorner, 1.0, DamperMaxSpeedup);
        const double wanted = damperDecay / speedup;
        const double radius = std::exp(-SixtyDecibels / (std::max(wanted, MinDecayTime) * m_sampleRate));
        // Only ever shorter: a partial already dying faster than the damper is left alone.
        if (radius < mode.radius) {
            mode.radius = radius;
            mode.a1 = 2.0 * radius * std::cos(mode.omega);
            mode.a2 = -radius * radius;
        }
    }
}

double ModalPianoString::nextSample()
{
    if (!isActive()) {
        return 0.0;
    }

    double out = 0.0;
    for (int i = 0; i < m_modeCount; i++) {
        Mode & mode = m_modes[i];
        const double y = mode.a1 * mode.y1 + mode.a2 * mode.y2;
        mode.y2 = mode.y1;
        mode.y1 = y;
        out += y;
    }

    out *= m_gain;

    if (m_attackPhase < 1.0) {
        out *= 0.5 * (1.0 - std::cos(std::numbers::pi * m_attackPhase));
        m_attackPhase += m_attackStep;
    }

    // The partials at the top go first, so once in a while the bank is shortened to the
    // ones still ringing. Everything below stays exactly where it is, which is why the
    // modes were laid down in ascending pitch.
    if (++m_pruneCounter >= PruneInterval) {
        m_pruneCounter = 0;
        while (m_modeCount > 0) {
            const Mode & mode = m_modes[m_modeCount - 1];
            if (std::abs(mode.y1) > ModeSilenceThreshold || std::abs(mode.y2) > ModeSilenceThreshold) {
                break;
            }
            m_modeCount--;
        }
    }

    // Exponential moving average of the squared output, so that a note that has fallen
    // below hearing stops costing anything even while its lowest modes are still moving.
    const double coeff = 20.0 / m_sampleRate;
    m_energy += (out * out - m_energy) * coeff;

    return out;
}

bool ModalPianoString::isActive() const
{
    return m_modeCount > 0 && m_energy > SilenceThreshold;
}

uint8_t ModalPianoString::note() const
{
    return m_note;
}

int ModalPianoString::activeModeCount() const
{
    return m_modeCount;
}

void ModalPianoString::reset()
{
    for (int i = 0; i < m_modeCount; i++) {
        m_modes[i] = Mode {};
    }
    m_modeCount = 0;
    m_gain = 0.0;
    m_attackPhase = 1.0;
    m_attackStep = 0.0;
    m_releasing = false;
    m_pruneCounter = 0;
    m_energy = 0.0;
}

} // namespace noteahead
