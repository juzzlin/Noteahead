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

#include "speech_prosody.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace noteahead {

namespace {

constexpr std::string_view PausePunctuation = ".,;:!?-";

constexpr double NominalSilenceMs = 90.0;
constexpr double CommaPauseMs = 140.0;
constexpr double DashPauseMs = 115.0;
constexpr double TerminalPauseMs = 280.0;

constexpr double AccentPeakProgress = 0.40;
constexpr double MicroprosodyTimeConstantSeconds = 0.030;
constexpr double AccentShoulderShare = 0.38;
constexpr double AccentShoulderSeconds = 0.140;

std::vector<char> collectPausePunctuation(std::string_view text)
{
    std::vector<char> puncts;
    size_t pos = 0;
    while (pos < text.size()) {
        if (text[pos] == '/') {
            const size_t end = text.find('/', pos + 1);
            pos = (end == std::string_view::npos) ? text.size() : (end + 1);
            continue;
        }
        if (PausePunctuation.find(text[pos]) != std::string_view::npos) {
            puncts.push_back(text[pos]);
        }
        pos++;
    }
    return puncts;
}

BoundaryTone toneForPunctuation(char mark)
{
    switch (mark) {
    case ',':
    case ';':
    case ':':
    case '-':
        return BoundaryTone::Continuation;
    case '.':
        return BoundaryTone::Statement;
    case '?':
        return BoundaryTone::Question;
    case '!':
        return BoundaryTone::Exclamation;
    default:
        return BoundaryTone::None;
    }
}

double pauseScaleFor(char mark, bool prevWasSilence)
{
    switch (mark) {
    case ',':
    case ';':
    case ':':
        return CommaPauseMs / NominalSilenceMs;
    case '-':
        return DashPauseMs / NominalSilenceMs;
    case '.':
    case '?':
    case '!':
        return prevWasSilence ? 1.0 : (TerminalPauseMs / NominalSilenceMs);
    default:
        return 1.0;
    }
}

} // namespace

