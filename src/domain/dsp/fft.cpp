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

#include "fft.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <utility>
#include <vector>

namespace noteahead::Fft {

namespace {

constexpr int MaxPrecomputedLog2 { 14 }; // N = 16384
constexpr int MaxPrecomputedSize { 1 << MaxPrecomputedLog2 };

struct FftTables
{
    std::array<std::vector<std::pair<uint16_t, uint16_t>>, MaxPrecomputedLog2 + 1> bitRevSwaps;
    std::array<std::vector<double>, MaxPrecomputedLog2 + 1> twiddleRe;
    std::array<std::vector<double>, MaxPrecomputedLog2 + 1> twiddleIm;

    FftTables()
    {
        for (int s = 1; s <= MaxPrecomputedLog2; s++) {
            const int len = 1 << s;
            const int half = len >> 1;

            auto & swaps = bitRevSwaps[static_cast<size_t>(s)];
            for (int i = 1, j = 0; i < len; i++) {
                int bit = half;
                for (; j & bit; bit >>= 1) {
                    j ^= bit;
                }
                j ^= bit;
                if (i < j) {
                    swaps.emplace_back(static_cast<uint16_t>(i), static_cast<uint16_t>(j));
                }
            }

            auto & tRe = twiddleRe[static_cast<size_t>(s)];
            auto & tIm = twiddleIm[static_cast<size_t>(s)];
            tRe.resize(static_cast<size_t>(half));
            tIm.resize(static_cast<size_t>(half));
            const double angStep = -2.0 * std::numbers::pi / static_cast<double>(len);
            for (int j = 0; j < half; j++) {
                const double ang = angStep * static_cast<double>(j);
                tRe[static_cast<size_t>(j)] = std::cos(ang);
                tIm[static_cast<size_t>(j)] = std::sin(ang);
            }
        }
    }
};

const FftTables & fftTables()
{
    static const FftTables tables {};
    return tables;
}

} // namespace

void forward(double * re, double * im, int N)
{
    if (N < 2) {
        return;
    }

    const unsigned uN = static_cast<unsigned>(N);
    if (N <= MaxPrecomputedSize && std::has_single_bit(uN)) {
        const auto & tables = fftTables();
        const int log2N = std::countr_zero(uN);
        for (const auto [i, j] : tables.bitRevSwaps[static_cast<size_t>(log2N)]) {
            std::swap(re[i], re[j]);
            std::swap(im[i], im[j]);
        }

        // len = 2 stage (twiddles = 1 + 0i)
        for (int i = 0; i < N; i += 2) {
            const double uRe = re[i];
            const double uIm = im[i];
            const double vRe = re[i + 1];
            const double vIm = im[i + 1];
            re[i] = uRe + vRe;
            im[i] = uIm + vIm;
            re[i + 1] = uRe - vRe;
            im[i + 1] = uIm - vIm;
        }

        if (N >= 4) {
            // len = 4 stage (w0 = 1, w1 = -i)
            for (int i = 0; i < N; i += 4) {
                const double u0Re = re[i];
                const double u0Im = im[i];
                const double v0Re = re[i + 2];
                const double v0Im = im[i + 2];
                re[i] = u0Re + v0Re;
                im[i] = u0Im + v0Im;
                re[i + 2] = u0Re - v0Re;
                im[i + 2] = u0Im - v0Im;

                const double u1Re = re[i + 1];
                const double u1Im = im[i + 1];
                const double v1Re = im[i + 3];
                const double v1Im = -re[i + 3];
                re[i + 1] = u1Re + v1Re;
                im[i + 1] = u1Im + v1Im;
                re[i + 3] = u1Re - v1Re;
                im[i + 3] = u1Im - v1Im;
            }
        }

        for (int s = 3; s <= log2N; s++) {
            const int len = 1 << s;
            const int halfLen = len >> 1;
            const double * wRePtr = tables.twiddleRe[static_cast<size_t>(s)].data();
            const double * wImPtr = tables.twiddleIm[static_cast<size_t>(s)].data();
            for (int i = 0; i < N; i += len) {
                double * reLo = re + i;
                double * imLo = im + i;
                double * reHi = reLo + halfLen;
                double * imHi = imLo + halfLen;
                for (int j = 0; j < halfLen; j++) {
                    const double wRe = wRePtr[j];
                    const double wIm = wImPtr[j];
                    const double xHiRe = reHi[j];
                    const double xHiIm = imHi[j];
                    const double uRe = reLo[j];
                    const double uIm = imLo[j];
                    const double vRe = xHiRe * wRe - xHiIm * wIm;
                    const double vIm = xHiRe * wIm + xHiIm * wRe;
                    reLo[j] = uRe + vRe;
                    imLo[j] = uIm + vIm;
                    reHi[j] = uRe - vRe;
                    imHi[j] = uIm - vIm;
                }
            }
        }
        return;
    }

    for (int i = 1, j = 0; i < N; i++) {
        int bit = N >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(re[i], re[j]);
            std::swap(im[i], im[j]);
        }
    }

