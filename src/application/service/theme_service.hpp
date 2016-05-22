// This file is part of Noteahead.
// Copyright (C) 2020 Jussi Lind <jussi.lind@iki.fi>
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

#ifndef THEME_SERVICE_HPP
#define THEME_SERVICE_HPP

#include "../../common/theme_preset.hpp"

#include <QColor>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <optional>
#include <vector>

namespace noteahead {

class ThemeService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(QColor accentTextColor READ accentTextColor NOTIFY accentColorChanged)

    //! Hue of the edit cursor. The renderer draws it half-transparent, so the note under it stays readable.
    Q_PROPERTY(QColor cursorColor READ cursorColor WRITE setCursorColor NOTIFY cursorColorChanged)

    //! The equalizers' live spectrum: the output, filled, and the input, outlined and in the
    //! difference modes. Derived from the accent color until the user picks one.
    Q_PROPERTY(QColor spectrumPostColor READ spectrumPostColor WRITE setSpectrumPostColor NOTIFY spectrumColorsChanged)
    Q_PROPERTY(QColor spectrumPreColor READ spectrumPreColor WRITE setSpectrumPreColor NOTIFY spectrumColorsChanged)

    //! The built-in themes, Init first, and then the user's own, as the preset dropdown lists them.
    Q_PROPERTY(QStringList themePresetNames READ themePresetNames NOTIFY themePresetsChanged)
    //! The theme last picked or saved. Left where it is when a setting is changed by hand, as an
    //! effect's preset is.
    Q_PROPERTY(int currentThemePresetIndex READ currentThemePresetIndex NOTIFY themePresetsChanged)
    Q_PROPERTY(bool currentThemePresetIsUserPreset READ currentThemePresetIsUserPreset NOTIFY themePresetsChanged)

    //! How far the track and automation palettes are pulled towards the accent color, as a
    //! percentage. 0 is the original fixed palette, 100 puts every entry on the accent hue.
    Q_PROPERTY(int paletteAccentBlend READ paletteAccentBlend WRITE setPaletteAccentBlend NOTIFY paletteAccentBlendChanged)

    //! How the tracker's per-column volume meters are drawn. See Constants::VolumeMeterStyle.
    Q_PROPERTY(int volumeMeterStyle READ volumeMeterStyle WRITE setVolumeMeterStyle NOTIFY volumeMeterStyleChanged)

    Q_PROPERTY(QColor lineNumberColumnBackgroundColor READ lineNumberColumnBackgroundColor CONSTANT)
    Q_PROPERTY(QColor lineNumberColumnBorderColor READ lineNumberColumnBorderColor CONSTANT)
    Q_PROPERTY(QColor lineNumberColumnCellBackgroundColor READ lineNumberColumnCellBackgroundColor CONSTANT)
    Q_PROPERTY(QColor lineNumberColumnCellBorderColor READ lineNumberColumnCellBorderColor CONSTANT)
    Q_PROPERTY(QColor lineNumberColumnOverflowTextColor READ lineNumberColumnOverflowTextColor CONSTANT)
    Q_PROPERTY(QColor mainMenuTextColor READ mainMenuTextColor CONSTANT)
    //! Backdrop of the monospace example blocks in the user manual.
    Q_PROPERTY(QColor manualCodeBackgroundColor READ manualCodeBackgroundColor CONSTANT)
    //! Rules and table cell borders in the user manual, and its footer text.
    Q_PROPERTY(QColor manualRuleColor READ manualRuleColor CONSTANT)
    Q_PROPERTY(QColor manualFooterTextColor READ manualFooterTextColor CONSTANT)
    Q_PROPERTY(QColor mainToolBarGradientStartColor READ mainToolBarGradientStartColor CONSTANT)
    Q_PROPERTY(QColor mainToolBarGradientStopColor READ mainToolBarGradientStopColor CONSTANT)
    Q_PROPERTY(QColor mainToolBarSeparatorColor READ mainToolBarSeparatorColor CONSTANT)
    Q_PROPERTY(QColor mainToolBarTextColor READ mainToolBarTextColor CONSTANT)
    Q_PROPERTY(QColor noteColumnBackgroundColor READ noteColumnBackgroundColor CONSTANT)
    Q_PROPERTY(QColor noteColumnBorderColor READ noteColumnBorderColor CONSTANT)
    Q_PROPERTY(QColor noteColumnCellBackgroundColor READ noteColumnCellBackgroundColor CONSTANT)
    Q_PROPERTY(QColor noteColumnCellBorderColor READ noteColumnCellBorderColor CONSTANT)
    //! Colours automation curves cycle through, so several automations on one column stay apart.
    Q_PROPERTY(QVariantList automationCurveColors READ automationCurveColors NOTIFY automationCurveColorsChanged)
    Q_PROPERTY(QColor automationCurveCenterLineColor READ automationCurveCenterLineColor CONSTANT)
    Q_PROPERTY(QColor noteColumnTextColor READ noteColumnTextColor CONSTANT)
    Q_PROPERTY(QColor noteColumnTextColorEmpty READ noteColumnTextColorEmpty CONSTANT)
    //! Dim gray, so a neighboring pattern peeked in the offset area reads as a subordinate glimpse.
    Q_PROPERTY(QColor noteColumnTextColorGhost READ noteColumnTextColorGhost CONSTANT)
    Q_PROPERTY(QColor positionBarBorderColor READ positionBarBorderColor CONSTANT)
    Q_PROPERTY(QColor positionBarBorderColorEditMode READ positionBarBorderColorEditMode CONSTANT)
    Q_PROPERTY(QColor progressBarBackgroundColor READ progressBarBackgroundColor CONSTANT)
    Q_PROPERTY(QColor recentFileItemTextColor READ recentFileItemTextColor CONSTANT)
    Q_PROPERTY(QColor trackBorderColor READ trackBorderColor CONSTANT)
    Q_PROPERTY(QColor trackHeaderBackgroundColor READ trackHeaderBackgroundColor CONSTANT)
    Q_PROPERTY(QColor trackHeaderBorderColor READ trackHeaderBorderColor CONSTANT)
    //! Unfilled part of a velocity scale widget. The filled part is drawn in the accent color.
    Q_PROPERTY(QColor velocityScaleTroughColor READ velocityScaleTroughColor CONSTANT)