std::vector<ProsodyEvent> SpeechProsody::analyze(std::string_view text, PhonemeEventList & events)
{
    std::vector<ProsodyEvent> prosody(events.size());
    const auto puncts = collectPausePunctuation(text);

    size_t punctIndex = 0;
    bool prevWasSilence = false;
    for (size_t i = 0; i < events.size(); i++) {
        if (events[i].spec->type == PhonemeType::Silence) {
            const char mark = (punctIndex < puncts.size()) ? puncts[punctIndex++] : '.';
            prosody[i].boundaryTone = toneForPunctuation(mark);
            events[i].lengthScale = pauseScaleFor(mark, prevWasSilence);
            prevWasSilence = true;
        } else {
            prevWasSilence = false;
        }
    }

    size_t cursor = 0;
    while (cursor < events.size()) {
        while (cursor < events.size() && events[cursor].spec->type == PhonemeType::Silence) {
            cursor++;
        }
        if (cursor >= events.size()) {
            break;
        }

        const size_t clauseStart = cursor;
        while (cursor < events.size() && events[cursor].spec->type != PhonemeType::Silence) {
            cursor++;
        }
        const size_t clauseEnd = cursor;
        const BoundaryTone clauseTone = (clauseEnd < events.size()) ? prosody[clauseEnd].boundaryTone : BoundaryTone::Statement;

        size_t lastVowelIdx = clauseEnd;
        size_t lastStressedVowelIdx = clauseEnd;
        size_t firstStressedVowelIdx = clauseEnd;
        for (size_t i = clauseEnd; i > clauseStart; i--) {
            if (events[i - 1].spec->type == PhonemeType::Vowel) {
                if (lastVowelIdx == clauseEnd) {
                    lastVowelIdx = i - 1;
                }
                if (events[i - 1].stressed) {
                    if (lastStressedVowelIdx == clauseEnd) {
                        lastStressedVowelIdx = i - 1;
                    }
                    firstStressedVowelIdx = i - 1;
                }
            }
        }

        const size_t span = clauseEnd - clauseStart;
        std::vector<double> t0(span, 0.0);
        std::vector<double> t1(span, 1.0);
        double totalNominalMs = 0.0;
        for (size_t k = 0; k < span; k++) {
            totalNominalMs += std::max(1.0, static_cast<double>(events[clauseStart + k].spec->nominalMs) * events[clauseStart + k].lengthScale);
        }
        double accumMs = 0.0;
        for (size_t k = 0; k < span; k++) {
            const double durMs = std::max(1.0, static_cast<double>(events[clauseStart + k].spec->nominalMs) * events[clauseStart + k].lengthScale);
            t0[k] = accumMs / totalNominalMs;
            accumMs += durMs;
            t1[k] = accumMs / totalNominalMs;
        }

        struct AccentPeak
        {
            double tPeak { 0.0 };
            double height { 0.0 };
        };

        std::vector<AccentPeak> peaks;

        size_t accentIndex = 0;
        bool inStressedSyllable = false;
        double currentAccent = 0.0;

        for (size_t i = clauseStart; i < clauseEnd; i++) {
            const size_t k = i - clauseStart;
            prosody[i].boundaryTone = clauseTone;
            prosody[i].clausePosition = 0.5 * (t0[k] + t1[k]);

            if (lastVowelIdx < clauseEnd) {
                if (i == lastVowelIdx) {
                    prosody[i].carriesBoundaryTone = true;
                }
                if (i >= lastVowelIdx) {
                    prosody[i].inBoundaryRegion = true;
                }
            }

            const bool newSyllable = (i == clauseStart) || events[i].syllableStart || events[i].wordStart;
            if (newSyllable) {
                if (events[i].stressed) {
                    const double baseAccent = (clauseTone == BoundaryTone::Exclamation) ? 5.2 : 4.0;
                    const double downstep = std::max(0.38, std::pow(0.78, static_cast<double>(accentIndex)));
                    currentAccent = baseAccent * downstep;
                    accentIndex++;
                    inStressedSyllable = true;
                } else {
                    inStressedSyllable = false;
                    currentAccent = 0.0;
                }
            } else if (!events[i].stressed) {
                inStressedSyllable = false;
                currentAccent = 0.0;
            }

            if (events[i].spec->type == PhonemeType::Vowel) {
                if (inStressedSyllable) {
                    prosody[i].accentSemitones = currentAccent;
                    prosody[i].loudnessGain = (clauseTone == BoundaryTone::Exclamation) ? 1.38 : 1.30;
                    peaks.push_back({ t0[k] + AccentPeakProgress * (t1[k] - t0[k]), currentAccent });
                } else if (events[i].spec->name == "AX") {
                    prosody[i].loudnessGain = 0.80;
                } else {
                    prosody[i].loudnessGain = 0.92;
                }

                if (i > clauseStart) {
                    const auto * prev = events[i - 1].spec;
                    if (prev->type == PhonemeType::Plosive || prev->type == PhonemeType::Fricative) {
                        prosody[i].microprosodySemitones = (prev->voicing <= 0.0) ? 1.2 : -0.7;
                    }
                }
            }

            if (lastVowelIdx < clauseEnd && i >= lastVowelIdx
                && clauseTone != BoundaryTone::Question && clauseTone != BoundaryTone::Exclamation) {
                prosody[i].loudnessGain *= 0.86;
            }
        }

        const double shoulderHalf = std::clamp((AccentShoulderSeconds * 1000.0) / std::max(1.0, totalNominalMs), 0.06, 0.45);
        double boundaryStart = 0.72;
        if (lastStressedVowelIdx < clauseEnd) {
            const size_t k = lastStressedVowelIdx - clauseStart;
            boundaryStart = t0[k] + AccentPeakProgress * (t1[k] - t0[k]);
        } else if (lastVowelIdx < clauseEnd) {
            const size_t k = lastVowelIdx - clauseStart;
            boundaryStart = 0.5 * (t0[k] + t1[k]);
        }
        boundaryStart = std::min(boundaryStart, 0.92);

        double initialRiseEnd = 0.0;
        if (firstStressedVowelIdx < clauseEnd && firstStressedVowelIdx > clauseStart) {
            const size_t k = firstStressedVowelIdx - clauseStart;
            if (!events[clauseStart].stressed) {
                initialRiseEnd = t0[k];
            }
        }

        const auto evalClauseBase = [&](double t) {
            double declinationSlope = -2.8;
            if (clauseTone == BoundaryTone::Question) {
                declinationSlope = -1.0;
            } else if (clauseTone == BoundaryTone::Exclamation) {
                declinationSlope = -2.0;
            }
            const double declination = declinationSlope * t;

            double initRise = 0.0;
            if (initialRiseEnd > 1.0e-4 && t < initialRiseEnd) {
                const double u = std::clamp(t / initialRiseEnd, 0.0, 1.0);
                const double s = u * u * (3.0 - 2.0 * u);
                initRise = -1.4 * (1.0 - s);
            }

            double shoulder = 0.0;
            for (const auto & peak : peaks) {
                const double d = std::abs(t - peak.tPeak);
                if (d < shoulderHalf) {
                    const double win = 0.5 * (1.0 + std::cos(std::numbers::pi * d / shoulderHalf));
                    shoulder += AccentShoulderShare * peak.height * win;
                }
            }

            double boundary = 0.0;
            if (t >= boundaryStart) {
                const double u = std::clamp((t - boundaryStart) / std::max(1.0e-4, 1.0 - boundaryStart), 0.0, 1.0);
                const double s = u * u * (3.0 - 2.0 * u);
                switch (clauseTone) {
                case BoundaryTone::Question:
                    boundary = 6.0 * s;
                    break;
                case BoundaryTone::Continuation:
                    boundary = -0.6 * std::sin(std::numbers::pi * u) + 1.6 * s;
                    break;
                case BoundaryTone::Statement:
                    boundary = -4.2 * s;
                    break;
                case BoundaryTone::Exclamation:
                    boundary = 1.4 * (1.0 - s) - 2.8 * s;
                    break;
                case BoundaryTone::None:
                    break;
                }
            }

            return declination + initRise + shoulder + boundary;
        };

        for (size_t k = 0; k < span; k++) {
            prosody[clauseStart + k].contourStartSemitones = evalClauseBase(t0[k]);
            prosody[clauseStart + k].contourEndSemitones = evalClauseBase(t1[k]);
        }
    }

    return prosody;
}

