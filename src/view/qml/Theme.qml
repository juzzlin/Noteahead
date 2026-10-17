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

pragma Singleton
import QtQuick 2.15

//! The colours the UI is drawn in, by what they are for rather than by what they are.
//!
//! A dialog asks for borderSubtle or textSecondary, never for "#333" or "#aaa": one value per role,
//! so the look can change here without touching the places that use it. The accent colour is the
//! user's and comes from themeService; everything else lives here.
//!
//! The values are the ones the UI had before there was a theme, so introducing this changed nothing
//! on screen. Several roles still share a value -- separator, borderSubtle and surfacePanel are all
//! the same grey -- and are kept apart because they will not stay the same.
QtObject {
    // Surfaces, from the deepest to the most raised
    readonly property color surfaceSunken: "#111111"
    readonly property color surfaceDialog: "#1e1e1e"
    readonly property color surfaceInset: "#222222"
    readonly property color surfacePanel: "#333333"
    readonly property color surfaceRaised: "#555555"

    // Borders, and the one-pixel rules that divide a dialog's sections
    readonly property color borderSubtle: "#333333"
    readonly property color borderMedium: "#444444"
    readonly property color borderStrong: "#555555"
    readonly property color separator: "#333333"
    readonly property color separatorStrong: "#555555"

    // Text, from the most to the least prominent
    readonly property color textBright: "#dddddd"
    readonly property color textSecondary: "#aaaaaa"
    readonly property color textTertiary: "#999999"
    readonly property color textMuted: "#888888"
    readonly property color textFaint: "#777777"
    readonly property color textDim: "#666666"

    // States
    readonly property color danger: "#ff6060"
}
