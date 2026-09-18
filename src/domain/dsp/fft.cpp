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
#include <cmath>
#include <numbers>

namespace noteahead::Fft {

void forward(double * re, double * im, int N)
{
    for (int i = 1, j = 0; i < N; i++) {
        int bit = N >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
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
