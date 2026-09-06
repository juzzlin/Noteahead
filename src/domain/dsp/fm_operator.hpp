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

#ifndef FM_OPERATOR_HPP
#define FM_OPERATOR_HPP

#include "adsr_envelope.hpp"
#include "dsp_component.hpp"

#include <cstddef>
#include <numbers>
#include <string>
#include <vector>

namespace noteahead {

//! One operator of the FM synth: an oscillator, its own envelope, and a self-feedback path.
//!
//! This is phase modulation, not frequency modulation. The modulating signal is added to the
//! oscillator's phase rather than to its frequency, which is what Yamaha's chips did and what
//! everyone still calls FM. The difference matters: true frequency modulation walks the pitch
//! around as the index rises, because the modulator's average is integrated into the phase along
//! with everything else. Phase modulation leaves the pitch exactly where the note put it.
//!
//! The operator does not know whether it is a carrier or a modulator -- that is the algorithm's
//! business. It takes a phase offset in, and hands its output back for whoever wants it.
class FmOperator : public DspComponent
{
public:
    //! Serialized as a raw ordinal, so this is append-only: inserting a value would silently change
    //! the waveform of every operator in every project saved before the change.
    //!
    //! The set is the eight of the OPL chips: a sine, and seven ways of gating, rectifying and
    //! doubling it. Square and Saw are the two that are not sine-derived, and both are drawn as
    //! curves rather than as literal steps -- see the note on the tables in the .cpp. Quarter Sine
    //! is the one waveform that does step: its drop to zero a quarter of the way through is what
    //! the waveform is, so it is left alone and is correspondingly the dirtiest of the eight.
    enum class Waveform
    {
        Sine = 0,
        HalfSine,
        AbsSine,
        QuarterSine,
        AlternatingSine,
        AlternatingAbsSine,
        Square,
        Saw
    };

    static constexpr size_t WaveformCount { 8 };

    static std::vector<std::string> waveformNames();

    //! Phase, in cycles, that a full-scale modulator adds when its level is wide open.
    //!
    //! Eight radians. Past roughly this the sidebands stop getting brighter and start folding back
    //! down over the ones already there, so the top of the knob would sound no different from the
    //! middle of it -- only noisier.
    static constexpr double MaxIndexCycles { 8.0 / (2.0 * std::numbers::pi) };

    FmOperator();

    void setSampleRate(double sampleRate) override;

    void setFrequency(double frequency);
    double frequency() const;

    void setWaveform(Waveform waveform);
    Waveform waveform() const;

    //! Output level, 0..1, applied after the envelope. Tapering the knob that feeds this is the
    //! device's business: the operator is linear in it.
    void setLevel(double level);
    double level() const;

    //! Self-feedback depth, in cycles of phase per unit of output. Zero switches the path off.
    void setFeedback(double cycles);
    double feedback() const;

    AdsrEnvelope & envelope();
    const AdsrEnvelope & envelope() const;

    void trigger();
    void release();

    //! Back to silence: phase, envelope and the feedback history all cleared.
    void reset();

    //! Restarts the phase without touching anything else. What a fresh note uses to make every
    //! voice of a stack start alike; a retrigger of a sounding voice must not, or it clicks.
    void sync(double phase);

    //! Advances one sample. @p phaseMod is the modulation coming in from the algorithm, in cycles.
    double nextSample(double phaseMod = 0.0);

    //! The sample nextSample() last returned.
    double value() const;

    double phase() const;

    //! True once the envelope can no longer produce anything, so the operator may be dropped.
    bool isSilent() const;

private:
    //! Long enough that the linear interpolation between entries is inaudible, short enough that
    //! all eight waveforms together stay well inside a data cache.
    static constexpr size_t TableSize { 4096 };

    //! All eight waveforms end to end, built once and shared. Waveform @c w occupies
    //! [w * TableSize, (w + 1) * TableSize).
    static const std::vector<float> & tables();

    double lookup(double phase) const;

    void updatePhaseStep();

    AdsrEnvelope m_envelope;

    Waveform m_waveform { Waveform::Sine };
    double m_frequency { 440.0 };
    double m_level { 1.0 };
    double m_feedback { 0.0 };

    double m_phase { 0.0 };
    double m_phaseStep { 0.0 };
    double m_value { 0.0 };

    //! The two most recent outputs. Feedback runs off their average rather than off the last one
    //! alone -- see the note in nextSample().
    double m_previous { 0.0 };
    double m_previous2 { 0.0 };
};

} // namespace noteahead

#endif // FM_OPERATOR_HPP