    for (int len = 2; len <= N; len <<= 1) {
        const double ang = -2.0 * std::numbers::pi / len;
        const double wStepRe = std::cos(ang);
        const double wStepIm = std::sin(ang);
        for (int i = 0; i < N; i += len) {
            double wRe = 1.0;
            double wIm = 0.0;
            for (int j = 0; j < len / 2; j++) {
                const double uRe = re[i + j];
                const double uIm = im[i + j];
                const double vRe = re[i + j + len / 2] * wRe - im[i + j + len / 2] * wIm;
                const double vIm = re[i + j + len / 2] * wIm + im[i + j + len / 2] * wRe;
                re[i + j] = uRe + vRe;
                im[i + j] = uIm + vIm;
                re[i + j + len / 2] = uRe - vRe;
                im[i + j + len / 2] = uIm - vIm;
                const double newWRe = wRe * wStepRe - wIm * wStepIm;
                wIm = wRe * wStepIm + wIm * wStepRe;
                wRe = newWRe;
            }
        }
    }
}

void forwardReal(const double * input, double * re, double * im, int N)
{
    if (N < 4) {
        return;
    }

    const int half = N / 2;

    // Even samples as the real part, odd as the imaginary: one N/2-point transform carries both.
    for (int i = 0; i < half; i++) {
        re[i] = input[2 * i];
        im[i] = input[2 * i + 1];
    }

    forward(re, im, half);

    // Unpacking. With Z the packed transform, the transforms of the even and odd samples are
    //   E[k] = (Z[k] + conj(Z[half-k])) / 2,  O[k] = (Z[k] - conj(Z[half-k])) / 2i
    // and the spectrum of the original is E[k] + exp(-2*pi*i*k/N) * O[k]. Bins k and half-k are
    // written together because each is read while the other is computed.
    const double z0Re = re[0];
    const double z0Im = im[0];
    re[0] = z0Re + z0Im; // DC and Nyquist are both real and fall out of Z[0] alone.
    im[0] = 0.0;
    re[half] = z0Re - z0Im;
    im[half] = 0.0;

    const unsigned uN = static_cast<unsigned>(N);
    if (N <= MaxPrecomputedSize && std::has_single_bit(uN)) {
        const auto & tables = fftTables();
        const int log2N = std::countr_zero(uN);
        const double * wRePtr = tables.twiddleRe[static_cast<size_t>(log2N)].data();
        const double * wImPtr = tables.twiddleIm[static_cast<size_t>(log2N)].data();
        for (int k = 1; k <= half / 2; k++) {
            const double wRe = wRePtr[k];
            const double wIm = wImPtr[k];
            const int mirror = half - k;

            const double zkRe = re[k];
            const double zkIm = im[k];
            const double zmRe = re[mirror];
            const double zmIm = im[mirror];

            const double evenRe = 0.5 * (zkRe + zmRe);
            const double evenIm = 0.5 * (zkIm - zmIm);
            const double oddRe = 0.5 * (zkIm + zmIm);
            const double oddIm = -0.5 * (zkRe - zmRe);

            const double rotRe = oddRe * wRe - oddIm * wIm;
            const double rotIm = oddRe * wIm + oddIm * wRe;

            re[k] = evenRe + rotRe;
            im[k] = evenIm + rotIm;

            if (mirror != k) {
                re[mirror] = evenRe - rotRe;
                im[mirror] = -(evenIm - rotIm);
            }
        }
        return;
    }

    // Twiddles by the same recurrence the butterflies use rather than a cosine per bin: this runs
    // on the audio thread, and a transcendental per bin would cost more than the transform it
    // serves.
    const double ang = -std::numbers::pi / half;
    const double wStepRe = std::cos(ang);
    const double wStepIm = std::sin(ang);
    double wRe = 1.0;
    double wIm = 0.0;

    for (int k = 1; k <= half / 2; k++) {
        const double nextWRe = wRe * wStepRe - wIm * wStepIm;
        wIm = wRe * wStepIm + wIm * wStepRe;
        wRe = nextWRe;

        const int mirror = half - k;

        const double zkRe = re[k];
        const double zkIm = im[k];
        const double zmRe = re[mirror];
        const double zmIm = im[mirror];

        const double evenRe = 0.5 * (zkRe + zmRe);
        const double evenIm = 0.5 * (zkIm - zmIm);
        const double oddRe = 0.5 * (zkIm + zmIm);
        const double oddIm = -0.5 * (zkRe - zmRe);

        const double rotRe = oddRe * wRe - oddIm * wIm;
        const double rotIm = oddRe * wIm + oddIm * wRe;

        re[k] = evenRe + rotRe;
        im[k] = evenIm + rotIm;

        // The mirrored bin shares the same even and odd parts, conjugated, against the twiddle a
        // half turn away.
        if (mirror != k) {
            re[mirror] = evenRe - rotRe;
            im[mirror] = -(evenIm - rotIm);
        }
    }
}

