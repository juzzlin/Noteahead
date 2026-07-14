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
//! 1. Batter head: a bank of decaying modes with lug-asymmetry doublets. Air inside the drum
//!    pulls the low overtones up while upper modes reach past 5x the fundamental.
//! 2. Quadratic tension modulation: a head stretched by a hit rises in pitch with displacement
//!    squared and settles early rather than sagging through the whole tail.
//! 3. Front head: a second set of modes near the batter's, driven across the shell air transit
//!    delay. Solid it adds the long boom and beating of two heads; ported it is short and quiet.
//! 4. Two-stage damping: a pillow damps the batter impact quickly into the slower cavity/front tail.
//! 5. Beater: Hertzian felt-to-hard contact time shapes upper modes, finite attack ramp, low-mid
//!    thud and high slap click nonlinearly with velocity.
//! 6. Shell knock, inside-to-outside microphone balance, comb-colored upper-mode ring, and drive.
class KickPmEngine : public DrumEngine
{
public:
    static constexpr int BatterModes = 14;
    static constexpr int FrontModes = 6;
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
    //! How sharp a full-velocity hit starts, 0..1.
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
    //! Level of the head's dense upper modes, which the discrete mode bank leaves out: the broad
    //! upper ring of a real kick that outlasts the beater.
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
    void updateVelocityFilters(double velocity);
    //! How much of a mode at @p hz a beater in contact for contactSeconds(velocity) reaches:
    //! the spectrum of a half-sine force pulse.
    double contactWeight(double hz, double velocity) const;
    double contactSeconds(double velocity) const;

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

    // Fast batter impact punch & local geometric tension relaxation
    double m_impactEnv { 0.0 };
    double m_impactDecayRate { 1.0 };
    double m_impactAmount { 0.0 };
    double m_tensionEnv { 0.0 };
    double m_tensionDecayRate { 1.0 };

    // Beater contact attack ramp & acoustic shell-transit delay to front head
    double m_headAttack { 1.0 };
    double m_headAttackCoeff { 1.0 };
    double m_frontAttack { 1.0 };
    double m_frontAttackCoeff { 1.0 };
    std::array<double, 512> m_frontDelayBuf {};
    size_t m_frontDelayPos { 0 };
    size_t m_frontDelaySamples { 1 };

    // Beater click: high slap + low-mid thud, noise-driven
    double m_clickEnv { 0.0 };
    double m_clickDecayRate { 1.0 };
    CascadedSvf m_clickHp {};
    CascadedSvf m_clickLp {};
    double m_thudEnv { 0.0 };
    double m_thudDecayRate { 1.0 };
    CascadedSvf m_thudFilter {};

    CascadedSvf m_shellResonator {};

    float m_ring { 0.3f };
    float m_ringDecay { 0.3f };
    double m_ringEnv { 0.0 };
    double m_ringDecayRate { 1.0 };
    CascadedSvf m_ringHp {};
    CascadedSvf m_ringLp {};
    std::array<double, 256> m_ringCombBuf {};
    size_t m_ringCombPos { 0 };
    size_t m_ringCombDelay { 1 };

    double m_tailDamping { 0.0 };

    static constexpr uint32_t RngSeedClick { 20271 };
    static constexpr uint32_t RngSeedRing { 20272 };
    std::mt19937 m_rngClick { RngSeedClick };
    std::mt19937 m_rngRing { RngSeedRing };
    std::uniform_real_distribution<float> m_dist { -1.0f, 1.0f };
    BaseRateSource m_noiseBankClick {};
    BaseRateSource m_noiseBankRing {};

    double m_lastSampleRate { 0.0 };
};

} // namespace noteahead

#endif // KICK_PM_ENGINE_HPP
