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

#include "mix_advisor.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace noteahead {

namespace {

//! The span the tilt is fitted over, in hertz.
//!
//! Stops short of both ends on purpose. Every mix rolls off below the bottom octave and above the
//! top one, and letting those ends pull the line would tilt it by their rolloff rather than by the
//! balance of everything between them, which is what the residuals are then read against.
constexpr double FitLowHz = 50.0;
constexpr double FitHighHz = 10000.0;

//! The regions a finding can be about, as [low, high) in hertz. Adjacent and non-overlapping, so
//! that one excess is reported once rather than under two names.
struct Region
{
    MixAdvisor::Topic topic;
    double lowHz;
    double highHz;
    //! Whether falling short of the tilt is worth saying, as well as standing above it.
    //!
    //! False for the two regions that sit outside the fitted range, where the line is extrapolated
    //! and every mix ever made falls under it: music rolls off at both ends, and a note that fires
    //! on all of them says nothing about any of them. Standing *above* an extrapolated line still
    //! means something, because nothing about a mix requires it. Thin at the bottom is caught by
    //! LowEnd, which is inside the fit and is where the ear looks for it anyway.
    bool reportBelow;
};

constexpr Region Regions[] {
    { MixAdvisor::Topic::Sub, 20.0, 50.0, false },
    { MixAdvisor::Topic::LowEnd, 50.0, 125.0, true },
    { MixAdvisor::Topic::LowMid, 125.0, 400.0, true },
    { MixAdvisor::Topic::Mid, 400.0, 1250.0, true },
    { MixAdvisor::Topic::Presence, 1250.0, 4000.0, true },
    { MixAdvisor::Topic::Top, 4000.0, 10000.0, true },
    { MixAdvisor::Topic::Air, 10000.0, 20000.0, false }
};

//! How far a region has to depart from the fitted tilt before it is worth a line, in dB.
//!
//! A first guess, and the part of this that wants measuring against finished mixes rather than
//! reasoning: two and a half decibels is about where a broad tonal shift stops being a matter of
//! taste, and four and a half is where it is the first thing anyone says about the mix. Both are
//! read against the mix's own fitted line, so a dark or bright master passes them untouched.
constexpr float RegionNoteDb = 2.5f;
constexpr float RegionCautionDb = 4.5f;

//! How far above both of its neighbours a single third-octave band has to stand to be called out.
//!
//! Wide enough not to fire on the ordinary lumpiness of a real spectrum, and deliberately one-sided:
//! a band sitting low between two higher ones is what a note simply not being played sounds like,
//! while a band standing above both is a sustained note, a room mode or a resonance.
constexpr float ResonanceDb = 4.0f;

//! Ceiling a master wants to leave under full scale, in dBTP.
//!
//! Not a matter of platform or genre, which is why it survives here while the loudness targets do
//! not: a lossy encoder reconstructs a waveform that overshoots the samples it was given, and a
//! master that already touches full scale clips on the way through one.
constexpr float TruePeakCeilingDbTp = -1.0f;

//! Loudness low enough that the file was silent or near enough, where true peak says nothing.
constexpr float TruePeakFloorDbTp = -60.0f;

//! Mean level of the bands whose centre falls in [lowHz, highHz).
//!
//! Returns nothing rather than zero for an empty range: a region above the Nyquist of the rendered
//! file has no level, and zero is a level that would read as perfectly balanced.
std::optional<float> meanLevel(const std::vector<SpectrumAnalyzer::Band> & bands, double lowHz, double highHz,
                               const std::vector<float> & levels)
{
    double sum = 0.0;
    int count = 0;
    for (size_t i = 0; i < bands.size(); i++) {
        if (bands[i].centerHz >= lowHz && bands[i].centerHz < highHz) {
            sum += levels[i];
            count++;
        }
    }
    return count ? std::optional<float> { static_cast<float>(sum / count) } : std::nullopt;
}

MixAdvisor::Severity severityOf(float deviationDb)
{
    return std::abs(deviationDb) >= RegionCautionDb ? MixAdvisor::Severity::Caution : MixAdvisor::Severity::Note;
}

//! A band as the fit sees it: octaves from 1 kHz against level.
struct Point
{
    double x { 0.0 };
    double y { 0.0 };
};

struct Line
{
    double slope { 0.0 }; //!< dB per octave.
    double intercept { 0.0 }; //!< dB at 1 kHz.

