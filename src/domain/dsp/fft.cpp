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
