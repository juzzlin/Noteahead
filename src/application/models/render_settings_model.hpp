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

#ifndef RENDER_SETTINGS_MODEL_HPP
#define RENDER_SETTINGS_MODEL_HPP

#include <functional>
#include <memory>

#include <QObject>

namespace noteahead {

class EditorService;
class RenderSettings;

//! The current song's render settings, for the render dialog.
//!
//! These live with the song rather than the application, so this reads and writes through the
//! EditorService's song and marks the project modified on every change.
class RenderSettingsModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int format READ format WRITE setFormat NOTIFY changed)
    Q_PROPERTY(int sampleRate READ sampleRate WRITE setSampleRate NOTIFY changed)
    Q_PROPERTY(int bitDepth READ bitDepth WRITE setBitDepth NOTIFY changed)
    Q_PROPERTY(int oversampleFactor READ oversampleFactor WRITE setOversampleFactor NOTIFY changed)
    Q_PROPERTY(bool normalizeEnabled READ normalizeEnabled WRITE setNormalizeEnabled NOTIFY changed)
    //! Tenths of a decibel, which is also how the dialog's slider works. The project format holds no
    //! floating-point values; see RenderSettings.
    Q_PROPERTY(int normalizeLevelTenthsDb READ normalizeLevelTenthsDb WRITE setNormalizeLevelTenthsDb NOTIFY changed)
    Q_PROPERTY(bool trimEnabled READ trimEnabled WRITE setTrimEnabled NOTIFY changed)
    Q_PROPERTY(int trimMinutes READ trimMinutes WRITE setTrimMinutes NOTIFY changed)
    Q_PROPERTY(int trimSeconds READ trimSeconds WRITE setTrimSeconds NOTIFY changed)
    //! Whole seconds plus tenths of a second, the same split the trim spin boxes use for minutes and
    //! seconds. The project format holds no floating-point values; see RenderSettings.
    Q_PROPERTY(bool fadeOutEnabled READ fadeOutEnabled WRITE setFadeOutEnabled NOTIFY changed)
    Q_PROPERTY(int fadeOutSeconds READ fadeOutSeconds WRITE setFadeOutSeconds NOTIFY changed)
    Q_PROPERTY(int fadeOutTenths READ fadeOutTenths WRITE setFadeOutTenths NOTIFY changed)
    Q_PROPERTY(bool silenceEnabled READ silenceEnabled WRITE setSilenceEnabled NOTIFY changed)
    Q_PROPERTY(int silenceSeconds READ silenceSeconds WRITE setSilenceSeconds NOTIFY changed)
    Q_PROPERTY(int silenceTenths READ silenceTenths WRITE setSilenceTenths NOTIFY changed)
    Q_PROPERTY(bool analyzeEnabled READ analyzeEnabled WRITE setAnalyzeEnabled NOTIFY changed)

public:
    using EditorServiceS = std::shared_ptr<EditorService>;

    explicit RenderSettingsModel(EditorServiceS editorService, QObject * parent = nullptr);
    ~RenderSettingsModel() override;

    int format() const;
    void setFormat(int format);

    int sampleRate() const;
    void setSampleRate(int sampleRate);

    int bitDepth() const;
    void setBitDepth(int bitDepth);

    int oversampleFactor() const;
    void setOversampleFactor(int factor);

    bool normalizeEnabled() const;
    void setNormalizeEnabled(bool enabled);

    int normalizeLevelTenthsDb() const;
    void setNormalizeLevelTenthsDb(int tenths);

    bool trimEnabled() const;
    void setTrimEnabled(bool enabled);

    int trimMinutes() const;
    void setTrimMinutes(int minutes);

    int trimSeconds() const;
    void setTrimSeconds(int seconds);

    bool fadeOutEnabled() const;
    void setFadeOutEnabled(bool enabled);

    int fadeOutSeconds() const;
    void setFadeOutSeconds(int seconds);

    int fadeOutTenths() const;
    void setFadeOutTenths(int tenths);

    bool silenceEnabled() const;
    void setSilenceEnabled(bool enabled);

    int silenceSeconds() const;
    void setSilenceSeconds(int seconds);

    int silenceTenths() const;
    void setSilenceTenths(int tenths);

    bool analyzeEnabled() const;
    void setAnalyzeEnabled(bool enabled);

signals:
    //! One signal for the lot: the dialog reads all of them together and there is nothing to gain
    //! from ten separate notifications.
    void changed();

private:
    //! Applies a change to the current song's settings and marks the project modified.
    void apply(const std::function<void(RenderSettings &)> & change);

    EditorServiceS m_editorService;
};

} // namespace noteahead

#endif // RENDER_SETTINGS_MODEL_HPP