    double at(double x) const
    {
        return slope * x + intercept;
    }
};

double median(std::vector<double> & values)
{
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const size_t middle = values.size() / 2;
    return values.size() % 2 ? values[middle] : 0.5 * (values[middle - 1] + values[middle]);
}

//! The line the mix follows, fitted so that the departures from it cannot draw it.
//!
//! Theil-Sen: the slope is the median of the slopes of every pair of bands, and the offset the
//! median of what that slope leaves behind. Least squares is no use here, because the thing being
//! fitted is the thing the residuals are then read against: five decibels of mud over a few bands
//! tilts a least-squares line, the tilt takes most of the mud back out of the residual, and the
//! bands either side of it inherit a departure from a shape the mud drew. A median cannot be pulled
//! that way until nearly a third of the spectrum is pulling, and by then it is the mix.
//!
//! Costs a few hundred divisions on thirty bands, once per rendered file.
Line fitLine(const std::vector<SpectrumAnalyzer::Band> & bands)
{
    std::vector<Point> points;
    for (auto && band : bands) {
        if (band.centerHz >= FitLowHz && band.centerHz <= FitHighHz) {
            points.push_back(Point { std::log2(band.centerHz / 1000.0), band.levelDb });
        }
    }

    if (points.size() < 2) {
        return {};
    }

    std::vector<double> slopes;
    slopes.reserve(points.size() * (points.size() - 1) / 2);
    for (size_t i = 0; i < points.size(); i++) {
        for (size_t j = i + 1; j < points.size(); j++) {
            const double dx = points[j].x - points[i].x;
            if (std::abs(dx) > 1.0e-12) {
                slopes.push_back((points[j].y - points[i].y) / dx);
            }
        }
    }

    Line line;
    line.slope = median(slopes);

    std::vector<double> offsets;
    offsets.reserve(points.size());
    for (auto && point : points) {
        offsets.push_back(point.y - line.slope * point.x);
    }
    line.intercept = median(offsets);

    return line;
}

} // namespace

float MixAdvisor::fitTiltDbPerOctave(const std::vector<SpectrumAnalyzer::Band> & bands)
{
    return static_cast<float>(fitLine(bands).slope);
}

MixAdvisor::Summary MixAdvisor::advise(const SpectrumAnalyzer::Result & spectrum, const LoudnessAnalyzer::Result & loudness)
{
    Summary summary;

    std::vector<Finding> balanceFindings;

    if (spectrum.isValid && spectrum.bands.size() >= 2) {
        summary.hasBalance = true;

        const auto line = fitLine(spectrum.bands);
        summary.tiltDbPerOctave = static_cast<float>(line.slope);

        // Residuals: what is left of each band once the mix's own lean is taken out of it. Taken
        // over every band, including the ones outside the fitted range and the ones the fit trimmed
        // away, because those are exactly the bands worth reporting on.
        std::vector<float> residuals;
        residuals.reserve(spectrum.bands.size());
        for (auto && band : spectrum.bands) {
            residuals.push_back(static_cast<float>(band.levelDb - line.at(std::log2(band.centerHz / 1000.0))));
        }

        for (auto && region : Regions) {
            const auto level = meanLevel(spectrum.bands, region.lowHz, region.highHz, residuals);
            if (!level || std::abs(*level) < RegionNoteDb) {
                continue;
            }
            if (*level < 0.0f && !region.reportBelow) {
                continue;
            }
            balanceFindings.push_back(Finding {
              region.topic,
              severityOf(*level),
              *level > 0.0f ? Direction::Above : Direction::Below,
              *level,
              0.0 });
        }

        // Presence against highs, which the report already prints as a number. Only worth a sentence
        // when it is the hollow direction: the ear reads a lead buried under brightness far more
        // readily than the other way round.
        if (spectrum.upperMidToHighDb <= -RegionNoteDb) {
            balanceFindings.push_back(Finding {
              Topic::PresenceAgainstHighs,
              severityOf(spectrum.upperMidToHighDb),
              Direction::Below,
              spectrum.upperMidToHighDb,
              0.0 });
        }

        // Worst first. A reader who stops after one line has read the one that mattered.
        std::sort(balanceFindings.begin(), balanceFindings.end(), [](const Finding & a, const Finding & b) {
            return std::abs(a.valueDb) > std::abs(b.valueDb);
        });

        // One band standing above both of its neighbours. Left at the end whatever its size: it is
        // the most specific thing here and the least often a fault.
        for (size_t i = 1; i + 1 < residuals.size(); i++) {
            const float againstNeighbours = residuals[i] - 0.5f * (residuals[i - 1] + residuals[i + 1]);
            if (againstNeighbours >= ResonanceDb) {
                balanceFindings.push_back(Finding {
                  Topic::Resonance,
                  Severity::Note,
                  Direction::Neutral,
                  againstNeighbours,
                  spectrum.bands[i].centerHz });
            }
        }

        // The lean the rest is read against, first, because it frames everything under it.
        summary.findings.push_back(Finding { Topic::Tilt, Severity::Note, Direction::Neutral, summary.tiltDbPerOctave, 0.0 });

        if (balanceFindings.empty()) {
            // Said out loud rather than left as an empty section, which reads as something failing.
            summary.findings.push_back(Finding { Topic::Balanced, Severity::Note, Direction::Neutral, 0.0f, 0.0 });
        } else {
            summary.findings.insert(summary.findings.end(), balanceFindings.begin(), balanceFindings.end());
        }
    }

    // Loudness last, and only this one thing from it: what is left under full scale does not depend
    // on platform or genre the way a loudness target or a dynamic range does. A file too quiet to
    // have a true peak worth reporting says nothing here at all.
    if (loudness.truePeak > TruePeakCeilingDbTp && loudness.truePeak > TruePeakFloorDbTp) {
        summary.findings.push_back(Finding {
          Topic::TruePeak,
          loudness.truePeak >= 0.0f ? Severity::Caution : Severity::Note,
          Direction::Neutral,
          loudness.truePeak,
          0.0 });
    }

    return summary;
}

} // namespace noteahead
