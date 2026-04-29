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

#ifndef EFFECT_HPP
#define EFFECT_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace noteahead {

class Effect
{
public:
    virtual ~Effect() = default;
    virtual void process(float & left, float & right, uint32_t sampleRate) = 0;
    virtual void reset() {}
};

class VolumeEffect : public Effect
{
public:
    void setVolume(float volume) { m_volume = volume; }
    void process(float & left, float & right, uint32_t /*sampleRate*/) override
    {
        left *= m_volume;
        right *= m_volume;
    }

private:
    float m_volume { 1.0f };
};

class PanningEffect : public Effect
{
public:
    void setPan(float pan) { m_pan = pan; }
    void process(float & left, float & right, uint32_t /*sampleRate*/) override
    {
        const float gainL = std::min(1.0f, 2.0f - m_pan * 2.0f);
        const float gainR = std::min(1.0f, m_pan * 2.0f);
        left *= gainL;
        right *= gainR;
    }

private:
    float m_pan { 0.5f };
};

class LowPassFilterEffect : public Effect
{
public:
    void setCutoff(float cutoff) { m_cutoff = cutoff; }
    void process(float & left, float & right, uint32_t sampleRate) override
    {
        if (m_cutoff >= 0.999f) {
            return;
        }

        const float freq = 20.0f * std::pow(std::min(20000.0f, sampleRate * 0.49f) / 20.0f, m_cutoff);
        const float f = 2.0f * std::sin(std::numbers::pi_v<float> * freq / static_cast<float>(sampleRate));
        const float q = 0.5f;

        m_hpL = left - m_lpL - q * m_bpL;
        m_bpL += f * m_hpL;
        m_lpL += f * m_bpL;
        left = m_lpL;

        m_hpR = right - m_lpR - q * m_bpR;
        m_bpR += f * m_hpR;
        m_lpR += f * m_bpR;
        right = m_lpR;
    }

    void reset() override
    {
        m_lpL = m_hpL = m_bpL = 0.0f;
        m_lpR = m_hpR = m_bpR = 0.0f;
    }

private:
    float m_cutoff { 1.0f };
    float m_lpL { 0.0f }, m_hpL { 0.0f }, m_bpL { 0.0f };
    float m_lpR { 0.0f }, m_hpR { 0.0f }, m_bpR { 0.0f };
};

class HighPassFilterEffect : public Effect
{
public:
    void setCutoff(float cutoff) { m_cutoff = cutoff; }
    void process(float & left, float & right, uint32_t sampleRate) override
    {
        if (m_cutoff <= 0.001f) {
            return;
        }

        const float freq = 20.0f * std::pow(std::min(20000.0f, sampleRate * 0.49f) / 20.0f, m_cutoff);
        const float f = 2.0f * std::sin(std::numbers::pi_v<float> * freq / static_cast<float>(sampleRate));
        const float q = 0.5f;

        m_hpL = left - m_lpL - q * m_bpL;
        m_bpL += f * m_hpL;
        m_lpL += f * m_bpL;
        left = m_hpL;

        m_hpR = right - m_lpR - q * m_bpR;
        m_bpR += f * m_hpR;
        m_lpR += f * m_bpR;
        right = m_hpR;
    }

    void reset() override
    {
        m_lpL = m_hpL = m_bpL = 0.0f;
        m_lpR = m_hpR = m_bpR = 0.0f;
    }

private:
    float m_cutoff { 0.0f };
    float m_lpL { 0.0f }, m_hpL { 0.0f }, m_bpL { 0.0f };
    float m_lpR { 0.0f }, m_hpR { 0.0f }, m_bpR { 0.0f };
};

} // namespace noteahead

#endif // EFFECT_HPP
