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

    //! One harmonic of one wave. Phase is in radians, with zero meaning a sine: a table that wants
    //! only amplitudes leaves it alone. Asymmetric shapes such as a pulse need it.
    struct Harmonic
    {
        float amplitude = 0.0f;
        float phase = 0.0f;
    };

    //! Harmonics of a single wave, indexed by harmonic number minus one.
    using Spectrum = std::vector<Harmonic>;
    //! One spectrum per morph position, NumWaves of them.
    using SpectrumList = std::vector<Spectrum>;

    //! How each synthesized wave is scaled.
    enum class Normalization
    {
        //! Every wave ends at full scale. Waves of differing crest factor then differ in loudness,
        //! so a morph across them changes level.
        Peak,
        //! Every wave ends at the loudness of a full-scale sine, backing off to Peak for waves too
        //! spiky to get there without clipping. Keeps a morph, and a glide across mip levels, even.
        Rms
    };

    explicit Wavetable(std::string name);

    const std::string & name() const;

    // Returns a sample using linear interpolation between waves and within waves
    // phase: 0.0 to 1.0
    // position: 0.0 to 1.0 (morph between the 64 waves)
    // frequency: used to select the correct MIP level
    float getSample(double phase, double position, double frequency, double sampleRate) const;

    //! Names of the selectable sets, in ordinal order.
    static std::vector<std::string> setNames();

    //! Builds the set at the given ordinal. Costs tens of milliseconds, so callers that can be on
    //! or behind the audio thread should go through the synth's shared cache instead.
    static WavetableS createSet(size_t index);

    static WavetableS createClassicSet();
    static WavetableS createSpectralSet();

private:
    //! A selectable set: its name, how to build its spectra, and how its waves are scaled. The synth
    //! stores the selection as an ordinal, so entries are only ever appended, never reordered.
    struct SetDefinition
    {
        const char * name;
        SpectrumList (*spectra)();
        Normalization normalization;
    };

    static const std::vector<SetDefinition> & definitions();

    static SpectrumList classicSpectra();
    static SpectrumList spectralSpectra();
    static SpectrumList pulseWidthSpectra();
    static SpectrumList vocalFormantSpectra();
    static SpectrumList resonantSweepSpectra();
    static SpectrumList organDrawbarSpectra();

    static WavetableS createFromSpectra(std::string name, const SpectrumList & spectra, Normalization normalization);

    void addMipLevel(const SpectrumList & waveSpectra, int maxHarmonics, Normalization normalization);

    std::string m_name;
    std::vector<MipLevel> m_mips;
};

} // namespace noteahead

#endif // WAVETABLE_HPP
