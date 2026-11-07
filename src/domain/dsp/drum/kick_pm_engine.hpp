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

#ifndef KICK_PM_ENGINE_HPP
#define KICK_PM_ENGINE_HPP

#include "../base_rate_source.hpp"
#include "../cascaded_svf.hpp"
#include "drum_engine.hpp"

#include <array>
#include <cstdint>
#include <random>

namespace noteahead {

//! Physical modelling bass drum engine.
//!
//! Models the parts of an acoustic kick that real recordings show to matter:
//! 1. Batter head: a bank of decaying modes. The air inside the drum loads them, which pulls the
//!    overtones down into a cluster at 1.3-2x the fundamental rather than a free membrane's ratios.
//! 2. Tension modulation: a head stretched by a hit is tighter, so it starts sharp and settles as it
//!    decays. The rise follows the head's own amplitude, so a harder hit starts sharper and a long
//!    undamped drum stays sharp longer, as real ones do.
//! 3. Front head: a second set of modes near the batter's, coupled through the air inside. Solid it
//!    adds the long boom and the beating of two heads; ported it is short and quiet.
//! 4. Damping: a pillow against the heads, which shortens a kick faster than exponentially.
//! 5. Beater: felt to hard. The contact time decides how much of the higher modes the hit reaches,
//!    and the click it makes.
//! 6. Shell knock, a microphone from inside the drum to outside the front head, and drive.
class KickPmEngine : public DrumEngine
{
public:
    static constexpr int BatterModes = 8;
    static constexpr int FrontModes = 4;
    static constexpr int TotalModes = BatterModes + FrontModes;

    KickPmEngine();
    ~KickPmEngine() override;

    void trigger(float velocity) override;
    float nextSample() override;
    void nextSample(float & left, float & right);
    bool isActive() const override;
    void reset() override;
    void stop() override;

    //! The settled pitch of the batter head's fundamental.
    void setTune(float tuneHz);
    //! T60 of the fundamental with no damping.
    void setDecay(float decaySeconds);
    //! How fast the higher modes die against the fundamental: 0 dark, 1 bright.
    void setTone(float tone);
    //! How sharp a full-velocity hit starts, 0..1: 1 is 60 % above the settled pitch.
    void setPitchDrop(float drop);
    //! A pillow against the heads, 0..1. 0 leaves the drum ringing.
    void setDamping(float damping);

    //! Felt (0) to hard plastic or wood (1).
    void setBeater(float beater);
    //! Level of the beater's click.
    void setClick(float click);
    //! Where the beater hits, centre (0) to near the rim (1).
    void setPosition(float position);

    //! The front head, ported (0) to solid (1).
    void setFrontHead(float frontHead);
    //! The front head's pitch against the batter's, as a ratio.
    void setFrontTune(float ratio);
    //! Level of the shell's knock.
    void setShell(float shell);
    //! The microphone, inside the drum near the beater (0) to outside the front head (1).
    void setMic(float mic);
    void setDrive(float drive);
    //! Level of the head's dense upper modes, which a mode bank of eight leaves out: the broad
    //! 300 Hz-5 kHz ring of a real kick that outlasts the beater.
    void setRing(float ring);
    //! T60 of that ring.
    void setRingDecay(float seconds);

    void updateRates();

private:
    struct Mode
    {
        double re { 0.0 };
        double im { 0.0 };
        double tailRe { 0.0 };
        double tailIm { 0.0 };
        double ratio { 1.0 };
        double damping { 0.0 };
        double amplitude { 0.0 };
    };

    void initModes();
    void triggerModes(float velocity);
    //! How much of a mode at @p hz a beater in contact for the beater's contact time reaches: the
    //! spectrum of a half-sine force pulse.
    double contactWeight(double hz) const;
    double contactSeconds() const;

    std::array<Mode, TotalModes> m_modes {};

    float m_tune { 55.0f };
    float m_decay { 1.0f };
    float m_tone { 0.5f };
    float m_pitchDrop { 0.4f };
    float m_damping { 0.5f };

    float m_beater { 0.5f };
    float m_click { 0.5f };
    float m_position { 0.2f };

    float m_frontHead { 0.5f };
    float m_frontTune { 1.05f };
    float m_shell { 0.3f };
    float m_mic { 0.5f };
    float m_drive { 0.0f };

    bool m_active { false };
    float m_velocity { 1.0f };
    //! The fundamental's amplitude right after a full-velocity hit: what the tension rise is
    //! measured against.
    double m_referenceAmplitude { 1.0 };

    // Pillow: exp(-(t / tau)^2), run as a recurrence
    double m_pillowEnv { 1.0 };
    double m_pillowRatio { 1.0 };
    double m_pillowRatioStep { 1.0 };
    double m_pillowFirstRatio { 1.0 };

    // Beater click: noise through a band set by the beater, decaying with it
    double m_clickEnv { 0.0 };
    double m_clickDecayRate { 1.0 };
    CascadedSvf m_clickHp {};
    CascadedSvf m_clickLp {};

    CascadedSvf m_shellResonator {};

    float m_ring { 0.3f };
    float m_ringDecay { 0.3f };
    double m_ringEnv { 0.0 };
    double m_ringDecayRate { 1.0 };
    CascadedSvf m_ringHp {};
    CascadedSvf m_ringLp {};

    double m_tailDamping { 0.0 };

    static constexpr uint32_t RngSeed { 20271 };
    std::mt19937 m_rng { RngSeed };
    std::uniform_real_distribution<float> m_dist { -1.0f, 1.0f };
    BaseRateSource m_noiseBank {};

    double m_lastSampleRate { 0.0 };
};

} // namespace noteahead

#endif // KICK_PM_ENGINE_HPP
