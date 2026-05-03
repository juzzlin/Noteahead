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

#ifndef MULTI_ENGINE_HPP
#define MULTI_ENGINE_HPP

#include <cstdint>
#include <random>

namespace noteahead {

class MultiEngine
{
public:
    enum class Type
    {
        High,
        Low,
        Peak,
        Decim
    };

    MultiEngine();

    void setSampleRate(double sampleRate);
    void setType(Type type);
    void setShape(float shape);
    void setKeyTrack(float keyTrack);
    void setNote(uint8_t note);

    float nextSample();
    void reset();

private:
    Type m_type { Type::Low };
    double m_sampleRate { 44100.0 };
    float m_shape { 0.5f };
    float m_keyTrack { 0.0f };
    uint8_t m_note { 60 };

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist;

    // Filter states
    double m_s1 { 0.0 };
    double m_s2 { 0.0 };

    // Decimator state
    float m_lastSample { 0.0f };
    double m_phase { 0.0 };

    float processFilter(float input, float cutoff, float resonance, int mode);
};

} // namespace noteahead

#endif // MULTI_ENGINE_HPP
