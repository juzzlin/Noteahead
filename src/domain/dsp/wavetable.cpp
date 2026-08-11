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

#include "fft.hpp"

#include <algorithm>
#include <array>
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

namespace {

//! Loudness every RMS-normalized wave aims for. Set to sit alongside the two peak-normalized
//! sets rather than below them, which is as far as it can go before waves start needing more
//! headroom than they have.
constexpr float RmsTarget = 0.5f;

float wavePeak(const float * waveData)
{
    float maxVal = 0.0f;
    for (int i = 0; i < Wavetable::WaveSize; i++) {
        maxVal = std::max(maxVal, std::abs(waveData[i]));
    }
    return maxVal;
}

void scaleWave(float * waveData, float gain)
{
    for (int i = 0; i < Wavetable::WaveSize; i++) {
        waveData[i] *= gain;
    }
}

void normalizeWave(float * waveData, Wavetable::Normalization normalization)
{
    const float peak = wavePeak(waveData);
    if (peak < 1e-6f) {
        return;
    }

    if (normalization == Wavetable::Normalization::Peak) {
        scaleWave(waveData, 1.0f / peak);
        return;
    }

    double sumSquares = 0.0;
    for (int i = 0; i < Wavetable::WaveSize; i++) {
        sumSquares += static_cast<double>(waveData[i]) * waveData[i];
    }
    const auto rms = static_cast<float>(std::sqrt(sumSquares / Wavetable::WaveSize));
    if (rms < 1e-6f) {
        return;
    }

    // A wave spiky enough that matching the target would clip settles for full scale instead.
    scaleWave(waveData, std::min(RmsTarget / rms, 1.0f / peak));
}

} // namespace

void Wavetable::addMipLevel(const SpectrumList & waveSpectra, int maxHarmonics, Normalization normalization)
{
    std::vector<float> data(NumWaves * (WaveSize + 1), 0.0f);
    std::vector<double> re(WaveSize);
    std::vector<double> im(WaveSize);

    for (int w = 0; w < NumWaves; w++) {
        std::fill(re.begin(), re.end(), 0.0);
        std::fill(im.begin(), im.end(), 0.0);

        const auto & spectrum = waveSpectra[w];
        // Nyquist itself is dropped: bin WaveSize / 2 is its own mirror and a sine sits exactly on
        // the zero crossings there, so it can carry no energy anyway.
        const int n = std::min({ maxHarmonics, static_cast<int>(spectrum.size()), WaveSize / 2 - 1 });

        for (int h = 1; h <= n; h++) {
            const auto & harmonic = spectrum[h - 1];
            if (std::abs(harmonic.amplitude) < 1e-6f) {
                continue;
            }

            // A sin(2*pi*h*t + phase) is bin h and its mirror at WaveSize - h. The leading sine
            // convention puts amplitude on the imaginary axis, which is the -pi/2 rotation here.
            const double scale = static_cast<double>(WaveSize) * 0.5 * harmonic.amplitude;
            const double real = scale * std::sin(harmonic.phase);
            const double imag = -scale * std::cos(harmonic.phase);
            re[h] = real;
            im[h] = imag;
            re[WaveSize - h] = real;
            im[WaveSize - h] = -imag;
        }

        Fft::inverse(re.data(), im.data(), WaveSize);

        float * waveData = &data[w * (WaveSize + 1)];
        for (int i = 0; i < WaveSize; i++) {
            waveData[i] = static_cast<float>(re[i]);
        }

        normalizeWave(waveData, normalization);

        // Guard point for interpolation
        waveData[WaveSize] = waveData[0];
    }
    m_mips.push_back({ std::move(data), 0.0f });
}

Wavetable::WavetableS Wavetable::createFromSpectra(std::string name, const SpectrumList & spectra, Normalization normalization)
{
    auto table = std::make_shared<Wavetable>(std::move(name));
    for (int m = 0; m <= NumMips; m++) {
        table->addMipLevel(spectra, (WaveSize / 2) >> m, normalization);
    }
    return table;
}

