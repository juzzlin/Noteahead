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

#ifndef WAVETABLE_HPP
#define WAVETABLE_HPP

#include <memory>
#include <string>
#include <vector>

namespace noteahead {

class Wavetable
{
public:
    using WavetableS = std::shared_ptr<Wavetable>;
    using WavetableCS = std::shared_ptr<const Wavetable>;
    using WavetableList = std::vector<WavetableS>;

    static constexpr int NumWaves = 64;
    static constexpr int WaveSize = 2048; // Larger size for better quality before interpolation
    static constexpr int NumMips = 10; // MIP levels for band-limiting

    struct MipLevel
    {
        std::vector<float> data;
        float maxFrequency;
    };

    explicit Wavetable(std::string name);

    const std::string & name() const;

    // Returns a sample using linear interpolation between waves and within waves
    // phase: 0.0 to 1.0
    // position: 0.0 to 1.0 (morph between the 64 waves)
    // frequency: used to select the correct MIP level
    float getSample(double phase, double position, double frequency, double sampleRate) const;

    static std::shared_ptr<Wavetable> createClassicSet();
    static std::shared_ptr<Wavetable> createSpectralSet();

private:
    void addMipLevel(const std::vector<std::vector<float>> & waveHarmonics, int maxHarmonics);

    std::string m_name;
    std::vector<MipLevel> m_mips;
};

} // namespace noteahead

#endif // WAVETABLE_HPP
