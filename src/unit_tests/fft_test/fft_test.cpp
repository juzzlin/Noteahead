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

#include "fft_test.hpp"

#include "../../domain/dsp/fft.hpp"

#include <QTest>

#include <cmath>
#include <numbers>
#include <random>
#include <vector>

namespace noteahead {

namespace {

//! The bins 0..N/2 of @p input the long way round: a full complex transform of the real signal.
void referenceSpectrum(const std::vector<double> & input, std::vector<double> & re, std::vector<double> & im)
{
    const int N = static_cast<int>(input.size());
    re = input;
    im.assign(N, 0.0);
    Fft::forward(re.data(), im.data(), N);
}

} // namespace

void FftTest::test_forwardReal_noise_shouldMatchTheComplexTransform()
{
    // Noise rather than a tone: a signal with something in every bin is what tells a mispacked
    // half-spectrum from a correct one, where a single sine would agree by symmetry alone.
    std::mt19937 rng { 12345 };
    std::uniform_real_distribution<double> dist { -1.0, 1.0 };

    for (const int N : { 4, 16, 256, 4096 }) {
        std::vector<double> input(N);
        for (auto & sample : input) {
            sample = dist(rng);
        }

        std::vector<double> expectedRe, expectedIm;
        referenceSpectrum(input, expectedRe, expectedIm);

        std::vector<double> re(N, 0.0), im(N, 0.0);
        Fft::forwardReal(input.data(), re.data(), im.data(), N);

        for (int k = 0; k <= N / 2; k++) {
            QVERIFY2(std::abs(re[k] - expectedRe[k]) < 1.0e-9 && std::abs(im[k] - expectedIm[k]) < 1.0e-9,
                     qPrintable(QString("N=%1 bin %2: got (%3, %4), expected (%5, %6)")
                                  .arg(N)
                                  .arg(k)
                                  .arg(re[k])
                                  .arg(im[k])
                                  .arg(expectedRe[k])
                                  .arg(expectedIm[k])));
        }
    }
}

void FftTest::test_forwardReal_sine_shouldPeakAtTheSineBin()
{
    constexpr int N = 1024;
    constexpr int bin = 37;

    std::vector<double> input(N);
    for (int i = 0; i < N; i++) {
        input[i] = std::sin(2.0 * std::numbers::pi * bin * i / N);
    }

    std::vector<double> re(N, 0.0), im(N, 0.0);
    Fft::forwardReal(input.data(), re.data(), im.data(), N);

    // A whole number of cycles lands entirely in one bin, at half the amplitude in each of the two
    // halves of the spectrum.
    const double magnitude = std::hypot(re[bin], im[bin]);
    QVERIFY(std::abs(magnitude - N / 2.0) < 1.0e-6);

    for (int k = 0; k <= N / 2; k++) {
        if (k != bin) {
            QVERIFY2(std::hypot(re[k], im[k]) < 1.0e-6, qPrintable(QString("bin %1 leaked").arg(k)));
        }
    }
}

void FftTest::test_forwardReal_dcAndNyquist_shouldBeReal()
{
    constexpr int N = 64;

    std::vector<double> input(N);
    for (int i = 0; i < N; i++) {
        input[i] = 2.0 + ((i % 2 == 0) ? 0.5 : -0.5); // A DC offset plus a tone exactly at Nyquist.
    }

    std::vector<double> re(N, 0.0), im(N, 0.0);
    Fft::forwardReal(input.data(), re.data(), im.data(), N);

    QVERIFY(std::abs(re[0] - 2.0 * N) < 1.0e-9);
    QVERIFY(std::abs(im[0]) < 1.0e-12);
    QVERIFY(std::abs(re[N / 2] - 0.5 * N) < 1.0e-9);
    QVERIFY(std::abs(im[N / 2]) < 1.0e-12);
}

} // namespace noteahead

QTEST_GUILESS_MAIN(noteahead::FftTest)
