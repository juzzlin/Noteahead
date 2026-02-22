// This file is part of Noteahead.
// Copyright (C) 2025 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef UTILS_HPP
#define UTILS_HPP

#include <array>
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <QString>
#include <QStringList>

namespace noteahead {
class ProjectReader;
}

namespace noteahead::Utils {
namespace Misc {
void ensureFileExists(const std::filesystem::path & filePath);
QStringList stdStringVectorToQStringList(const std::vector<std::string> & stringVector);
std::optional<double> parseDecimal(std::string_view string);
} // namespace Misc

namespace Midi {
uint8_t scaleVelocityByKey(uint8_t velocity, uint8_t note, int keyTrackPercentage, int keyTrackOffset = 0);
double portNameMatchScore(const std::string & s1, const std::string & s2);
} // namespace Midi

namespace Xml {
std::optional<bool> readBoolAttribute(ProjectReader & reader, QString name, bool required = true);
std::optional<int> readIntAttribute(ProjectReader & reader, QString name, bool required = true);
std::optional<double> readDoubleAttribute(ProjectReader & reader, QString name, bool required = true);
std::optional<size_t> readUIntAttribute(ProjectReader & reader, QString name, bool required = true);
std::optional<QString> readStringAttribute(ProjectReader & reader, QString name, bool required = true);
std::optional<std::chrono::milliseconds> readMSecAttribute(ProjectReader & reader, QString name, bool required = true);
} // namespace Xml

namespace Dsp {
float cutoffToHz(float cutoff, float sampleRate);
float dbToLinear(float db);
float linearToDb(float linear);

//! Equal-power gain compensation for stacking @p voicesPerNote detuned voices on a single note, as
//! unison and dual voice modes do.
//!
//! Detuned voices are mutually uncorrelated for most of their beat cycle, so their power sums rather
//! than their amplitude and the compensation is 1/sqrt(N). Dividing by N instead — the amplitude sum
//! — would hold even when every voice happens to align, but it makes unison audibly quieter than a
//! single voice, which is not what a unison mode is for. The remaining sqrt(N) between the two is
//! the transient that arrives when the voices do drift into phase, and it is what the headroom below
//! full scale exists to absorb.
float voiceStackGain(int voicesPerNote);

//! Relative detune of the JP-8000's seven saws, the spacing that makes a supersaw a supersaw.
//!
//! The point is that they are *not* evenly spaced. Even spacing gives every adjacent pair the same
//! beat rate and every wider pair an exact multiple of it, so the beating lines up into one periodic
//! comb — the buzz that makes plain unison harsh in a dense mix. These offsets never line up. The
//! centre one matters most: the whole arrangement hangs off having a voice exactly at pitch, so a
//! device with fewer than seven voices drops outer saws and keeps the centre.
inline constexpr std::array<double, 7> supersawOffsets { -0.11002313, -0.06288439, -0.01952356, 0.0, 0.01991221, 0.06216538, 0.10745242 };

//! How far the outermost voice of a detuned stack sits from pitch, in semitones, at full depth.
//! Shared by every mode that spreads voices in pitch — the outer saws of a supersaw, the end voices
//! of a unison stack and both halves of a dual pair all land here, so the depth knob means the same
//! width whichever mode it is turned in.
inline constexpr double voiceSpreadMaxSemitones = 0.5;

//! Widest wander of a Drift voice, in cents, at full depth. Deliberately narrower than the supersaw
//! spread: the movement is what thickens the sound there, not the interval.
inline constexpr double driftModeMaxCents = 25.0;

//! Level of the centre voice, from Szabo's analysis of the JP-8000. It starts at unity and gives way
//! as the detune opens up.
double supersawCentreGain(double depth);

//! Level of every other voice, likewise. Near silent at zero detune, so a closed supersaw collapses
//! to a single clean saw rather than a stack in unison.
double supersawSideGain(double depth);

//! Where each voice of an eight-voice stack starts at 0% Phase Randomness, as a fraction of a cycle,
//! for the devices that share the eight-voice unison, supersaw and dual layouts.
//!
//! Every phase is a multiple of 1/57, and the eight multiples form a perfect difference set modulo
//! 57: each of the 56 differences between two of them occurs exactly once, so eight copies of a wave
//! at these phases sum, at every harmonic but every 57th, to 7/8 of the power eight randomly phased
//! copies give on average. A stack started from them is never in phase with itself and never deep
//! in cancellation.
//!
//! The multiplier of 20 and the order were chosen by measuring the first 50 ms of a saw, square,
//! triangle and sine stack over the playable range at quarter, half and full depth, in all three
//! layouts at once: the whole set as unison, the first seven as a supersaw and the first two as a
//! dual pair. Seven phases cannot be spread as evenly as eight, so the supersaw is the hardest of
//! the three; each stays within about two decibels of the power random phases give on average,
//! the unison within one.
inline constexpr std::array<double, 8> eightVoiceStackStartPhases {
    20.0 / 57.0, 3.0 / 57.0, 14.0 / 57.0, 5.0 / 57.0, 13.0 / 57.0, 32.0 / 57.0, 36.0 / 57.0, 0.0 / 57.0
};

//! A start phase @p randomness of the way from @p fixed towards @p random, the short way round the
//! cycle.
double partlyRandomPhase(double fixed, double random, float randomness);
} // namespace Dsp
} // namespace noteahead::Utils

#endif // UTILS_HPP
