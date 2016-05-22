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

#include "theme_service.hpp"

#include "../../common/constants.hpp"
#include "../../infra/settings.hpp"

#include <algorithm>
#include <cmath>

namespace noteahead {

namespace {

//! Consecutive entries of a golden-ratio sequence land far apart and it never short-cycles,
//! which keeps a palette usable even when every entry shares the accent hue.
constexpr auto goldenRatio = 0.6180339887498949;
//! WCAG contrast against the near-black editor background. Track names are drawn in these colors.
constexpr auto minContrastOnBlack = 4.5;
constexpr auto paletteSaturation = 0.72;
constexpr auto paletteLightnessMin = 0.46;
constexpr auto paletteLightnessMax = 0.88;

double relativeLuminance(const QColor & color)
{
    const auto linearize = [](double channel) {
        return channel <= 0.03928 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * linearize(color.redF()) + 0.7152 * linearize(color.greenF()) + 0.0722 * linearize(color.blueF());
}

double contrastOnBlack(const QColor & color)
{
    return (relativeLuminance(color) + 0.05) / 0.05;
}

//! Lowest lightness at which the given hue still reads against the editor background. Blues and
//! violets are intrinsically dark, so their usable band starts much higher than a yellow's.
double lightnessFloor(double hue)
{
    auto low = 0.0, high = paletteLightnessMax;
    for (int i = 0; i < 24; i++) {
        const auto mid = (low + high) / 2.0;
        const auto candidate = QColor::fromHslF(static_cast<float>(hue), static_cast<float>(paletteSaturation), static_cast<float>(mid));
        if (contrastOnBlack(candidate) >= minContrastOnBlack) {
            high = mid;
        } else {
            low = mid;
        }
    }
    return high;
}

//! Rotates around the shorter way, so a red pulled towards a magenta accent does not sweep
//! through the whole spectrum on the way.
double blendHue(double from, double to, double amount)
{
    auto delta = to - from;
    if (delta > 0.5) {
        delta -= 1.0;
    } else if (delta < -0.5) {
        delta += 1.0;
    }
    return std::fmod(from + delta * amount + 1.0, 1.0);
}

QColor blendColor(const QColor & from, const QColor & to, double amount)
{
    const auto toHue = static_cast<double>(to.hueF() < 0.0F ? 0.0F : to.hueF());
    // An achromatic source has no hue to rotate from, so start it at the target and let the
    // saturation alone carry it there
    const auto fromHue = from.hueF() < 0.0F ? toHue : static_cast<double>(from.hueF());
    const auto hue = blendHue(fromHue, toHue, amount);
    const auto saturation = from.hslSaturationF() + (to.hslSaturationF() - from.hslSaturationF()) * amount;
    const auto lightness = from.lightnessF() + (to.lightnessF() - from.lightnessF()) * amount;
    return QColor::fromHslF(static_cast<float>(hue), static_cast<float>(saturation), static_cast<float>(lightness));
}

} // namespace

ThemeService::ThemeService()
  : m_accentColor { Settings::accentColor(defaultAccentColor()) }
  , m_cursorColor { Settings::cursorColor(defaultCursorColor()) }
  , m_spectrumPostColor { Settings::spectrumPostColor() }
  , m_spectrumPreColor { Settings::spectrumPreColor() }
  , m_userThemePresets { Settings::userThemePresets() }
  , m_paletteAccentBlend { Settings::paletteAccentBlend(defaultPaletteAccentBlend()) }
  , m_volumeMeterStyle { Settings::volumeMeterStyle(Constants::defaultVolumeMeterStyle()) }
{
    // The theme last picked, by name, so that the list can change under it. A factory theme wins
    // over one of the user's own of the same name, which is the order the list shows them in.
    const auto name = Settings::themePresetName();
    const auto & factory = factoryThemePresets();
    if (const auto it = std::ranges::find_if(factory, [&name](auto && preset) { return preset.name == name; }); it != factory.end()) {
        m_themePresetIndex = static_cast<int>(std::distance(factory.begin(), it));
    } else if (const auto user = std::ranges::find_if(m_userThemePresets, [&name](auto && preset) { return preset.name == name; }); user != m_userThemePresets.end()) {
        m_themePresetIndex = static_cast<int>(factory.size()) + static_cast<int>(std::distance(m_userThemePresets.begin(), user));
    }
}

ThemeService::~ThemeService() = default;

QColor ThemeService::accentColor() const
{
    return m_accentColor;
}

void ThemeService::setAccentColor(const QColor & accentColor)
{
    if (m_accentColor != accentColor) {
        m_accentColor = accentColor;
        Settings::setAccentColor(m_accentColor);
        emit accentColorChanged();
        emit spectrumColorsChanged();
        emit trackHeaderTextColorsChanged();
        emit automationCurveColorsChanged();
    }
}

QColor ThemeService::defaultAccentColor()
{
    // Mint green
    return QColor { "#8ff0a4" };
}

int ThemeService::defaultPaletteAccentBlend()
{
    // A light tint: the original palette still shows through, just pulled towards the accent
    return 25;
}

int ThemeService::paletteAccentBlend() const
{
    return m_paletteAccentBlend;
}

void ThemeService::setPaletteAccentBlend(int paletteAccentBlend)
{
    if (const auto clamped = std::clamp(paletteAccentBlend, 0, 100); m_paletteAccentBlend != clamped) {
        m_paletteAccentBlend = clamped;
        Settings::setPaletteAccentBlend(m_paletteAccentBlend);
        emit paletteAccentBlendChanged();
        emit trackHeaderTextColorsChanged();
        emit automationCurveColorsChanged();
    }
}

int ThemeService::volumeMeterStyle() const
{
    return m_volumeMeterStyle;
}

void ThemeService::setVolumeMeterStyle(int style)
{
    if (m_volumeMeterStyle != style) {
        m_volumeMeterStyle = style;
        Settings::setVolumeMeterStyle(m_volumeMeterStyle);
        emit volumeMeterStyleChanged();
    }
}

ThemeService::Palette ThemeService::accentPalette(int count) const
{
    Palette palette;
    if (count <= 0) {
        return palette;
    }

    // An achromatic accent has no hue of its own, so anchor those at the top of the wheel
    const auto accentHue = m_accentColor.hueF() < 0.0F ? 0.0 : static_cast<double>(m_accentColor.hueF());
    // The band this hue can afford. Blues start much higher than yellows, and mapping into the
    // band rather than clamping afterwards keeps entries off a shared floor.
    const auto lightnessMin = std::max(paletteLightnessMin, lightnessFloor(accentHue));

    for (int i = 0; i < count; i++) {
        // Consecutive entries of a golden-ratio sequence land far apart and it never short-cycles,
        // so lightness alone can tell every entry apart
        const auto position = std::fmod(0.5 + i * goldenRatio, 1.0);
        const auto lightness = lightnessMin + (paletteLightnessMax - lightnessMin) * position;
        palette.append(QColor::fromHslF(static_cast<float>(accentHue), static_cast<float>(paletteSaturation), static_cast<float>(lightness)));
    }

    return palette;
}

ThemeService::Palette ThemeService::blendedPalette(const Palette & palette) const
{
    // Hand back the originals untouched rather than round-tripping them through HSL, so a blend
    // of 0 really is the palette the editor shipped with, to the byte
    if (!m_paletteAccentBlend) {
        return palette;
    }

    const auto accent = accentPalette(static_cast<int>(palette.size()));
    if (m_paletteAccentBlend >= 100) {
        return accent;
    }

    const auto amount = static_cast<double>(m_paletteAccentBlend) / 100.0;
    Palette blended;
    for (int i = 0; i < palette.size(); i++) {
        blended.append(blendColor(palette.at(i), accent.at(i), amount));
    }
    return blended;
}

QColor ThemeService::accentTextColor() const
{
    return contrastingTextColor(m_accentColor);
}

QColor ThemeService::defaultCursorColor()
{
    return QColor { "red" };
}

QColor ThemeService::cursorColor() const
{
    return m_cursorColor;
}

void ThemeService::setCursorColor(const QColor & cursorColor)
{
    if (m_cursorColor != cursorColor) {
        m_cursorColor = cursorColor;
        Settings::setCursorColor(m_cursorColor);
        emit cursorColorChanged();
    }
}

namespace {

//! A bright color @p hueOffset degrees round the hue circle from @p accent. Backwards from the
//! default mint that lands on yellow and orange, which stand apart from a mint curve and from each
//! other on the dark plot; any other accent gets the same relation to it.
QColor accentRelatedColor(const QColor & accent, int hueOffset)
{
    // A gray accent has no hue to turn from, so it is taken as the default mint's
    const int hue = accent.hsvHue() >= 0 ? accent.hsvHue() : ThemeService::defaultAccentColor().hsvHue();
    return QColor::fromHsv((hue + hueOffset + 360) % 360, 190, 250);
}

} // namespace

QColor ThemeService::spectrumPostColor() const
{
    return m_spectrumPostColor.value_or(accentRelatedColor(m_accentColor, -85));
}

void ThemeService::setSpectrumPostColor(const QColor & color)
{
    if (m_spectrumPostColor != color) {
        m_spectrumPostColor = color;
        Settings::setSpectrumPostColor(m_spectrumPostColor);
        emit spectrumColorsChanged();
    }
}

QColor ThemeService::spectrumPreColor() const
{
    return m_spectrumPreColor.value_or(accentRelatedColor(m_accentColor, -110));
}

void ThemeService::setSpectrumPreColor(const QColor & color)
{
    if (m_spectrumPreColor != color) {
        m_spectrumPreColor = color;
        Settings::setSpectrumPreColor(m_spectrumPreColor);
        emit spectrumColorsChanged();
    }
}

void ThemeService::setSpectrumColors(std::optional<QColor> post, std::optional<QColor> pre)
{
    if (m_spectrumPostColor != post || m_spectrumPreColor != pre) {
        m_spectrumPostColor = post;
        m_spectrumPreColor = pre;
        Settings::setSpectrumPostColor(m_spectrumPostColor);
        Settings::setSpectrumPreColor(m_spectrumPreColor);
        emit spectrumColorsChanged();
    }
}

const std::vector<ThemePreset> & ThemeService::factoryThemePresets()
{
    const auto lines = Constants::VolumeMeterStyle::Lines;
    const auto gradient = Constants::VolumeMeterStyle::Gradient;
    // Init first, as it is the way back; the rest alphabetical. The names are not translated, like
    // those of effect presets. A spectrum color left out follows the accent: Init's do, and the
    // others name theirs where turning from the accent would land somewhere that clashes with it.
    static const std::vector<ThemePreset> presets {
        { .name = "Init", .accentColor = defaultAccentColor(), .cursorColor = defaultCursorColor(), .paletteAccentBlend = defaultPaletteAccentBlend(), .volumeMeterStyle = Constants::defaultVolumeMeterStyle(), .spectrumPostColor = std::nullopt, .spectrumPreColor = std::nullopt },
        { .name = "Ice", .accentColor = QColor { "#7fd4ff" }, .cursorColor = QColor { "#ff5577" }, .paletteAccentBlend = 25, .volumeMeterStyle = static_cast<int>(lines), .spectrumPostColor = QColor { "#ffd75e" }, .spectrumPreColor = QColor { "#ff9a5e" } },
        { .name = "Mono", .accentColor = QColor { "#d0d0d0" }, .cursorColor = QColor { "white" }, .paletteAccentBlend = 0, .volumeMeterStyle = static_cast<int>(lines), .spectrumPostColor = QColor { "#a0a0a0" }, .spectrumPreColor = QColor { "#e8e8e8" } },
        // The accent and palette the application had before mint
        { .name = "Orange", .accentColor = QColor { "orange" }, .cursorColor = QColor { "red" }, .paletteAccentBlend = 0, .volumeMeterStyle = static_cast<int>(gradient), .spectrumPostColor = QColor { "#5ec8ff" }, .spectrumPreColor = QColor { "#b48cff" } },
        { .name = "Rose", .accentColor = QColor { "#ff8fb1" }, .cursorColor = QColor { "#ffd166" }, .paletteAccentBlend = 35, .volumeMeterStyle = static_cast<int>(gradient), .spectrumPostColor = QColor { "#8fe3ff" }, .spectrumPreColor = QColor { "#c8a2ff" } },
        { .name = "Violet", .accentColor = QColor { "#b69cff" }, .cursorColor = QColor { "#ff6b6b" }, .paletteAccentBlend = 25, .volumeMeterStyle = static_cast<int>(lines), .spectrumPostColor = QColor { "#ffd36b" }, .spectrumPreColor = QColor { "#ff9f6b" } },
    };
    return presets;
}

QStringList ThemeService::themePresetNames() const
{
    QStringList names;
    const auto append = [&names](const QString & name, bool isUserPreset) {
        // Numbered and marked as the effect presets are, so the two dropdowns read alike
        names.append(QString { "%1: %2%3" }
                       .arg(names.size(), 3, 10, QChar { '0' })
                       .arg(name)
                       .arg(isUserPreset ? Constants::userPresetMarker() : QString {}));
    };
    for (auto && preset : factoryThemePresets()) {
        append(preset.name, false);
    }
    for (auto && preset : m_userThemePresets) {
        append(preset.name, true);
    }
    return names;
}

int ThemeService::currentThemePresetIndex() const
{
    return m_themePresetIndex;
}

bool ThemeService::currentThemePresetIsUserPreset() const
{
    return m_themePresetIndex >= static_cast<int>(factoryThemePresets().size());
}

void ThemeService::setThemePresetIndex(int index)
{
    const int count = static_cast<int>(factoryThemePresets().size() + m_userThemePresets.size());
    m_themePresetIndex = std::clamp(index, 0, count - 1);
    const auto factoryCount = static_cast<int>(factoryThemePresets().size());
    Settings::setThemePresetName(m_themePresetIndex < factoryCount ? factoryThemePresets().at(static_cast<size_t>(m_themePresetIndex)).name : m_userThemePresets.at(static_cast<size_t>(m_themePresetIndex - factoryCount)).name);
    emit themePresetsChanged();
}

void ThemeService::loadThemePreset(int index)
{
    const auto factoryCount = static_cast<int>(factoryThemePresets().size());
    if (index < 0 || index >= factoryCount + static_cast<int>(m_userThemePresets.size())) {
        return;
    }
    const auto & preset = index < factoryCount ? factoryThemePresets().at(static_cast<size_t>(index)) : m_userThemePresets.at(static_cast<size_t>(index - factoryCount));
    const auto & init = factoryThemePresets().front();

    setAccentColor(preset.accentColor.value_or(*init.accentColor));
    setCursorColor(preset.cursorColor.value_or(*init.cursorColor));
    setPaletteAccentBlend(preset.paletteAccentBlend.value_or(*init.paletteAccentBlend));
    setVolumeMeterStyle(preset.volumeMeterStyle.value_or(*init.volumeMeterStyle));
    // Left out means following the accent, which is Init's own value, so there is nothing to fall
    // back on
    setSpectrumColors(preset.spectrumPostColor, preset.spectrumPreColor);
    setThemePresetIndex(index);
}

bool ThemeService::userPresetExists(const QString & name) const
{
    return std::ranges::any_of(m_userThemePresets, [&name](auto && preset) { return preset.name == name; });
}

bool ThemeService::saveUserPreset(const QString & name)
{
    if (name.trimmed().isEmpty()) {
        return false;
    }
    const ThemePreset preset { .name = name,
                               .accentColor = m_accentColor,
                               .cursorColor = m_cursorColor,
                               .paletteAccentBlend = m_paletteAccentBlend,
                               .volumeMeterStyle = m_volumeMeterStyle,
                               .spectrumPostColor = m_spectrumPostColor,
                               .spectrumPreColor = m_spectrumPreColor };
    if (const auto it = std::ranges::find_if(m_userThemePresets, [&name](auto && existing) { return existing.name == name; }); it != m_userThemePresets.end()) {
        *it = preset;
    } else {
        m_userThemePresets.push_back(preset);
    }
    Settings::setUserThemePresets(m_userThemePresets);

    // Show what was just saved as the selected theme, as saving an effect preset does
    const auto it = std::ranges::find_if(m_userThemePresets, [&name](auto && existing) { return existing.name == name; });
    setThemePresetIndex(static_cast<int>(factoryThemePresets().size()) + static_cast<int>(std::distance(m_userThemePresets.begin(), it)));
    return true;
}

bool ThemeService::deleteCurrentUserPreset()
{
    if (!currentThemePresetIsUserPreset()) {
        return false;
    }
    m_userThemePresets.erase(m_userThemePresets.begin() + (m_themePresetIndex - static_cast<int>(factoryThemePresets().size())));
    Settings::setUserThemePresets(m_userThemePresets);
    // Onto the theme that took the deleted one's place, or the last one left. The settings stay as
    // they are: deleting a theme is not a reason to change how the application looks.
    setThemePresetIndex(m_themePresetIndex);
    return true;
}

QString ThemeService::currentUserPresetName() const
{
    return currentThemePresetIsUserPreset() ? m_userThemePresets.at(static_cast<size_t>(m_themePresetIndex) - factoryThemePresets().size()).name : QString {};
}

QColor ThemeService::contrastingTextColor(const QColor & background) const
{
    // Perceived (Rec. 601) luminance in the 0..1 range
    const auto luminance = 0.299 * background.redF() + 0.587 * background.greenF() + 0.114 * background.blueF();
    return luminance > 0.6 ? QColor { "#555555" } : QColor { "white" };
}

QColor ThemeService::lineNumberColumnBackgroundColor() const
{
    return QColor { "black" };
}

QColor ThemeService::lineNumberColumnBorderColor() const
{
    return QColor { "#444444" };
}

QColor ThemeService::lineNumberColumnCellBackgroundColor() const
{
    return QColor { "black" };
}

QColor ThemeService::lineNumberColumnCellBorderColor() const
{
    return QColor { "#222222" };
}

QColor ThemeService::lineNumberColumnOverflowTextColor() const
{
    return QColor { "#444444" };
}

QColor ThemeService::mainMenuTextColor() const
{
    return QColor { "white" };
}

QColor ThemeService::manualCodeBackgroundColor() const
{
    return QColor { "#333333" };
}

QColor ThemeService::manualRuleColor() const
{
    return QColor { "#444444" };
}

QColor ThemeService::manualFooterTextColor() const
{
    return QColor { "#888888" };
}

QColor ThemeService::mainToolBarGradientStartColor() const
{
    return QColor { "#303030" };
}

QColor ThemeService::mainToolBarGradientStopColor() const
{
    return QColor { "black" };
}

QColor ThemeService::mainToolBarSeparatorColor() const
{
    return QColor { "white" };
}

QColor ThemeService::mainToolBarTextColor() const
{
    return QColor { "white" };
}

QColor ThemeService::noteColumnBackgroundColor() const
{
    return QColor { "black" };
}

QColor ThemeService::noteColumnBorderColor() const
{
    return QColor { "#444444" };
}

QColor ThemeService::noteColumnCellBackgroundColor() const
{
    return QColor { "black" };
}

QColor ThemeService::noteColumnCellBorderColor() const
{
    return QColor { "#222222" };
}

ThemeService::Palette ThemeService::legacyAutomationCurveColors()
{
    // White first, so the common single-automation case reads as a plain trace; the rest are picked
    // to stay legible against the tracker's dark rows and apart from each other.
    return {
        QColor { "#ffffff" },
        QColor { "#00c8ff" },
        QColor { "#ffb000" },
        QColor { "#00e070" },
        QColor { "#ff5fa0" },
        QColor { "#b78cff" }
    };
}

QVariantList ThemeService::automationCurveColors() const
{
    QVariantList colors;
    for (auto && color : blendedPalette(legacyAutomationCurveColors())) {
        colors.append(color);
    }
    return colors;
}

QColor ThemeService::automationCurveCenterLineColor() const
{
    return QColor { "#808080" };
}

QColor ThemeService::noteColumnTextColor() const
{
    return QColor { "white" };
}

QColor ThemeService::noteColumnTextColorEmpty() const
{
    return QColor { "#888888" };
}

QColor ThemeService::noteColumnTextColorGhost() const
{
    return QColor { "#444444" };
}

QColor ThemeService::positionBarBorderColor() const
{
    return QColor { "white" };
}

QColor ThemeService::positionBarBorderColorEditMode() const
{
    return QColor { "red" };
}

QColor ThemeService::progressBarBackgroundColor() const
{
    return QColor { "#4a4a4a" };
}

QColor ThemeService::recentFileItemTextColor() const
{
    return QColor { "white" };
}

QColor ThemeService::trackBorderColor() const
{
    return QColor { "#222222" };
}

QColor ThemeService::trackHeaderBackgroundColor() const
{
    return QColor { "black" };
}

QColor ThemeService::trackHeaderBorderColor() const
{
    return QColor { "#222222" };
}

QColor ThemeService::velocityScaleTroughColor() const
{
    return QColor { "#3a3a3a" };
}

ThemeService::Palette ThemeService::legacyTrackHeaderTextColors() const
{
    return {
        m_accentColor,
        QColor { "white" },
        QColor { "#ff5555" },
        QColor { "#55ff55" },
        QColor { "#5555ff" },
        QColor { "#ffff55" },
        QColor { "#ff55ff" },
        QColor { "#55ffff" },
        QColor { "#aaaaaa" },
        QColor { "#ff8800" },
        QColor { "#88ff00" },
        QColor { "#0088ff" },
        QColor { "#ff0088" },
        QColor { "#8800ff" },
        QColor { "#00ff88" },
        QColor { "#888888" }
    };
}

QVariantList ThemeService::trackHeaderTextColors() const
{
    QVariantList colors;
    for (auto && color : blendedPalette(legacyTrackHeaderTextColors())) {
        colors.append(color);
    }
    return colors;
}

QColor ThemeService::trackHeaderTextColor(int trackIndex) const
{
    const auto colors = trackHeaderTextColors();
    return colors.at(trackIndex % colors.size()).value<QColor>();
}

} // namespace noteahead