Wavetable::SpectrumList Wavetable::classicSpectra()
{
    SpectrumList spectra(NumWaves);
    for (int w = 0; w < NumWaves; w++) {
        spectra[w].resize(WaveSize / 2);
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
            spectra[w][h - 1].amplitude = amp;
        }
    }

    return spectra;
}

Wavetable::SpectrumList Wavetable::spectralSpectra()
{
    SpectrumList spectra(NumWaves);
    for (int w = 0; w < NumWaves; w++) {
        spectra[w].resize(WaveSize / 2);
        const int harmonicsCount = 1 + w;
        for (int h = 1; h <= std::min(harmonicsCount, WaveSize / 2); h++) {
            spectra[w][h - 1].amplitude = 1.0f / static_cast<float>(h);
        }
    }

    return spectra;
}

Wavetable::SpectrumList Wavetable::pulseWidthSpectra()
{
    // A rectangular pulse of duty d has harmonic h at 2 * sin(pi * h * d) / (pi * h), and needs
    // cosine phase to come out as an actual pulse rather than a spectrally identical smear.
    constexpr double NarrowestDuty = 0.05;
    const auto cosinePhase = static_cast<float>(std::numbers::pi * 0.5);

    SpectrumList spectra(NumWaves);
    for (int w = 0; w < NumWaves; w++) {
        const double morph = static_cast<double>(w) / static_cast<double>(NumWaves - 1);
        const double duty = std::lerp(0.5, NarrowestDuty, morph);

        spectra[w].resize(WaveSize / 2);
        for (int h = 1; h <= WaveSize / 2; h++) {
            const double amplitude = 2.0 * std::sin(std::numbers::pi * h * duty) / (std::numbers::pi * h);
            spectra[w][h - 1] = { static_cast<float>(amplitude), cosinePhase };
        }
    }

    return spectra;
}

Wavetable::SpectrumList Wavetable::vocalFormantSpectra()
{
    struct Vowel
    {
        double f1;
        double f2;
        double f3;
    };

    // Formant centres in Hz, morphed through in this order.
    static const std::vector<Vowel> vowels = {
        { 730.0, 1090.0, 2440.0 }, // A
        { 530.0, 1840.0, 2480.0 }, // E
        { 270.0, 2290.0, 3010.0 }, // I
        { 570.0, 840.0, 2410.0 }, // O
        { 300.0, 870.0, 2240.0 } // U
    };

    // The table has no pitch of its own, so the formants are laid out against a reference
    // fundamental and then travel with the note, the way they do on any wavetable vocal patch.
    constexpr double ReferenceF0 = 130.0;
    constexpr int Harmonics = 256;
    constexpr std::array<double, 3> formantGains = { 1.0, 0.55, 0.3 };
    constexpr std::array<double, 3> formantWidths = { 130.0, 180.0, 250.0 };

    SpectrumList spectra(NumWaves);
    for (int w = 0; w < NumWaves; w++) {
        const double morph = static_cast<double>(w) / static_cast<double>(NumWaves - 1) * static_cast<double>(vowels.size() - 1);
        const auto index = std::min(static_cast<size_t>(morph), vowels.size() - 2);
        const double blend = morph - static_cast<double>(index);
        const auto & from = vowels[index];
        const auto & to = vowels[index + 1];
        const std::array<double, 3> formants = {
            std::lerp(from.f1, to.f1, blend),
            std::lerp(from.f2, to.f2, blend),
            std::lerp(from.f3, to.f3, blend)
        };

        spectra[w].resize(Harmonics);
        for (int h = 1; h <= Harmonics; h++) {
            const double frequency = h * ReferenceF0;
            double envelope = 0.0;
            for (size_t f = 0; f < formants.size(); f++) {
                const double offset = (frequency - formants[f]) / (formantWidths[f] * 0.5);
                envelope += formantGains[f] / (1.0 + offset * offset);
            }

            // Schroeder phases spread the energy out in time instead of piling it into one spike.
            // They depend only on the harmonic number, so every wave shares them and the morph
            // between two waves stays free of cancellation.
            const auto phase = static_cast<float>(std::numbers::pi * h * h / Harmonics);
            spectra[w][h - 1] = { static_cast<float>(envelope / h), phase };
        }
    }

    return spectra;
}