void inverseReal(double * re, double * im, double * output, int N)
{
    if (N == 2) {
        output[0] = 0.5 * (re[0] + re[1]);
        output[1] = 0.5 * (re[0] - re[1]);
        return;
    }
    if (N < 4) {
        return;
    }

    const int half = N / 2;
    const double dc = re[0];
    const double nyquist = re[half];
    re[0] = 0.5 * (dc + nyquist);
    im[0] = 0.5 * (dc - nyquist);

    auto packPair = [&](int k, double wRe, double wIm) {
        const int mirror = half - k;
        const double xkRe = re[k];
        const double xkIm = im[k];
        const double xmRe = re[mirror];
        const double xmIm = im[mirror];

        const double evenRe = 0.5 * (xkRe + xmRe);
        const double evenIm = 0.5 * (xkIm - xmIm);
        const double rotRe = 0.5 * (xkRe - xmRe);
        const double rotIm = 0.5 * (xkIm + xmIm);

        const double oddRe = rotRe * wRe + rotIm * wIm;
        const double oddIm = rotIm * wRe - rotRe * wIm;

        re[k] = evenRe - oddIm;
        im[k] = evenIm + oddRe;
        if (mirror != k) {
            re[mirror] = evenRe + oddIm;
            im[mirror] = oddRe - evenIm;
        }
    };

    const unsigned uN = static_cast<unsigned>(N);
    if (N <= MaxPrecomputedSize && std::has_single_bit(uN)) {
        const auto & tables = fftTables();
        const int log2N = std::countr_zero(uN);
        const double * wRePtr = tables.twiddleRe[static_cast<size_t>(log2N)].data();
        const double * wImPtr = tables.twiddleIm[static_cast<size_t>(log2N)].data();
        for (int k = 1; k <= half / 2; k++) {
            packPair(k, wRePtr[k], wImPtr[k]);
        }
    } else {
        const double ang = -std::numbers::pi / half;
        const double wStepRe = std::cos(ang);
        const double wStepIm = std::sin(ang);
        double wRe = 1.0;
        double wIm = 0.0;
        for (int k = 1; k <= half / 2; k++) {
            const double nextWRe = wRe * wStepRe - wIm * wStepIm;
            wIm = wRe * wStepIm + wIm * wStepRe;
            wRe = nextWRe;
            packPair(k, wRe, wIm);
        }
    }

    inverse(re, im, half);

    for (int i = 0; i < half; i++) {
        output[2 * i] = re[i];
        output[2 * i + 1] = im[i];
    }
}

void inverse(double * re, double * im, int N)
{
    // Conjugate, forward transform, conjugate back: the same butterflies run backwards.
    for (int i = 0; i < N; i++) {
        im[i] = -im[i];
    }

    forward(re, im, N);

    const double scale = 1.0 / static_cast<double>(N);
    for (int i = 0; i < N; i++) {
        re[i] *= scale;
        im[i] *= -scale;
    }
}

} // namespace noteahead::Fft
