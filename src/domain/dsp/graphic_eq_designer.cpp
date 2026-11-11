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

#include "graphic_eq_designer.hpp"

#include "matched_bell_filter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

//! The band that sits at 1 kHz.
constexpr int ReferenceBand { 18 };
//! Where the first pass puts every bell, which is the size of boost the panel is built around.
constexpr double ReferenceGainDb { 12.0 };
//! Weight of a point between two centres against a centre's.
constexpr double MidpointWeight { 0.2 };
constexpr int RefinementPasses { 2 };
//! A bell measured at a gain this small has no shape left to measure, so the refinement passes
//! measure it at this size instead, with its own sign.
constexpr double SmallestMeasuredGainDb { 0.5 };
//! How near Nyquist a centre may sit.
constexpr double HighestCentreRatio { 0.49 };

//! At most one point per band and one between each pair of neighbours.
constexpr size_t MaxPoints { GraphicEqDesigner::BandCount * 2 - 1 };

using Vector = std::array<double, GraphicEqDesigner::BandCount>;
using Matrix = std::array<Vector, GraphicEqDesigner::BandCount>;

//! Solves the symmetric positive definite system @p matrix x = @p vector of the first @p size rows in
//! place, by Gaussian elimination with partial pivoting. The overlap matrix is well enough
//! conditioned -- around 5 -- that the normal equations lose nothing. Fixed-size storage, because
//! the design runs on the audio thread whenever a slider is automated.
Vector solve(Matrix & matrix, Vector & vector, size_t size)
{
    for (size_t column = 0; column < size; column++) {
        size_t pivot = column;
        for (size_t row = column + 1; row < size; row++) {
            if (std::abs(matrix[row][column]) > std::abs(matrix[pivot][column])) {
                pivot = row;
            }
        }
        std::swap(matrix[column], matrix[pivot]);
        std::swap(vector[column], vector[pivot]);
        for (size_t row = column + 1; row < size; row++) {
            const double factor = matrix[row][column] / matrix[column][column];
            for (size_t i = column; i < size; i++) {
                matrix[row][i] -= factor * matrix[column][i];
            }
            vector[row] -= factor * vector[column];
        }
    }
    Vector result {};
    for (size_t row = size; row-- > 0;) {
        double sum = vector[row];
        for (size_t i = row + 1; i < size; i++) {
            sum -= matrix[row][i] * result[i];
        }
        result[row] = sum / matrix[row][row];
    }
    return result;
}

//! A bell's level in decibels at a point given by its phi terms; see MatchedBellFilter::SquaredMagnitude.
double levelDb(const MatchedBellFilter::SquaredMagnitude & magnitude, const std::array<double, 3> & phi)
{
    const auto dot = [&phi](const std::array<double, 3> & terms) { return terms[0] * phi[0] + terms[1] * phi[1] + terms[2] * phi[2]; };
    return 10.0 * std::log10(dot(magnitude.numerator) / dot(magnitude.denominator));
}

} // namespace

double GraphicEqDesigner::centreFrequency(size_t band)
{
    return 1000.0 * std::pow(2.0, (static_cast<double>(band) - ReferenceBand) / 3.0);
}

double GraphicEqDesigner::bandQ()
{
    // Q from a bandwidth in octaves; half an octave is one and a half thirds.
    const double ratio = std::pow(2.0, 0.5);
    return std::sqrt(ratio) / (ratio - 1.0);
}

bool GraphicEqDesigner::isBandUsable(size_t band, double sampleRate)
{
    return band < BandCount && centreFrequency(band) < sampleRate * HighestCentreRatio;
}

GraphicEqDesigner::Gains GraphicEqDesigner::filterGains(const Gains & sliderGainsDb, double sampleRate)
{
    Gains result {};
    if (std::ranges::all_of(sliderGainsDb, [](double gain) { return gain == 0.0; })) {
        return result;
    }

    std::array<size_t, BandCount> bands {};
    size_t size = 0;
    for (size_t band = 0; band < BandCount; band++) {
        if (isBandUsable(band, sampleRate)) {
            bands[size++] = band;
        }
    }
    if (size == 0) {
        return result;
    }

    // Every centre, and the point half way between each pair on a log scale, where the target is
    // the straight line between the two sliders. Each point keeps the phi terms its frequency gives,
    // so that a bell is measured there with a few multiplications instead of a complex division.
    struct Point
    {
        std::array<double, 3> phi;
        double targetDb;
        double weight;
    };

    const auto phiAt = [sampleRate](double frequency) {
        const double half = std::sin(std::numbers::pi * frequency / sampleRate);
        const double phi1 = half * half;
        const double phi0 = 1.0 - phi1;
        return std::array<double, 3> { phi0, phi1, 4.0 * phi0 * phi1 };
    };

    std::array<Point, MaxPoints> points {};
    size_t pointCount = 0;
    for (size_t i = 0; i < size; i++) {
        points[pointCount++] = { phiAt(centreFrequency(bands[i])), sliderGainsDb.at(bands[i]), 1.0 };
        if (i + 1 < size) {
            const double between = std::sqrt(centreFrequency(bands[i]) * centreFrequency(bands[i + 1]));
            points[pointCount++] = { phiAt(between), 0.5 * (sliderGainsDb.at(bands[i]) + sliderGainsDb.at(bands[i + 1])), MidpointWeight };
        }
    }

    Vector gains {};
    gains.fill(ReferenceGainDb);
    Vector measuredAt = gains;
    MatchedBellFilter bell;
    std::array<Vector, MaxPoints> overlap {};
    for (int pass = 0; pass <= RefinementPasses; pass++) {
        if (pass > 0) {
            for (size_t i = 0; i < size; i++) {
                measuredAt[i] = std::abs(gains[i]) < SmallestMeasuredGainDb ? std::copysign(SmallestMeasuredGainDb, gains[i]) : gains[i];
            }
        }

        // The response of each band per decibel of its gain, at every point, weighted.
        for (size_t column = 0; column < size; column++) {
            bell.calculate(centreFrequency(bands[column]), sampleRate, bandQ(), measuredAt[column]);
            const auto magnitude = bell.squaredMagnitude();
            for (size_t row = 0; row < pointCount; row++) {
                overlap[row][column] = points[row].weight * levelDb(magnitude, points[row].phi) / measuredAt[column];
            }
        }

        // Normal equations of the weighted least-squares fit.
        Matrix normal {};
        Vector projected {};
        for (size_t row = 0; row < pointCount; row++) {
            const double target = points[row].weight * points[row].targetDb;
            for (size_t i = 0; i < size; i++) {
                projected[i] += overlap[row][i] * target;
                for (size_t j = 0; j < size; j++) {
                    normal[i][j] += overlap[row][i] * overlap[row][j];
                }
            }
        }
        gains = solve(normal, projected, size);
    }

    for (size_t i = 0; i < size; i++) {
        result.at(bands[i]) = gains[i];
    }
    return result;
}

} // namespace noteahead