Wavetable::SpectrumList Wavetable::resonantSweepSpectra()
{
    // A saw with a resonant peak climbing it. The peak keeps a constant Q, so it reads as a filter
    // sweep that tracks pitch rather than a fixed formant.
    constexpr int Harmonics = 256;
    constexpr double TopHarmonic = 40.0;
    constexpr double PeakGain = 12.0;

    SpectrumList spectra(NumWaves);
    for (int w = 0; w < NumWaves; w++) {
        const double morph = static_cast<double>(w) / static_cast<double>(NumWaves - 1);
        const double centre = std::pow(TopHarmonic, morph);
        const double width = centre * 0.15 + 0.5;

        spectra[w].resize(Harmonics);
        for (int h = 1; h <= Harmonics; h++) {
            const double offset = (h - centre) / width;
            const double peak = 1.0 / (1.0 + offset * offset);
            spectra[w][h - 1].amplitude = static_cast<float>((1.0 + PeakGain * peak) / h);
        }
    }

    return spectra;
}

Wavetable::SpectrumList Wavetable::organDrawbarSpectra()
{
    // Drawbars that land on whole harmonics. The 16' and 5 1/3' bars are left out: they are the
    // sub-octave and its fifth, which a single-cycle table cannot hold without dropping the whole
    // set an octave.
    constexpr std::array<int, 7> drawbarHarmonics = { 1, 2, 3, 4, 5, 6, 8 };
    static const std::vector<std::array<int, 7>> registrations = {
        { 8, 0, 0, 0, 0, 0, 0 }, // Flute
        { 8, 6, 4, 2, 0, 0, 0 }, // Diapason
        { 8, 8, 6, 0, 4, 0, 2 }, // Jazz
        { 8, 6, 8, 6, 6, 4, 6 }, // Bright
        { 8, 8, 8, 8, 8, 8, 8 } // Full
    };

    SpectrumList spectra(NumWaves);
    for (int w = 0; w < NumWaves; w++) {
        const double morph = static_cast<double>(w) / static_cast<double>(NumWaves - 1) * static_cast<double>(registrations.size() - 1);
        const auto index = std::min(static_cast<size_t>(morph), registrations.size() - 2);
        const double blend = morph - static_cast<double>(index);

        spectra[w].resize(drawbarHarmonics.back());
        for (size_t d = 0; d < drawbarHarmonics.size(); d++) {
            const double level = std::lerp(static_cast<double>(registrations[index][d]), static_cast<double>(registrations[index + 1][d]), blend);
            spectra[w][drawbarHarmonics[d] - 1].amplitude = static_cast<float>(level / 8.0);
        }
    }

    return spectra;
}

const std::vector<Wavetable::SetDefinition> & Wavetable::definitions()
{
    // Append only. The synth serializes the selection as an index into this list.
    static const std::vector<SetDefinition> definitions = {
        { "Classic Morph", &Wavetable::classicSpectra, Normalization::Peak },
        { "Spectral Additive", &Wavetable::spectralSpectra, Normalization::Peak },
        { "Pulse Width", &Wavetable::pulseWidthSpectra, Normalization::Rms },
        { "Vocal Formant", &Wavetable::vocalFormantSpectra, Normalization::Rms },
        { "Resonant Sweep", &Wavetable::resonantSweepSpectra, Normalization::Rms },
        { "Organ Drawbar", &Wavetable::organDrawbarSpectra, Normalization::Rms }
    };
    return definitions;
}

std::vector<std::string> Wavetable::setNames()
{
    std::vector<std::string> names;
    for (auto && definition : definitions()) {
        names.push_back(definition.name);
    }
    return names;
}

Wavetable::WavetableS Wavetable::createSet(size_t index)
{
    const auto & definition = definitions().at(index);
    return createFromSpectra(definition.name, definition.spectra(), definition.normalization);
}

Wavetable::WavetableS Wavetable::createClassicSet()
{
    return createSet(0);
}

Wavetable::WavetableS Wavetable::createSpectralSet()
{
    return createSet(1);
}

} // namespace noteahead
