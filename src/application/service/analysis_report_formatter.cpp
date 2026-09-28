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

#include "analysis_report_formatter.hpp"

#include "../../contrib/SimpleLogger/src/simple_logger.hpp"
#include "../../domain/utility/mix_advisor.hpp"
#include "audio_analysis.hpp"
#include "mix_advice_formatter.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <algorithm>
#include <cmath>

namespace noteahead::AnalysisReportFormatter {

static const auto TAG = "AnalysisReportFormatter";

namespace {

QString label(const QString & text, const QString & value)
{
    return QString { "%1%2\n" }.arg(text, -22).arg(value);
}

QString number(float value, const QString & unit)
{
    // Written the same way the report dialog writes it, so the file and the dialog agree
    return QString { "%1 %2" }.arg(value, 0, 'f', 1).arg(unit);
}

QString hz(double frequency)
{
    return frequency >= 1000.0
      ? QString { "%1 kHz" }.arg(frequency / 1000.0, 0, 'f', frequency >= 10000.0 ? 0 : 1)
      : QString { "%1 Hz" }.arg(frequency, 0, 'f', 0);
}

QString fileName(const AudioAnalysis & analysis)
{
    return QFileInfo { analysis.filePath }.fileName();
}

} // namespace

QString html(const AudioAnalysis & analysis)
{
    const auto & result = analysis;
    QString report = QString("<table width='100%' cellpadding='5' cellspacing='0'>"
                             "<tr>"
                             "<td bgcolor='#2c2c2c'><b>Parameter</b></td><td align='right' bgcolor='#2c2c2c'><b>Value</b></td>"
                             "</tr>"
                             "<tr>"
                             "<td>Integrated Loudness</td><td align='right'><font color='#4CAF50'><b>%1 LUFS</b></font></td>"
                             "</tr>"
                             "<tr>"
                             "<td>True Peak</td><td align='right'><font color='#FF9800'><b>%2 dBTP</b></font></td>"
                             "</tr>"
                             "<tr>"
                             "<td>Loudness Range (LRA)</td><td align='right'><font color='#2196F3'><b>%3 LU</b></font></td>"
                             "</tr>"
                             "<tr>"
                             "<td>Threshold</td><td align='right'><font color='#888888'>%4 LUFS</font></td>"
                             "</tr>"
                             "</table>")
                       .arg(result.loudness.integratedLoudness, 0, 'f', 1)
                       .arg(result.loudness.truePeak, 0, 'f', 1)
                       .arg(result.loudness.loudnessRange, 0, 'f', 1)
                       .arg(result.loudness.threshold, 0, 'f', 1);

    if (result.spectrum.isValid) {
        const auto row = [](const QString & name, float value, const QString & color) {
            return QString { "<tr><td>%1</td><td align='right'><font color='%2'><b>%3 dB</b></font></td></tr>" }
              .arg(name, color)
              .arg(value, 0, 'f', 1);
        };
        report += QString { "<br><table width='100%' cellpadding='5' cellspacing='0'>"
                            "<tr><td bgcolor='#2c2c2c'><b>Balance</b></td>"
                            "<td align='right' bgcolor='#2c2c2c'><b>vs. own midrange</b></td></tr>%1%2%3%4%5</table>" }
                    .arg(row("Low mids (100-250 Hz)", result.spectrum.lowMidDb, "#4CAF50"),
                         row("Mids (250-630 Hz)", result.spectrum.midDb, "#4CAF50"),
                         row("Presence (0.8-1.6 kHz)", result.spectrum.upperMidDb, "#2196F3"),
                         row("Highs (2.5-8 kHz)", result.spectrum.highDb, "#FF9800"),
                         row("Presence - highs", result.spectrum.upperMidToHighDb, "#888888"));
    }

    // What the numbers above amount to, in sentences. Read off the same two results the tables are,
    // so the notes cannot describe a mix other than the one measured.
    if (const auto advice = MixAdvisor::advise(result.spectrum, result.loudness); !advice.findings.empty()) {
        QString notes;
        for (auto && finding : advice.findings) {
            const auto color = finding.severity == MixAdvisor::Severity::Caution ? "#FF9800" : "#bbbbbb";
            notes += QString { "<tr><td><font color='%1'>%2</font></td></tr>" }
                       .arg(color, MixAdviceFormatter::sentence(finding).toHtmlEscaped());
        }
        report += QString { "<br><table width='100%' cellpadding='5' cellspacing='0'>"
                            "<tr><td bgcolor='#2c2c2c'><b>%1</b></td></tr>%2</table>" }
                    .arg(QCoreApplication::translate("AnalysisReport", "Notes"), notes);
    }

    juzzlin::L(TAG).info() << "Analysis completed:\n"
                           << report.toStdString();
    return report;
}

QString text(const AudioAnalysis & analysis)
{
    const auto & result = analysis;

    QString report = "Noteahead loudness analysis\n\n";
    report += label("File:", fileName(analysis));
    report += label("Date:", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    report += label("Sample rate:", QString { "%1 Hz" }.arg(analysis.sampleRate));
    report += "\n";
    report += label("Integrated loudness:", number(result.loudness.integratedLoudness, "LUFS"));
    report += label("True peak:", number(result.loudness.truePeak, "dBTP"));
    report += label("Loudness range (LRA):", number(result.loudness.loudnessRange, "LU"));
    report += label("Threshold:", number(result.loudness.threshold, "LUFS"));

    if (result.spectrum.isValid) {
        const auto & spectrum = result.spectrum;
        report += "\nBalance\n";
        report += "Levels are relative to this mix's own 100 Hz - 8 kHz average, so they can be\n";
        report += "compared with another mix directly whatever the two were mastered to.\n\n";
        report += label("Low mids 100-250 Hz:", number(spectrum.lowMidDb, "dB"));
        report += label("Mids 250-630 Hz:", number(spectrum.midDb, "dB"));
        report += label("Presence 0.8-1.6 kHz:", number(spectrum.upperMidDb, "dB"));
        report += label("Highs 2.5-8 kHz:", number(spectrum.highDb, "dB"));
        report += label("Presence - highs:", number(spectrum.upperMidToHighDb, "dB"));

        report += "\nThird-octave average\n\n";
        for (const auto & band : spectrum.bands) {
            // A bar as well as the number: the shape of a balance is read across the bands at a
            // glance, and a column of figures hides it.
            const int bar = std::clamp(static_cast<int>(std::lround(band.levelDb + 18.0)), 0, 40);
            report += QString { "%1%2  %3\n" }
                        .arg(hz(band.centerHz), -10)
                        .arg(QString { "%1 dB" }.arg(band.levelDb, 6, 'f', 1))
                        .arg(QString { "#" }.repeated(bar));
        }
    }

    // The same findings the dialog shows, from the same call: the file and the dialog disagreeing
    // about a mix would be worse than neither saying anything.
    if (const auto advice = MixAdvisor::advise(result.spectrum, result.loudness); !advice.findings.empty()) {
        report += "\n" + QCoreApplication::translate("AnalysisReport", "Notes") + "\n\n";
        for (auto && finding : advice.findings) {
            report += "- " + MixAdviceFormatter::sentence(finding) + "\n";
        }
    }

    return report;
}

QString comparisonText(const AudioAnalysis & a, const AudioAnalysis & b)
{
    QString report = "Noteahead audio file comparison\n\n";
    report += label("Date:", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    report += label("A:", a.isValid ? fileName(a) : QString { "-" });
    report += label("B:", b.isValid ? fileName(b) : QString { "-" });

    if (a.isValid && b.isValid && a.spectrum.isValid && b.spectrum.isValid) {
        report += "\nBalance, B - A\n";
        report += "Each file's bands are relative to its own 100 Hz - 8 kHz average, so the\n";
        report += "difference is what separates the two balances whatever they were mastered to.\n";
        report += "A positive number means B has more there, and is the cut B needs to match A.\n\n";
        report += QString { "%1%2%3%4\n" }.arg("Band", -10).arg("A", 10).arg("B", 10).arg("B - A", 10);

        // Bands are the same third-octave centres for any file, but a lower sample rate stops
        // earlier, so the two are walked by centre rather than by index.
        for (auto && bandA : a.spectrum.bands) {
            const auto & bandsB = b.spectrum.bands;
            const auto it = std::find_if(bandsB.begin(), bandsB.end(), [&](auto && candidate) {
                return std::abs(candidate.centerHz - bandA.centerHz) < 0.1;
            });
            if (it == bandsB.end()) {
                continue;
            }
            report += QString { "%1%2%3%4\n" }
                        .arg(hz(bandA.centerHz), -10)
                        .arg(QString { "%1" }.arg(bandA.levelDb, 0, 'f', 1), 10)
                        .arg(QString { "%1" }.arg(it->levelDb, 0, 'f', 1), 10)
                        .arg(QString { "%1" }.arg(it->levelDb - bandA.levelDb, 0, 'f', 1), 10);
        }

        const auto summary = [&](const QString & name, float valueA, float valueB) {
            return QString { "%1%2%3%4\n" }
              .arg(name, -22)
              .arg(QString { "%1" }.arg(valueA, 0, 'f', 1), 8)
              .arg(QString { "%1" }.arg(valueB, 0, 'f', 1), 8)
              .arg(QString { "%1" }.arg(valueB - valueA, 0, 'f', 1), 8);
        };
        report += "\n";
        report += QString { "%1%2%3%4\n" }.arg("", -22).arg("A", 8).arg("B", 8).arg("B - A", 8);
        report += summary("Low mids 100-250 Hz:", a.spectrum.lowMidDb, b.spectrum.lowMidDb);
        report += summary("Mids 250-630 Hz:", a.spectrum.midDb, b.spectrum.midDb);
        report += summary("Presence 0.8-1.6 kHz:", a.spectrum.upperMidDb, b.spectrum.upperMidDb);
        report += summary("Highs 2.5-8 kHz:", a.spectrum.highDb, b.spectrum.highDb);
        report += summary("Integrated LUFS:", a.loudness.integratedLoudness, b.loudness.integratedLoudness);
        report += summary("True peak dBTP:", a.loudness.truePeak, b.loudness.truePeak);
    }

    // Each file's own report in full after the comparison: what the difference is made of, for the
    // side that is being matched as much as for the one being matched to.
    if (a.isValid) {
        report += "\n\nA\n=\n\n" + text(a);
    }
    if (b.isValid) {
        report += "\n\nB\n=\n\n" + text(b);
    }

    return report;
}

QString analysisFilePath(const QString & renderedPath)
{
    return renderedPath + ".loudness.txt";
}

bool writeReportFile(const QString & path, const QString & report)
{
    QFile file { path };
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        juzzlin::L(TAG).error() << "Failed to write the analysis file: " << path.toStdString();
        return false;
    }
    QTextStream stream { &file };
    stream << report;
    file.close();
    juzzlin::L(TAG).info() << "Analysis written to " << path.toStdString();
    return true;
}

} // namespace noteahead::AnalysisReportFormatter
