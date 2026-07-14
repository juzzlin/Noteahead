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
#include <cstdint>

namespace noteahead {

class Eq8BandParametric : public Effect
{
public:
    Eq8BandParametric();

    static std::string typeIdString();
    std::string type() const override;
    std::string typeId() const override;

    void process(double & left, double & right) override;
    void process(AudioContext & context) override;
    void reset() override;
    void sync() override;

    enum class StereoMode
    {
        MidSide, // Both mid and side channels are processed (equivalent to independent L/R processing).
        Mid, // Only the mid (mono/center) channel is processed; the side channel passes through.
        Side // Only the side (stereo/difference) channel is processed; the mid channel passes through.
    };

private:
    struct Band
    {
        // The EQ operates internally in Mid/Side, so these process the mid and side channels respectively.
        SvfFilter filterMid;
        SvfFilter filterSide;
        SvfFilter::Type type { SvfFilter::Type::Bypass };
        double frequency { 1000.0 };
        double gainDb { 0.0 };
        double q { 0.707 };

        void reset()
        {
            filterMid.reset();
            filterSide.reset();
        }

        void updateCoefficients(double sampleRate)
        {
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
                filterMid.calculateLowCut(frequency, sampleRate, q);
                filterSide.calculateLowCut(frequency, sampleRate, q);
                break;
            case SvfFilter::Type::HighCut:
                filterMid.calculateHighCut(frequency, sampleRate, q);
                filterSide.calculateHighCut(frequency, sampleRate, q);
                break;
            case SvfFilter::Type::Notch:
                filterMid.calculateNotch(frequency, sampleRate, q);
                filterSide.calculateNotch(frequency, sampleRate, q);
                break;
            }
        }
    };

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
