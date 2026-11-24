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
constexpr double BoundaryRiseStart = 0.20;
constexpr double MicroprosodyTimeConstantSeconds = 0.030;

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
        const BoundaryTone clauseTone = (clauseEnd < events.size()) ? prosody[clauseEnd].boundaryTone : BoundaryTone::None;

        size_t lastVowelIdx = clauseEnd;
        for (size_t i = clauseEnd; i > clauseStart; i--) {
            if (events[i - 1].spec->type == PhonemeType::Vowel) {
                lastVowelIdx = i - 1;
                break;
            }
        }

        const size_t span = clauseEnd - clauseStart;
        size_t accentIndex = 0;
        bool inStressedSyllable = false;
        double currentAccent = 0.0;

        for (size_t i = clauseStart; i < clauseEnd; i++) {
            prosody[i].boundaryTone = clauseTone;
            prosody[i].clausePosition = (span > 1)
              ? static_cast<double>(i - clauseStart) / static_cast<double>(span - 1)
              : 0.0;

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
                    const double baseAccent = (clauseTone == BoundaryTone::Exclamation) ? 3.6 : 2.6;
                    const double downstep = std::max(0.38, std::pow(0.80, static_cast<double>(accentIndex)));
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
                    prosody[i].loudnessGain = (clauseTone == BoundaryTone::Exclamation) ? 1.32 : 1.26;
                } else if (events[i].spec->name == "AX") {
                    prosody[i].loudnessGain = 0.86;
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
                prosody[i].loudnessGain *= 0.90;
            }
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

    double declinationSlope = -2.0;
    if (cue.boundaryTone == BoundaryTone::Question) {
        declinationSlope = -0.8;
    } else if (cue.boundaryTone == BoundaryTone::Exclamation) {
        declinationSlope = -1.5;
    }
    const double declination = declinationSlope * cue.clausePosition;

    double accent = 0.0;
    if (spec.type == PhonemeType::Vowel && cue.accentSemitones > 0.0) {
        const double arch = (p <= AccentPeakProgress)
          ? 0.25 + 0.75 * 0.5 * (1.0 - std::cos(std::numbers::pi * (p / AccentPeakProgress)))
          : 0.25 + 0.75 * 0.5 * (1.0 + std::cos(std::numbers::pi * ((p - AccentPeakProgress) / (1.0 - AccentPeakProgress))));
        accent = cue.accentSemitones * arch;
    }

    double boundary = 0.0;
    if (cue.inBoundaryRegion) {
        double s = 1.0;
        if (cue.carriesBoundaryTone) {
            const double bp = std::clamp((p - BoundaryRiseStart) / (1.0 - BoundaryRiseStart), 0.0, 1.0);
            s = bp * bp * (3.0 - 2.0 * bp);
        }
        switch (cue.boundaryTone) {
        case BoundaryTone::Question:
            boundary = 3.5 * s;
            break;
        case BoundaryTone::Continuation:
            boundary = 1.2 * s;
            break;
        case BoundaryTone::Statement:
            boundary = -2.0 * s;
            break;
        case BoundaryTone::Exclamation:
            boundary = 1.0 * (1.0 - s) - 1.5 * s;
            break;
        case BoundaryTone::None:
            break;
        }
    }

    const double micro = cue.microprosodySemitones * std::exp(-std::max(0.0, phonemeElapsedSeconds) / MicroprosodyTimeConstantSeconds);
    return (declination + accent + boundary) * intonation + micro * std::min(1.0, intonation * 1.5);
}

double SpeechProsody::loudnessGain(const ProsodyEvent & cue, double intonation)
{
    const double amount = std::clamp(intonation * 1.5, 0.0, 1.0);
    return 1.0 + (cue.loudnessGain - 1.0) * amount;
}

} // namespace noteahead
