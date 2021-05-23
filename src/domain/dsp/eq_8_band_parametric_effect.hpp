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

#ifndef EQ_8_BAND_PARAMETRIC_EFFECT_HPP
#define EQ_8_BAND_PARAMETRIC_EFFECT_HPP

#include "../devices/effect.hpp"
#include "svf_filter.hpp"

#include <array>
#include <vector>

namespace noteahead {

class Eq8BandParametricEffect : public Effect
{
public:
    Eq8BandParametricEffect();

    static std::string typeIdString()
    {
        return "b2e1f3a4-c5d6-4e7f-8a9b-0c1d2e3f4a5b";
    }

    std::string type() const override
    {
        return "eq8bandparametric";
    }

    std::string typeId() const override
    {
        return typeIdString();
    }

    void process(double & left, double & right) override;
    void process(AudioContext & context) override;
    void reset() override;
    void sync() override;

private:
    struct Band
    {
        SvfFilter filterL;
        SvfFilter filterR;
        SvfFilter::Type type { SvfFilter::Type::Bypass };
        float frequency { 1000.0f };
        float gainDb { 0.0f };
        float q { 0.707f };

        void reset()
        {
            filterL.reset();
            filterR.reset();
        }

        void updateCoefficients(double sampleRate)
        {
            switch (type) {
            case SvfFilter::Type::Bypass:
                filterL.setBypass();
                filterR.setBypass();
                break;
            case SvfFilter::Type::Bell:
                filterL.calculateBell(frequency, sampleRate, q, gainDb);
                filterR.calculateBell(frequency, sampleRate, q, gainDb);
                break;
            case SvfFilter::Type::LowShelf:
                filterL.calculateLowShelf(frequency, sampleRate, q, gainDb);
                filterR.calculateLowShelf(frequency, sampleRate, q, gainDb);
                break;
            case SvfFilter::Type::HighShelf:
                filterL.calculateHighShelf(frequency, sampleRate, q, gainDb);
                filterR.calculateHighShelf(frequency, sampleRate, q, gainDb);
                break;
            case SvfFilter::Type::LowCut:
                filterL.calculateLowCut(frequency, sampleRate, q);
                filterR.calculateLowCut(frequency, sampleRate, q);
                break;
            case SvfFilter::Type::HighCut:
                filterL.calculateHighCut(frequency, sampleRate, q);
                filterR.calculateHighCut(frequency, sampleRate, q);
                break;
            case SvfFilter::Type::Notch:
                filterL.calculateNotch(frequency, sampleRate, q);
                filterR.calculateNotch(frequency, sampleRate, q);
                break;
            }
        }
    };

    void syncParameters();
    void updateBuffers();
    void processStereo(double & left, double & right);

    static constexpr size_t NumBands = 8;
    std::array<Band, NumBands> m_bands;

    bool m_shouldSyncParameters { false };
    bool m_shouldUpdateBuffers { false };
    uint32_t m_lastSampleRate { 0 };
};

} // namespace noteahead

#endif // EQ_8_BAND_PARAMETRIC_EFFECT_HPP
