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

#include "modal_piano_string_v3.hpp"

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
constexpr double HammerCornerBase = 275.0;
// How the corner rises towards the treble, where the hammers are smaller and harder.
constexpr double HammerCornerPitchExponent = 0.4;
// Velocity the corner is quoted at, and how hard it follows the strike. Quoted at
// velocity 64, which is where the reference survey's spectrum is matched most closely,
// and the exponent is what carries it up to velocity 100: over the whole keyboard the
// reference's centroid rises by about a sixth between the two.
constexpr double ReferenceVelocity = 0.5;
constexpr double VelocityBrightnessExponent = 0.85;

// Amplitudes of the two components a struck unison splits into. The in-phase one leans
// on the bridge as one and is drained promptly; the out-of-phase one is what is left
// ringing afterwards. The prompt one carries far more of the note at the top of the
// keyboard than in the middle, which is what the reference's attack-over-tail ratio
// climbing from some twelve decibels at C4 to forty at C7 is.
constexpr double PromptComponentAmplitude = 0.10;
constexpr double PromptAmplitudePitchExponent = 1.6;
constexpr double MaxPromptAmplitude = 1.2;
constexpr double SlowComponentAmplitude = 0.42;

// How long the prompt component takes to fall sixty decibels, at the reference pitch.
// This is a time and not a fraction of the string's own decay: what drains it is the
// bridge, not the string, so it stays about as brief at the bottom of the keyboard as at
// the top. Making it a fraction left it alive well into the sustain, which is what used
// to leave every note above the middle of the keyboard dying twice too fast.
constexpr double PromptDecayTime = 0.35;
constexpr double PromptDecayPitchExponent = 0.3;

// The reference holds within a couple of decibels of itself over almost the whole
// keyboard, and gives way only at the two ends: the bottom octave sits a little under the
// middle, and the last octave falls away where the strings are too short to carry the
// strike. Between them the level is flat, so a note must not get quieter for being high.
constexpr double BassTrimEndNote = 55.0;
constexpr double BassTrimStartNote = 26.0;
constexpr double BassTrimDb = 3.0;
constexpr double TopRolloffStartNote = 93.0;
constexpr double TopRolloffEndNote = 108.0;
constexpr double TopRolloffDb = 5.0;

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
//! Range of the Attack control, as octaves of ramp length either side of the fitted one. The fitted
//! ramp is only a few milliseconds, so a narrow span moves nothing a listener would notice: this
//! reaches from a hard click at one end to a distinctly felt onset of some fifty milliseconds at
//! the other, with the fitted ramp in the middle.
constexpr double AttackScaleSpan = 8.0;

//! Longest the damper will wait for a strike to finish developing. Without a bound, a soft attack
//! would keep a staccato note ringing for as long as its own onset lasts.
constexpr double MaxDamperWaitSeconds = 0.04;

//! How long the damper takes to land after the key comes up. A real one has to travel and then
//! bed in, and in the bass it is heavier and slower still; what matters here is only that it is
//! never instant, because instant takes the strike with it.
constexpr double DamperEngagementSeconds = 0.018;

constexpr double DamperPitchCorner = 300.0;
constexpr double DamperMaxSpeedup = 4.0;

// Offset from equal temperament, in cents, that stretched octaves produce, sampled across
// the keyboard. Re-measured off the chromatic reference survey, which reaches far further
// into the bass than the octave-by-octave one did: the bottom key is tuned a third of a
// semitone flat, not the eighth of one a few octaves read as.
struct RailsbackPoint
{
    double note;
    double cents;
};

constexpr std::array<RailsbackPoint, 8> RailsbackCurve { {
  { 21.0, -34.0 },
  { 33.0, -11.0 },
  { 42.0, -4.0 },
  { 60.0, 0.0 },
  { 72.0, 3.0 },
  { 84.0, 5.0 },
  { 96.0, 13.0 },
  { 108.0, 33.0 },
} };

// Stiffness coefficient B, fitted to the reference an octave-group at a time. The curve
// is a shallow U: the wound bass strings are thick relative to their length and run
// strongly inharmonic, the middle of the keyboard is the nearest to a harmonic series,
// and the treble climbs again as the strings get short. A flat bass, which is what the
// octave survey read, misses the bottom of the keyboard by a factor of nearly twenty.
struct InharmonicityPoint
{
    double note;
    double coefficient;
};

constexpr std::array<InharmonicityPoint, 14> InharmonicityCurve { {
  { 21.0, 3.5e-3 },
  { 24.0, 2.2e-3 },
  { 27.0, 1.05e-3 },
  { 30.0, 8.8e-4 },
  { 34.0, 4.4e-4 },
  { 42.0, 2.6e-4 },
  { 53.0, 2.25e-4 },
  { 60.0, 3.5e-4 },
  { 66.0, 4.7e-4 },
  { 72.0, 8.5e-4 },
  { 78.0, 1.5e-3 },
  { 84.0, 2.6e-3 },
  { 96.0, 6.5e-3 },
  { 108.0, 1.6e-2 },
} };

} // namespace