double SpeechProsody::pitchOffsetSemitones(
  const ProsodyEvent & cue,
  const PhonemeSpec & spec,
  double phonemeProgress,
  double phonemeElapsedSeconds,
  double intonation)
{
    if (intonation <= 0.0) {
        return 0.0;
    }

    const double p = std::clamp(phonemeProgress, 0.0, 1.0);
    const double base = cue.contourStartSemitones + (cue.contourEndSemitones - cue.contourStartSemitones) * p;

    double vowelArch = 0.0;
    if (spec.type == PhonemeType::Vowel && cue.accentSemitones > 0.0) {
        const double arch = (p <= AccentPeakProgress)
          ? 0.5 * (1.0 - std::cos(std::numbers::pi * (p / AccentPeakProgress)))
          : 0.5 * (1.0 + std::cos(std::numbers::pi * ((p - AccentPeakProgress) / (1.0 - AccentPeakProgress))));
        vowelArch = (1.0 - AccentShoulderShare) * cue.accentSemitones * arch;
    }

    const double micro = cue.microprosodySemitones * std::exp(-std::max(0.0, phonemeElapsedSeconds) / MicroprosodyTimeConstantSeconds);
    return (base + vowelArch) * intonation + micro * std::min(1.0, intonation * 1.5);
}

double SpeechProsody::loudnessGain(const ProsodyEvent & cue, double intonation)
{
    const double amount = std::clamp(intonation * 1.5, 0.0, 1.0);
    return 1.0 + (cue.loudnessGain - 1.0) * amount;
}

} // namespace noteahead
