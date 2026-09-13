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

#ifndef EQ_8_BAND_PARAMETRIC_HPP
#define EQ_8_BAND_PARAMETRIC_HPP

#include "../dsp/svf_filter.hpp"
#include "effect.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>

namespace noteahead {

class Eq8BandParametric : public Effect
{
public:
    Eq8BandParametric();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void processSample(double & left, double & right) override;
    void processBlock(AudioContext & context) override;
    void reset() override;
    void sync() override;
    const EffectPresetList & factoryPresets() const override;

    //! The Q a band of @p type should open at, as a parameter value, or nothing for a type that has
    //! no sensible one.
    //!
    //! Q means a different thing to each type, so one default cannot serve them all: a bell wants
    //! the octave a musician reaches for, a cut wants the Butterworth that leaves its corner flat,
    //! and a notch is only useful narrow. Applied when the user picks a type rather than when a
    //! project loads, or opening a song would overwrite every Q it had saved.
    static std::optional<float> defaultQParameterValue(SvfFilter::Type type);

    //! What the whole equalizer does at @p frequency, in dB.
    //!
    //! Asked of the bands themselves rather than worked out from the parameters a second time, so
    //! that a curve drawn from it cannot drift away from what is being heard -- including the extra
    //! sections a steep cut runs, which no formula over the parameters would know about.
    double magnitudeDbAt(double frequency) const;

    enum class StereoMode
    {
        MidSide, // Both mid and side channels are processed (equivalent to independent L/R processing).
        Mid, // Only the mid (mono/center) channel is processed; the side channel passes through.
        Side // Only the side (stereo/difference) channel is processed; the mid channel passes through.
    };

private:
    //! Pole Qs of a Butterworth cascade, by slope.
    //!
    //! A steep cut is several second-order sections in series, and which Q each one takes is what
    //! decides whether the result is a Butterworth or merely steep. Cascading identical sections
    //! loses 3 dB at the corner per section -- a four-section 48 dB/oct cut would sit 12 dB down
    //! where it should be 3 -- and a user reads that as the corner being in the wrong place. These
    //! are the standard staggered values, and the user's own Q scales them so that dialling
    //! resonance still does what it does at 12 dB/oct.
    static constexpr double ButterworthQ12[] { 0.7071 };
    static constexpr double ButterworthQ24[] { 0.5412, 1.3066 };
    static constexpr double ButterworthQ48[] { 0.5098, 0.6013, 0.8999, 2.5629 };
    static constexpr size_t MaxCutStages { 4 };

    struct Band
    {
        // The EQ operates internally in Mid/Side, so these process the mid and side channels respectively.
        SvfFilter filterMid;
        SvfFilter filterSide;
        //! The sections beyond the first, used only by a cut band asking for more than 12 dB/oct.
        std::array<SvfFilter, MaxCutStages - 1> extraMid;
        std::array<SvfFilter, MaxCutStages - 1> extraSide;
        //! How many sections are actually in use, the first included.
        size_t stages { 1 };
        SvfFilter::Type type { SvfFilter::Type::Bypass };
        double frequency { 1000.0 };
        double gainDb { 0.0 };
        double q { 0.707 };
        //! 0 for 12 dB/oct, 1 for 24, 2 for 48. Meaningful only on a cut.
        int slope { 0 };

        void reset()
        {
            filterMid.reset();
            filterSide.reset();
            for (auto & filter : extraMid) {
                filter.reset();
            }
            for (auto & filter : extraSide) {
                filter.reset();
            }
        }

        //! The Butterworth Qs this band's slope asks for, scaled by the user's own Q.
        std::span<const double> poleQs() const
        {
            if (type != SvfFilter::Type::LowCut && type != SvfFilter::Type::HighCut) {
                return { ButterworthQ12 };
            }
            if (slope >= 2) {
                return { ButterworthQ48 };
            }
            if (slope == 1) {
                return { ButterworthQ24 };
            }
            return { ButterworthQ12 };
        }

        //! Configures however many sections the slope asks for, each at its own Butterworth Q.
        void updateCutCoefficients(double sampleRate)
        {
            const auto qs = poleQs();
            stages = qs.size();
            // A cut's level at its own corner is the product of its sections' Qs, and the Butterworth
            // stagger is chosen so that product is always 0.7071 whatever the order. So the user's Q
            // is spread across the sections rather than applied to each: the nth root of it leaves
            // the product equal to the Q asked for, and the corner therefore sits where it sat at 12
            // dB/oct. Applied to every section instead, a Q of 1 came out 3 dB up at 24 dB/oct and 9
            // dB up at 48 -- a high pass that boomed at the very frequency it was put there to
            // clear.
            const double scale = std::pow(q / 0.7071, 1.0 / static_cast<double>(stages));
            for (size_t i = 0; i < qs.size(); i++) {
                const double stageQ = qs[i] * scale;
                auto & mid = i == 0 ? filterMid : extraMid.at(i - 1);
                auto & side = i == 0 ? filterSide : extraSide.at(i - 1);
                if (type == SvfFilter::Type::LowCut) {
                    mid.calculateLowCut(frequency, sampleRate, stageQ);
                    side.calculateLowCut(frequency, sampleRate, stageQ);
                } else {
                    mid.calculateHighCut(frequency, sampleRate, stageQ);
                    side.calculateHighCut(frequency, sampleRate, stageQ);
                }
            }
        }

        void updateCoefficients(double sampleRate)
        {
            stages = 1;
            switch (type) {
            case SvfFilter::Type::Bypass:
                filterMid.setBypass();
                filterSide.setBypass();
                break;
            case SvfFilter::Type::Bell:
                filterMid.calculateBell(frequency, sampleRate, q, gainDb);
                filterSide.calculateBell(frequency, sampleRate, q, gainDb);
                break;
            case SvfFilter::Type::LowShelf:
                filterMid.calculateLowShelf(frequency, sampleRate, q, gainDb);
                filterSide.calculateLowShelf(frequency, sampleRate, q, gainDb);
                break;
            case SvfFilter::Type::HighShelf:
                filterMid.calculateHighShelf(frequency, sampleRate, q, gainDb);
                filterSide.calculateHighShelf(frequency, sampleRate, q, gainDb);
                break;
            case SvfFilter::Type::LowCut:
            case SvfFilter::Type::HighCut:
                updateCutCoefficients(sampleRate);
                break;
            case SvfFilter::Type::Notch:
                filterMid.calculateNotch(frequency, sampleRate, q);
                filterSide.calculateNotch(frequency, sampleRate, q);
                break;
            case SvfFilter::Type::BandPass:
                // A raw tap carries no dry signal, so it is meaningless in this cascade. The band
                // type selector never offers it; treat it as a no-op rather than silently shaping.
                filterMid.setBypass();
                filterSide.setBypass();
                break;
            }
        }
    };

    //! The settings of band @p index, read off the parameters. Its filters are left unconfigured;
    //! updateCoefficients() is what brings them up.
    Band bandFromParameters(size_t index) const;
    void syncParameters();
    void updateBuffers();
    void processStereo(double & left, double & right);

    static constexpr size_t NumBands = 8;
    std::array<Band, NumBands> m_bands;

    StereoMode m_stereoMode { StereoMode::MidSide };

    bool m_shouldSyncParameters { false };
    bool m_shouldUpdateBuffers { false };
    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // EQ_8_BAND_PARAMETRIC_HPP
