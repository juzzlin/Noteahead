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

#include "wavetable.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

Wavetable::Wavetable(std::string name)
  : m_name { std::move(name) }
{
}

const std::string & Wavetable::name() const
{
    return m_name;
}

float Wavetable::getSample(double phase, double position, double frequency, double sampleRate) const
{
    if (m_mips.empty()) {
        return 0.0f;
    }

    const double nyquist = sampleRate * 0.5;
    const double f0 = nyquist / static_cast<double>(WaveSize / 2);

    const double level = std::log2(std::max(frequency, f0) / f0);
    const int level1 = std::clamp(static_cast<int>(std::floor(level)), 0, static_cast<int>(m_mips.size() - 1));
    const int level2 = std::min(level1 + 1, static_cast<int>(m_mips.size() - 1));
    const float levelT = static_cast<float>(level - static_cast<double>(level1));

    const double scaledPos = position * static_cast<double>(NumWaves - 1);
    const int waveIndex1 = static_cast<int>(std::floor(scaledPos));
    const int waveIndex2 = std::min(waveIndex1 + 1, NumWaves - 1);
    const float waveT = static_cast<float>(scaledPos - static_cast<double>(waveIndex1));

    const double readPos = phase * static_cast<double>(WaveSize);
    const int i1 = static_cast<int>(std::floor(readPos));
    const float t = static_cast<float>(readPos - static_cast<double>(i1));

    const auto & mip1 = m_mips[level1];
    const size_t w1Offset = static_cast<size_t>(waveIndex1) * (WaveSize + 1);
    const size_t w2Offset = static_cast<size_t>(waveIndex2) * (WaveSize + 1);

    const float m1w1 = std::lerp(mip1.data[w1Offset + i1], mip1.data[w1Offset + i1 + 1], t);
    const float m1w2 = std::lerp(mip1.data[w2Offset + i1], mip1.data[w2Offset + i1 + 1], t);
    const float m1 = std::lerp(m1w1, m1w2, waveT);

    if (level1 == level2) {
        return m1;
    }

    const auto & mip2 = m_mips[level2];
    const float m2w1 = std::lerp(mip2.data[w1Offset + i1], mip2.data[w1Offset + i1 + 1], t);
    const float m2w2 = std::lerp(mip2.data[w2Offset + i1], mip2.data[w2Offset + i1 + 1], t);
    const float m2 = std::lerp(m2w1, m2w2, waveT);

    return std::lerp(m1, m2, levelT);
}

void Wavetable::addMipLevel(const std::vector<std::vector<float>> & waveHarmonics, int maxHarmonics)
{
    static const auto sinTable = []() {
        std::vector<float> table(WaveSize);
        for (int i = 0; i < WaveSize; i++) {
            table[i] = std::sin(static_cast<float>(2.0 * std::numbers::pi * i / WaveSize));
        }
        return table;
    }();

    std::vector<float> data(NumWaves * (WaveSize + 1), 0.0f);
    for (int w = 0; w < NumWaves; w++) {
        const auto & harmonics = waveHarmonics[w];
        const int n = std::min(maxHarmonics, static_cast<int>(harmonics.size()));
        float * waveData = &data[w * (WaveSize + 1)];

        for (int h = 0; h < n; h++) {
            const float amp = harmonics[h];
            if (std::abs(amp) < 1e-6f) {
                continue;
            }

            const int step = h + 1;
            for (int i = 0; i < WaveSize; i++) {
                waveData[i] += amp * sinTable[(i * step) & (WaveSize - 1)];
            }
        }

        // Normalize
        float maxVal = 0.0f;
        for (int i = 0; i < WaveSize; i++) {
            maxVal = std::max(maxVal, std::abs(waveData[i]));
        }
        if (maxVal > 1e-6f) {
            const float invMax = 1.0f / maxVal;
            for (int i = 0; i < WaveSize; i++) {
                waveData[i] *= invMax;
            }
        }

        // Guard point for interpolation
        waveData[WaveSize] = waveData[0];
    }
    m_mips.push_back({ std::move(data), 0.0f });
}

std::shared_ptr<Wavetable> Wavetable::createClassicSet()
{
    auto table = std::make_shared<Wavetable>("Classic Morph");

    std::vector<std::vector<float>> waveHarmonics(NumWaves);
    for (int w = 0; w < NumWaves; w++) {
        waveHarmonics[w].resize(WaveSize / 2);
        const float morph = static_cast<float>(w) / static_cast<float>(NumWaves - 1);

        for (int h = 1; h <= WaveSize / 2; h++) {
            float sineH = (h == 1) ? 1.0f : 0.0f;

            float triH = 0.0f;
            if (h % 2 != 0) {
                triH = 1.0f / static_cast<float>(h * h);
                if ((h / 2) % 2 != 0) {
                    triH = -triH;
                }
            }

            float sawH = 1.0f / static_cast<float>(h);

            float sqH = (h % 2 != 0) ? 1.0f / static_cast<float>(h) : 0.0f;

            float amp;
            if (morph < 0.33f) {
                amp = std::lerp(sineH, triH, morph / 0.33f);
            } else if (morph < 0.66f) {
                amp = std::lerp(triH, sawH, (morph - 0.33f) / 0.33f);
            } else {
                amp = std::lerp(sawH, sqH, (morph - 0.66f) / 0.34f);
            }
            waveHarmonics[w][h - 1] = amp;
        }
    }

    for (int m = 0; m <= NumMips; m++) {
        table->addMipLevel(waveHarmonics, (WaveSize / 2) >> m);
    }

    return table;
}

Wavetable::WavetableS Wavetable::createSpectralSet()
{
    auto table = std::make_shared<Wavetable>("Spectral Additive");

    std::vector<std::vector<float>> waveHarmonics(NumWaves);
    for (int w = 0; w < NumWaves; w++) {
        waveHarmonics[w].resize(WaveSize / 2, 0.0f);
        const int harmonicsCount = 1 + w;
        for (int h = 1; h <= std::min(harmonicsCount, WaveSize / 2); h++) {
            waveHarmonics[w][h - 1] = 1.0f / static_cast<float>(h);
        }
    }

    for (int m = 0; m <= NumMips; m++) {
        table->addMipLevel(waveHarmonics, (WaveSize / 2) >> m);
    }

    return table;
}

} // namespace noteahead
