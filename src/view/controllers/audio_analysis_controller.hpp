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

#ifndef AUDIO_ANALYSIS_CONTROLLER_HPP
#define AUDIO_ANALYSIS_CONTROLLER_HPP

#include <QObject>
#include <QStringList>
#include <QUrl>

#include <memory>

namespace noteahead {

class AudioAnalysisService;

//! Exposes the audio analysis tool to QML: two files, their reports, and the chart of both.
//!
//! A translator of the service's results, as every controller here is: the measuring, the comparing
//! and the report text all belong to the service and are testable without a window.
class AudioAnalysisController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString leftFileName READ leftFileName NOTIFY analysisChanged)
    Q_PROPERTY(QString rightFileName READ rightFileName NOTIFY analysisChanged)
    Q_PROPERTY(QString leftReport READ leftReport NOTIFY analysisChanged)
    Q_PROPERTY(QString rightReport READ rightReport NOTIFY analysisChanged)
    Q_PROPERTY(bool hasLeft READ hasLeft NOTIFY analysisChanged)
    Q_PROPERTY(bool hasRight READ hasRight NOTIFY analysisChanged)
    Q_PROPERTY(bool isAnalyzing READ isAnalyzing NOTIFY isAnalyzingChanged)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged)

public:
    using AudioAnalysisServiceS = std::shared_ptr<AudioAnalysisService>;

    explicit AudioAnalysisController(AudioAnalysisServiceS audioAnalysisService, QObject * parent = nullptr);

    Q_INVOKABLE void analyzeLeft(const QUrl & fileUrl);
    Q_INVOKABLE void analyzeRight(const QUrl & fileUrl);
    //! Same two, for a path out of the recent list rather than out of a file dialog.
    Q_INVOKABLE void analyzeLeftPath(const QString & filePath);
    Q_INVOKABLE void analyzeRightPath(const QString & filePath);
    //! The recent paths' file names, for a menu that would otherwise be a column of directories.
    Q_INVOKABLE QStringList recentFileNames() const;
    Q_INVOKABLE void clearLeft();
    Q_INVOKABLE void clearRight();
    Q_INVOKABLE void swap();
    Q_INVOKABLE bool saveReport(const QUrl & fileUrl);

    //! A name for the saved report, from whichever files are loaded.
    Q_INVOKABLE QString defaultReportFileName() const;

    //! Pushes both files' bands into a SpectrumCompareRenderer.
    //!
    //! Takes QObject* rather than the renderer's own type: a forward-declared pointer parameter
    //! makes QML drop the call altogether. Same reason as EffectRackController::rtaUpdateRenderer().
    Q_INVOKABLE void updateRenderer(QObject * renderer);

    QString leftFileName() const;
    QString rightFileName() const;
    QString leftReport() const;
    QString rightReport() const;
    bool hasLeft() const;
    bool hasRight() const;
    bool isAnalyzing() const;
    QStringList recentFiles() const;

signals:
    void analysisChanged();
    void isAnalyzingChanged();
    void recentFilesChanged();
    void errorOccurred(QString message);

private:
    AudioAnalysisServiceS m_audioAnalysisService;
};

} // namespace noteahead

#endif // AUDIO_ANALYSIS_CONTROLLER_HPP