double ModalPianoStringV3::midiNoteToFreq(uint8_t note)
{
    return 440.0 * std::exp2((note - 69) / 12.0);
}

double ModalPianoStringV3::inharmonicityCoefficient(uint8_t note)
{
    // Read off the fitted curve, geometrically between its points: the coefficient spans
    // two orders of magnitude across the keyboard, so it is the ratio between neighbours
    // and not the difference that has to come out smooth.
    const double n = static_cast<double>(note);
    if (n <= InharmonicityCurve.front().note) {
        return InharmonicityCurve.front().coefficient;
    }
    if (n >= InharmonicityCurve.back().note) {
        return InharmonicityCurve.back().coefficient;
    }
    for (size_t i = 1; i < InharmonicityCurve.size(); i++) {
        if (n <= InharmonicityCurve[i].note) {
            const auto & lower = InharmonicityCurve[i - 1];
            const auto & upper = InharmonicityCurve[i];
            const double t = (n - lower.note) / (upper.note - lower.note);
            return lower.coefficient * std::pow(upper.coefficient / lower.coefficient, t);
        }
    }
    return InharmonicityCurve.back().coefficient;
}

double ModalPianoStringV3::keyLevel(uint8_t note)
{
    const double n = static_cast<double>(note);

    double db = 0.0;
    if (n < BassTrimEndNote) {
        const double t = std::clamp((BassTrimEndNote - n) / (BassTrimEndNote - BassTrimStartNote), 0.0, 1.0);
        db -= BassTrimDb * t;
    }
    if (n > TopRolloffStartNote) {
        const double t = std::min((n - TopRolloffStartNote) / (TopRolloffEndNote - TopRolloffStartNote), 1.0);
        db -= TopRolloffDb * t;
    }
    return std::pow(10.0, db / 20.0);
}

double ModalPianoStringV3::railsbackCents(uint8_t note)
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

double ModalPianoStringV3::strikePosition(uint8_t note)
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

void ModalPianoStringV3::setSampleRate(double sampleRate)
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

void ModalPianoStringV3::setModePole(Mode & mode, double frequency, double decayTime) const
{
    const double omega = 2.0 * std::numbers::pi * frequency / m_sampleRate;
    const double radius = std::exp(-SixtyDecibels / (std::max(decayTime, MinDecayTime) * m_sampleRate));
    mode.omega = omega;
    mode.radius = radius;
    mode.a1 = 2.0 * radius * std::cos(omega);
    mode.a2 = -radius * radius;
}

