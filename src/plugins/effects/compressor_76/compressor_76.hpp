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

#ifndef COMPRESSOR_76_HPP
#define COMPRESSOR_76_HPP

#include "../../../domain/dsp/dc_blocker.hpp"
#include "../../../domain/dsp/fet_compressor_core.hpp"
#include "../../../domain/dsp/svf_filter.hpp"
#include "../../../domain/effects/effect.hpp"

#include <memory>
#include <string>
#include <vector>

namespace noteahead {

//! An 1176-style feedback FET limiting amplifier: Input drive into a fixed threshold, Output
//! make-up, ultra-fast Attack, two-stage program-dependent Release, Ratio buttons including
//! All-Buttons-In ("All"), a sidechain Low Cut, and Blackface / Blue Stripe revision character.
//!
//! Nonlinear FET and Class-A output stages run oversampled when requested under MixLaw::Internal,
//! with a latency-matched dry decimator so parallel Mix never combs.
class Compressor76 : public Effect
{
public:
    using Ratio = FetCompressorCore::Ratio;

    //! Persisted by index: append only.
    enum class Character
    {
        Blackface,
        BlueStripe
    };

    Compressor76();
    ~Compressor76() override;

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;
    std::vector<std::string> modulationPriority() const override;

    void processSample(double & left, double & right) override;
    void reset() override;
    void sync() override;
    bool isSettled() const override;

    const EffectPresetList & factoryPresets() const override;

    float reductionDb() const;

    // Parameter conversions, shared with the presets, summary, dialog, and tests
    static double inputGainDb(float position);
    static float inputGainPosition(double db);
    static double outputGainDb(float position);
    static float outputGainPosition(double db);
    static double attackMs(float position);
    static float attackPosition(double ms);
    static double releaseMs(float position);
    static float releasePosition(double ms);
    static double lowCutHz(float position);
    static float lowCutPosition(double hz);
    static std::string ratioLabel(Ratio ratio);

    static double fetShape(double value, double grNorm, bool allMode, bool blueStripe);
    static double outputShape(double value, bool blueStripe);

private:
    void syncParameters();

    FetCompressorCore m_core;
    SvfFilter m_presenceL;
    SvfFilter m_presenceR;
    DcBlocker m_dcBlockerL;
    DcBlocker m_dcBlockerR;

    Ratio m_ratio { Ratio::FourToOne };
    Character m_character { Character::Blackface };
    double m_inputGain { 1.0 };
    double m_outputGain { 1.0 };

    double m_tunedStageSampleRate { -1.0 };
    Character m_tunedCharacter { Character::Blackface };
    bool m_shouldSyncParameters { true };

    struct Oversampling;
    std::unique_ptr<Oversampling> m_oversampling;
};

} // namespace noteahead

#endif // COMPRESSOR_76_HPP
