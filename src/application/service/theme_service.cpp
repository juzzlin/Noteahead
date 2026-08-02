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
  : m_accentColor { Settings::accentColor(QColor { "orange" }) }
  , m_cursorColor { Settings::cursorColor(defaultCursorColor()) }
  , m_paletteAccentBlend { Settings::paletteAccentBlend(defaultPaletteAccentBlend()) }
{
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
        emit trackHeaderTextColorsChanged();
        emit automationCurveColorsChanged();
    }
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
