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

#ifndef GLOTTAL_SOURCE_HPP
#define GLOTTAL_SOURCE_HPP

#include "../dsp_component.hpp"
#include "../poly_blep_oscillator.hpp"

#include <random>

namespace noteahead {

//! The sound the vocal folds make, before the tract has done anything to it.
//!
//! A Rosenberg pulse: the folds blow open over one stretch of the period, snap shut over a shorter
//! one, and stay shut for the rest. What it produces is glottal *flow*; FormantVoice differentiates
//! it into the flow derivative with the same filter that stands for the lip radiation, which is why
//! nothing here has to.
//!
//! The point of it over the sawtooth it replaces is the open quotient -- the share of the period the
//! folds are apart. A saw has no such parameter, and the tilt filter that stood in for one is a
//! uniform spectral slope: it can make a voice darker or brighter, but it cannot change the level of
//! the first harmonic relative to the second, and that ratio is most of what separates a pressed
//! voice from a breathy one. It is also most of what separates a man's voice from a woman's, which
//! is why a tract length and a low pass between them did not make one.
//!
//! Jitter and shimmer live here rather than in the voice because they are properties of a *cycle*:
//! each period is drawn a little long or short and a little loud or quiet, and a perturbation that
//! moved within a period would be a modulation rather than the irregularity real folds have.
class GlottalSource : public DspComponent
{
public:
    GlottalSource();

    //! The waveform the folds are modelled by.
    enum class Model
    {
        //! The sawtooth this had before the Rosenberg pulse existed. Kept because it is what every
        //! project saved until now was written against, and those have to keep sounding the same.
        Saw = 0,
        Rosenberg = 1
    };

    void setSampleRate(double sampleRate) override;
    void setFrequency(double frequency);
    void setModel(Model model);

    //! Share of the period the folds are open, 0..1 -- in practice 0.3 to 0.8.
    //!
    //! Low is pressed: a short, sharp pulse, rich in harmonics and buzzy. High is breathy: a long,
    //! rounded one whose energy is nearly all in the first harmonic or two. Around 0.5 is the modal
    //! voice most speech is in.
    void setOpenQuotient(double openQuotient);

    //! How much longer the opening stretch is than the closing one.
    //!
    //! The asymmetry is what puts energy above the first harmonic at all: a symmetric pulse is close
    //! to a raised cosine and has almost nothing above its fundamental. Real folds close faster than
    //! they open, and the faster they close the brighter the voice.
    void setSpeedQuotient(double speedQuotient);

    //! Cycle-to-cycle wander of the period, as a fraction. Real voices measure 0.003 to 0.01; the
    //! ear reads none at all as a machine.
    void setJitter(double jitter);
    //! Cycle-to-cycle wander of the pulse height, as a fraction. Runs a few times deeper than the
    //! jitter in measured speech, which is the ratio setJitter()'s caller is expected to keep.
    void setShimmer(double shimmer);

    double nextSample();

    //! How far open the folds are on the sample just produced, scaled so that its mean over a
    //! period is 1.
    //!
    //! What aspiration is modulated by. Breath is air passing *through* the gap, so it is loudest
    //! when the gap is widest and absent while the folds are shut. Mixed in flat instead, it is a
    //! hiss sitting behind a tone rather than the sound of a breathy voice.
    //!
    //! Unit mean rather than unit peak so that pulsing the breath does not also turn it down: the
    //! caller's breathiness setting is a level, and it has to go on meaning the same level once the
    //! noise it scales is being gated by a pulse that is shut most of the time. One is what the saw
    //! model returns throughout, which is the flat aspiration it always had.
    double openness() const;

    void reset();

private:
    //! Draws the jitter and shimmer for the period about to start.
    void beginPeriod();
    void updatePhaseStep();
    //! The Rosenberg flow at @p phase, 0..1 through the period.
    double rosenberg(double phase) const;
    //! Recomputes what depends on the pulse's shape: its mean flow, which openness() divides by, and
    //! the part of the excitation scale the open quotient decides. Both derive in closed form, so the
    //! shape can change without nextSample() ever integrating anything.
    void updateShapeNorms();
    //! Recomputes where the part of the excitation scale the pitch decides is heading. Kept apart
    //! from the shape's because the caller sets the frequency once a frame -- the intonation contour
    //! and the vibrato both move it -- and the shape only when a control is touched.
    void updateFrequencyScale();

    PolyBlepOscillator m_saw;

    Model m_model { Model::Rosenberg };
    double m_frequency { 110.0 };
    double m_openQuotient { 0.5 };
    double m_speedQuotient { 2.5 };
    double m_jitter { 0.0 };
    double m_shimmer { 0.0 };

    double m_phase { 0.0 };
    double m_phaseStep { 0.0 };
    //! The jitter and shimmer drawn for the period now running. Held for its whole length, because
    //! a period that changed length halfway through would not have one.
    double m_periodScale { 1.0 };
    double m_amplitude { 1.0 };
    double m_openness { 1.0 };
    double m_meanFlow { 1.0 };
    //! What the pulse is scaled by so that its level does not follow its shape.
    //!
    //! The open quotient is a tone control: it decides how a voice is produced, not how loud it is.
    //! Left alone the level follows it, and the five voice types spread over 8 dB -- which a user
    //! reads as a broken preset rather than as a different larynx, and which no fader fixes because
    //! it moves with the knob.
    //!
    //! What is held constant is the steepest rate the flow falls at as the folds slam shut. That is
    //! the standard predictor of how loud a voice is, and it is the right one here for the same
    //! reason it is there: the tract is rung by the *closing*, so the excitation is a slope rather
    //! than an area. Normalising the pulse's energy instead made the spread worse, because a short
    //! pulse carries its energy higher up and so loses less of it to the tilt that follows.
    double m_shapeScale { 1.0 };
    //! The pitch's part of the excitation scale, and where it is heading.
    //!
    //! Glided rather than applied outright, because it is a gain on the pulse and the pitch does not
    //! only drift: a note-on sets it immediately, so a jump of an octave stepped the source by 6 dB
    //! between one sample and the next, in the middle of a pulse. That is a click, and it is what
    //! playing the device from a keyboard sounded like. Nothing is lost by gliding it -- it corrects
    //! for where a pulse's harmonics sit against a filter corner, which is not a thing that has to
    //! be right within a millisecond of the note starting.
    double m_frequencyScale { 1.0 };
    double m_frequencyScaleTarget { 1.0 };
    double m_frequencyScaleCoefficient { 1.0 };
    //! Whether a sample has been produced since the last reset.
    //!
    //! What tells a pitch that is *changing* from one that is merely being chosen. There is nothing
    //! to glide from before the first pulse, and gliding anyway would start a phrase at the level the
    //! previous one ended on.
    bool m_started { false };

    //! Seeded rather than random-seeded, so a render of the same project twice is the same file.
    std::mt19937 m_rng { 0x51EEC4 };
    std::uniform_real_distribution<double> m_deviation { -1.0, 1.0 };
};

} // namespace noteahead

#endif // GLOTTAL_SOURCE_HPP