    Q_PROPERTY(QVariantList trackHeaderTextColors READ trackHeaderTextColors NOTIFY trackHeaderTextColorsChanged)

public:
    using Palette = QList<QColor>;

    ThemeService();
    ~ThemeService() override;

    QColor accentColor() const;
    void setAccentColor(const QColor & accentColor);

    QColor accentTextColor() const;

    static QColor defaultAccentColor();

    QColor cursorColor() const;
    void setCursorColor(const QColor & cursorColor);

    static QColor defaultCursorColor();

    QColor spectrumPostColor() const;
    void setSpectrumPostColor(const QColor & color);
    QColor spectrumPreColor() const;
    void setSpectrumPreColor(const QColor & color);

    //! The themes that ship with the application. Init is the defaults, and the way back to them.
    static const std::vector<ThemePreset> & factoryThemePresets();

    QStringList themePresetNames() const;
    int currentThemePresetIndex() const;
    bool currentThemePresetIsUserPreset() const;

    //! Puts every setting a theme holds to what theme @p index has, Init's where it has none.
    Q_INVOKABLE void loadThemePreset(int index);

    // What the shared preset row and its Save and Delete dialogs ask of whatever they act on
    Q_INVOKABLE bool userPresetExists(const QString & name) const;
    //! Saves the current settings as a theme of the user's own, replacing one of the same name.
    Q_INVOKABLE bool saveUserPreset(const QString & name);
    Q_INVOKABLE bool deleteCurrentUserPreset();
    //! The name of the current theme if it is one of the user's own, otherwise empty.
    Q_INVOKABLE QString currentUserPresetName() const;

    int paletteAccentBlend() const;
    void setPaletteAccentBlend(int paletteAccentBlend);

    int volumeMeterStyle() const;
    void setVolumeMeterStyle(int style);

    static int defaultPaletteAccentBlend();

    //! Returns a legible text color (near-black or white) for the given background,
    //! chosen from the background's perceived luminance.
    Q_INVOKABLE QColor contrastingTextColor(const QColor & background) const;

    QColor lineNumberColumnBackgroundColor() const;
    QColor lineNumberColumnBorderColor() const;
    QColor lineNumberColumnCellBackgroundColor() const;
    QColor lineNumberColumnCellBorderColor() const;
    QColor lineNumberColumnOverflowTextColor() const;
    QColor mainMenuTextColor() const;
    QColor manualCodeBackgroundColor() const;
    QColor manualRuleColor() const;
    QColor manualFooterTextColor() const;
    QColor mainToolBarGradientStartColor() const;
    QColor mainToolBarGradientStopColor() const;
    QColor mainToolBarSeparatorColor() const;
    QColor mainToolBarTextColor() const;
    QColor noteColumnBackgroundColor() const;
    QColor noteColumnBorderColor() const;
    QColor noteColumnCellBackgroundColor() const;
    QColor noteColumnCellBorderColor() const;
    QVariantList automationCurveColors() const;
    QColor automationCurveCenterLineColor() const;
    QColor noteColumnTextColor() const;
    QColor noteColumnTextColorEmpty() const;
    QColor noteColumnTextColorGhost() const;
    QColor positionBarBorderColor() const;
    QColor positionBarBorderColorEditMode() const;
    QColor progressBarBackgroundColor() const;
    QColor recentFileItemTextColor() const;
    QColor trackBorderColor() const;
    QColor trackHeaderBackgroundColor() const;
    QColor trackHeaderBorderColor() const;
    QColor velocityScaleTroughColor() const;

    QVariantList trackHeaderTextColors() const;

    Q_INVOKABLE QColor trackHeaderTextColor(int trackIndex) const;

signals:
    void accentColorChanged();
    void cursorColorChanged();
    void spectrumColorsChanged();
    void themePresetsChanged();
    void paletteAccentBlendChanged();
    void volumeMeterStyleChanged();
    void automationCurveColorsChanged();
    void trackHeaderTextColorsChanged();

private:
    //! The fixed palettes the editor shipped with, returned unchanged at a blend of 0.
    Palette legacyTrackHeaderTextColors() const;
    static Palette legacyAutomationCurveColors();

    //! An all-accent-hue palette of the given size, told apart by lightness alone. This is what a
    //! blend of 100 resolves to, so its entries stay unique and legible on the editor background.
    Palette accentPalette(int count) const;

    //! Pulls each entry of the given palette towards its accent counterpart by the configured blend.
    Palette blendedPalette(const Palette & palette) const;

    QColor m_accentColor;
    QColor m_cursorColor;
    std::optional<QColor> m_spectrumPostColor;
    std::optional<QColor> m_spectrumPreColor;

    void setSpectrumColors(std::optional<QColor> post, std::optional<QColor> pre);
    void setThemePresetIndex(int index);

    std::vector<ThemePreset> m_userThemePresets;
    int m_themePresetIndex { 0 };
    int m_paletteAccentBlend;
    int m_volumeMeterStyle;
};

} // namespace noteahead

#endif // THEME_SERVICE_HPP
