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

#include "mix_advice_formatter.hpp"

#include <QCoreApplication>

#include <cmath>

namespace noteahead::MixAdviceFormatter {

namespace {

QString db(float value)
{
    // Unsigned: which way a region sits is said by the sentence itself, in words, rather than left
    // to a minus sign the reader has to interpret.
    return QString { "%1" }.arg(std::abs(value), 0, 'f', 1);
}

QString hz(double frequency)
{
    return frequency >= 1000.0
      ? QString { "%1 kHz" }.arg(frequency / 1000.0, 0, 'f', frequency >= 10000.0 ? 0 : 1)
      : QString { "%1 Hz" }.arg(frequency, 0, 'f', 0);
}

QString tiltSentence(float dbPerOctave)
{
    // Half a decibel an octave is under what anyone hears across the range; below that the mix is
    // level with pink rather than leaning either way.
    if (dbPerOctave <= -0.5f) {
        return QCoreApplication::translate("MixAdvice", "Overall the mix leans down %1 dB per octave. That is its tone rather than a fault, and everything below is read against that lean.").arg(db(dbPerOctave));
    }
    if (dbPerOctave >= 0.5f) {
        return QCoreApplication::translate("MixAdvice", "Overall the mix leans up %1 dB per octave, brighter than pink noise. Everything below is read against that lean.").arg(db(dbPerOctave));
    }
    return QCoreApplication::translate("MixAdvice", "Overall the mix sits level with pink noise. Everything below is read against that line.");
}

QString truePeakSentence(const MixAdvisor::Finding & finding)
{
    if (finding.valueDb >= 0.0f) {
        return QCoreApplication::translate("MixAdvice", "True peak reaches %1 dBTP, at or over full scale. It clips as it is, and again through any lossy encoder.").arg(finding.valueDb, 0, 'f', 1);
    }
    return QCoreApplication::translate("MixAdvice", "True peak reaches %1 dBTP, under a decibel of headroom. A lossy encoder rebuilds peaks above the samples it was given, so this can clip on playback.").arg(finding.valueDb, 0, 'f', 1);
}

} // namespace

QString sentence(const MixAdvisor::Finding & finding)
{
    const bool above = finding.direction == MixAdvisor::Direction::Above;
    const QString amount = db(finding.valueDb);

    switch (finding.topic) {
    case MixAdvisor::Topic::Tilt:
        return tiltSentence(finding.valueDb);

    case MixAdvisor::Topic::Balanced:
        return QCoreApplication::translate("MixAdvice", "Nothing stands out: every region sits close to the mix's own tilt.");

    case MixAdvisor::Topic::Sub:
        return above
          ? QCoreApplication::translate("MixAdvice", "Below 50 Hz sits %1 dB above the tilt. Most speakers will not reproduce it, and a limiter spends its headroom on it anyway.").arg(amount)
          : QCoreApplication::translate("MixAdvice", "Below 50 Hz sits %1 dB under the tilt. The bottom octave is thin, and on a big system the mix will sound smaller than it does here.").arg(amount);

    case MixAdvisor::Topic::LowEnd:
        return above
          ? QCoreApplication::translate("MixAdvice", "50-125 Hz sits %1 dB above the tilt, where bass and kick share the room. A mix that piles up here reads as boomy.").arg(amount)
          : QCoreApplication::translate("MixAdvice", "50-125 Hz sits %1 dB under the tilt. Bass and kick are thin against the rest of the mix.").arg(amount);

    case MixAdvisor::Topic::LowMid:
        return above
          ? QCoreApplication::translate("MixAdvice", "125-400 Hz sits %1 dB above the tilt. This is where a mix reads as thick or muddy, and it is the first thing heard on small speakers.").arg(amount)
          : QCoreApplication::translate("MixAdvice", "125-400 Hz sits %1 dB under the tilt. Most instruments lose their body, and the mix reads as lean.").arg(amount);

    case MixAdvisor::Topic::Mid:
        return above
          ? QCoreApplication::translate("MixAdvice", "400 Hz - 1.25 kHz sits %1 dB above the tilt. This is the boxy region: it makes a mix sound like it is being played through something.").arg(amount)
          : QCoreApplication::translate("MixAdvice", "400 Hz - 1.25 kHz sits %1 dB under the tilt. A scooped middle is impressive on a first listen and empty on a fifth.").arg(amount);

    case MixAdvisor::Topic::Presence:
        return above
          ? QCoreApplication::translate("MixAdvice", "1.25-4 kHz sits %1 dB above the tilt. Leads cut through easily here, and the same region turns harsh at listening level.").arg(amount)
          : QCoreApplication::translate("MixAdvice", "1.25-4 kHz sits %1 dB under the tilt. Leads sit behind the rest of the mix, and detail reads as distant.").arg(amount);

    case MixAdvisor::Topic::Top:
        return above
          ? QCoreApplication::translate("MixAdvice", "4-10 kHz sits %1 dB above the tilt. Cymbals and sibilance are forward, which tires the ear over a long listen.").arg(amount)
          : QCoreApplication::translate("MixAdvice", "4-10 kHz sits %1 dB under the tilt. The mix reads as dull and closed-in.").arg(amount);

    case MixAdvisor::Topic::Air:
        return above
          ? QCoreApplication::translate("MixAdvice", "Above 10 kHz sits %1 dB above the tilt. Plenty of air, to the point of reading as brittle.").arg(amount)
          : QCoreApplication::translate("MixAdvice", "Above 10 kHz sits %1 dB under the tilt. The top octave is closed in, and the sense of space goes before anything else does.").arg(amount);

    case MixAdvisor::Topic::PresenceAgainstHighs:
        return QCoreApplication::translate("MixAdvice", "Presence sits %1 dB under the highs. That is the number a hollow mix gives itself away by: brightness with nothing under it reads as thin rather than as clear.").arg(amount);

    case MixAdvisor::Topic::Resonance:
        return QCoreApplication::translate("MixAdvice", "The %1 band stands %2 dB above both of its neighbours. Usually one sustained note or a resonance rather than the balance of the mix.").arg(hz(finding.frequencyHz), amount);

    case MixAdvisor::Topic::TruePeak:
        return truePeakSentence(finding);
    }

    return {};
}

} // namespace noteahead::MixAdviceFormatter
