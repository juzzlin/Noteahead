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

#ifndef ANALYSIS_REPORT_FORMATTER_HPP
#define ANALYSIS_REPORT_FORMATTER_HPP

#include <QString>

namespace noteahead {

struct AudioAnalysis;
}

//! How an AudioAnalysis is written out, in rich text for a dialog and in plain text for a file.
//!
//! Shared by the render, which reports on the file it has just written, and by the analysis tool,
//! which reports on a file already on disk: an export and a later look at the same file must not
//! describe it differently.
namespace noteahead::AnalysisReportFormatter {

//! The analysis as a report dialog shows it.
QString html(const AudioAnalysis & analysis);

//! The analysis as it is written to disk.
QString text(const AudioAnalysis & analysis);

//! Two analyses beside each other, with the difference of their bands: @p b minus @p a, so a
//! positive number means @p b has more there. Either one may be invalid, which leaves that side and
//! the difference out rather than printing zeroes.
QString comparisonText(const AudioAnalysis & a, const AudioAnalysis & b);

//! Path of the report written beside a rendered file: its whole name plus ".loudness.txt", so that
//! rendering the same song to both WAV and FLAC cannot have one report overwrite the other.
QString analysisFilePath(const QString & renderedPath);

//! Writes @p report to @p path. Failing to write it is logged and swallowed: beside a render, the
//! audio is what the user asked for and it is already on disk by this point.
bool writeReportFile(const QString & path, const QString & report);

} // namespace noteahead::AnalysisReportFormatter

#endif // ANALYSIS_REPORT_FORMATTER_HPP
