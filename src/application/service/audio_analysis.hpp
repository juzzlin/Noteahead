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

#ifndef AUDIO_ANALYSIS_HPP
#define AUDIO_ANALYSIS_HPP

#include "../../domain/utility/loudness_analyzer.hpp"
#include "../../domain/utility/spectrum_analyzer.hpp"

#include <QMetaType>
#include <QString>

namespace noteahead {

//! Loudness and balance of one audio file, measured in a single pass over it.
//!
//! What a render reports about the file it has just written, and what the analysis tool reports
//! about a file already on disk: the same measurement of the same kind of thing, so both go through
//! this and through AnalysisReportFormatter rather than each growing its own.
struct AudioAnalysis
{
    LoudnessAnalyzer::Result loudness;
    SpectrumAnalyzer::Result spectrum;

    QString filePath;
    quint32 sampleRate { 0 };
    int channels { 0 };
    qint64 frames { 0 };

    //! False until something has actually been measured, which is what an empty side of the
    //! comparison is.
    bool isValid { false };
};

} // namespace noteahead

Q_DECLARE_METATYPE(noteahead::AudioAnalysis)

#endif // AUDIO_ANALYSIS_HPP