void ModalPianoStringV3::trigger(uint8_t note, float velocity, const Settings & settings)
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
    const double pitchDecay = std::max(
      ReferenceDecayTime * std::pow(ReferenceFrequency / f0, DecayPitchExponent), MinPitchDecayTime);
    const double fundamentalDecay = std::clamp(decayScale * pitchDecay, MinDecayTime, MaxDecayTime);
    // Brightness is neutral in the middle, so that at the default the bank is exactly what
    // was fitted to the reference and the control tilts away from it in both directions.
    // It acts twice: on how much of the strike lands in the upper partials, and on how long
    // they hold on afterwards. Only the second of those used to be wired up, which left the
    // control doing nothing at all to the note as it was struck — where brightness is heard.
    const double brightnessTilt = (static_cast<double>(settings.brightness) - 0.5) * 2.0;
    const double partialDamping = NeutralPartialDamping * std::pow(PartialDampingSpan, -brightnessTilt);

    const double detuneRatio = std::exp2(static_cast<double>(settings.detune) * 2.0 / 1200.0);

    // The prompt component: how long the bridge takes to drain the in-phase motion, and
    // how much of the strike went into it. Both are what the reference's attack sounds
    // like — a knock over a tail in the treble, barely a knee in the middle.
    const double promptDecay = std::clamp(
      PromptDecayTime * std::pow(ReferenceFrequency / f0, PromptDecayPitchExponent)
        / (0.25 + static_cast<double>(settings.doubleDecay) * 1.5),
      MinDecayTime, fundamentalDecay);
    const double promptAmplitude = std::min(
      PromptComponentAmplitude * std::pow(f0 / ReferenceFrequency, PromptAmplitudePitchExponent), MaxPromptAmplitude);

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
        // strike position cuts into it, and how well the bass end reaches the pickup. The last
        // is what leaves the bottom octave's fundamental quieter than its second partial.
        const double hammer = 1.0 / (1.0 + std::pow(frequency / hammerCorner, HammerSlope));
        // Signed, unlike V2. A node either side of the strike point puts the partial in
        // antiphase, and fabs() threw that away.
        const double comb = std::sin(kk * std::numbers::pi * p);
        const double radiated = frequency / std::hypot(frequency, RadiationCorner);
        const double tilt = std::pow(frequency / f0, BrightnessTiltDepth * brightnessTilt);
        // The piezo reads force at a rigid termination, where consecutive partials pull in
        // opposite directions. Levels are untouched; what moves is the shape of the strike.
        const double pickupSign = (k & 1) ? -1.0 : 1.0;
        const double amplitude = pickupSign * hammer * comb * radiated * radiated * tilt;

        const double decayTime = std::clamp(fundamentalDecay / (1.0 + partialDamping * (kk * kk - 1.0)), MinDecayTime, MaxDecayTime);

        if (k <= UnisonPartials && note >= LowestDoubleStrungNote) {
            // Struck together, the strings of a unison start in phase and lean on the
            // bridge as one, which drains them quickly; what is left is the out-of-phase
            // motion the bridge barely sees, and it rings on. Two modes with different
            // decay times and a hair of detune between them is that, and it is where the
            // reference's knee and its slow beating both come from.
            const double fastAmplitude = amplitude * promptAmplitude;
            const double slowAmplitude = amplitude * SlowComponentAmplitude;
            setModePole(m_modes[m_modeCount], frequency * detuneRatio, promptDecay);
            m_modes[m_modeCount].y1 = fastAmplitude;
            m_modeCount++;
            setModePole(m_modes[m_modeCount], frequency / detuneRatio, decayTime);
            m_modes[m_modeCount].y1 = slowAmplitude;
            m_modeCount++;
            sumOfSquares += fastAmplitude * fastAmplitude + slowAmplitude * slowAmplitude;
        } else {
            // Above the split, one mode stands in for the pair the note actually has. It has to
            // carry what the pair carried: emitting it at full amplitude stepped the series by
            // some six decibels at the crossover, right where the ear is most attentive to it.
            const double pairWeight = note >= LowestDoubleStrungNote
              ? promptAmplitude + SlowComponentAmplitude
              : 1.0;
            const double standInAmplitude = amplitude * pairWeight;
            setModePole(m_modes[m_modeCount], frequency, decayTime);
            m_modes[m_modeCount].y1 = standInAmplitude;
            m_modeCount++;
            sumOfSquares += standInAmplitude * standInAmplitude;
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
    // across all but its topmost keys — where the level does fall away, and steeply.
    // Velocity squares, matching the eight and a half decibels the reference gains
    // between velocity 64 and 100.
    const double norm = sumOfSquares > 0.0 ? 1.0 / std::sqrt(sumOfSquares) : 0.0;
    m_gain = norm * vel * vel * keyLevel(note);

    // A short ramp over the strike, long in the bass and brief at the top, so that the
    // modes do not all land on one sample. It is what an attack time is measured as.
    // Centred on 1, so the middle of the control is the ramp the bank was fitted with.
    const double attackScale = std::exp2((static_cast<double>(settings.attack) - 0.5) * AttackScaleSpan);
    const double attackSeconds = (0.0005 + 0.0035 * std::pow(ReferenceFrequency / std::max(f0, 20.0), 0.6)) * (1.3 - 0.5 * vel) * attackScale;
    m_attackPhase = 0.0;
    m_attackStep = 1.0 / std::max(1.0, attackSeconds * m_sampleRate);

    m_releasing = false;
    m_pruneCounter = 0;
    m_energy = 1.0;
}

void ModalPianoStringV3::release(float releaseTime)
{
    // A grand has no dampers over the top octave and a half, so the key going up there
    // leaves the note ringing exactly as it was.
    if (m_note >= LowestUndampedNote || m_releasing) {
        return;
    }

    m_releasing = true;

    // The damper is scheduled rather than applied. It has to land no sooner than the strike has
    // finished developing, or it takes the note's own attack away with it: the ramp at the start of
    // a bass note is the best part of ten milliseconds on its own.
    const int engagement = static_cast<int>(DamperEngagementSeconds * m_sampleRate);
    const int remainingAttack = m_attackStep > 0.0 && m_attackPhase < 1.0
      ? static_cast<int>(std::ceil((1.0 - m_attackPhase) / m_attackStep))
      : 0;
    m_damperCountdown = std::max(engagement, std::min(remainingAttack, static_cast<int>(MaxDamperWaitSeconds * m_sampleRate)));
    m_pendingReleaseTime = releaseTime;
}

void ModalPianoStringV3::applyDamper(float releaseTime)
{
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

double ModalPianoStringV3::nextSample()
{
    if (!isActive()) {
        return 0.0;
    }

    if (m_damperCountdown > 0 && --m_damperCountdown == 0) {
        applyDamper(m_pendingReleaseTime);
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

bool ModalPianoStringV3::isActive() const
{
    return m_modeCount > 0 && m_energy > SilenceThreshold;
}

uint8_t ModalPianoStringV3::note() const
{
    return m_note;
}

int ModalPianoStringV3::activeModeCount() const
{
    return m_modeCount;
}

void ModalPianoStringV3::reset()
{
    for (int i = 0; i < m_modeCount; i++) {
        m_modes[i] = Mode {};
    }
    m_modeCount = 0;
    m_gain = 0.0;
    m_attackPhase = 1.0;
    m_attackStep = 0.0;
    m_damperCountdown = 0;
    m_pendingReleaseTime = 0.0f;
    m_releasing = false;
    m_pruneCounter = 0;
    m_energy = 0.0;
}

} // namespace noteahead
