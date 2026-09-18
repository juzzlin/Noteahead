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

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 1.15
import Noteahead 1.0

// The OUT region of a mixer strip: one reading of the device's output at a time, out of several the
// same tap can be read in. Left-click cycles, right-click picks from the menu.
//
// A mixer strip has room for one readout and there are more worth showing than that, so the views
// take turns instead of competing for the width. Which one is showing is decided by the dialog and
// is the same on every strip -- a column of strips is only comparable when all of them show the
// same thing -- so this asks for a change rather than making one.
//
// Adding a view is three things: a file next to this one, a Component wrapping it below, and an
// entry in views. Nothing else here knows how many there are.
Item {
    id: root

    //! Everything the device's output taps currently read, as handed over by
    //! DeviceRackController::deviceOutputMeters(). A map so that a view added later reads a key of
    //! its own rather than an index every other view has to agree about.
    property var readings: ({})

    //! Id of the view to show. An unknown one falls back to the first, which is what makes a stored
    //! setting from another version harmless.
    property string viewId: ""

    //! Emitted with the id the user asked for. Nothing switches until the owner sets viewId.
    signal viewChangeRequested(string id)

    readonly property var views: {
        // The labels are built in a JS block, which Qt cannot rebind on a language change by itself:
        // naming the active language here is what declares the dependency.
        languageService.activeLanguage;
        return [
            {
                "id": "level",
                "label": qsTr("Level"),
                "component": levelView
            },
            {
                "id": "loudness",
                "label": qsTr("Loudness"),
                "component": loudnessView
            }
        ];
    }

    readonly property int currentIndex: {
        for (let i = 0; i < root.views.length; i++) {
            if (root.views[i].id === root.viewId) {
                return i;
            }
        }
        return 0;
    }

    //! Told to the views so that the way to switch is discoverable from the readout itself.
    readonly property string switchHint: qsTr("Click to switch the reading, right-click to pick one.")

    implicitWidth: 90
    implicitHeight: loader.implicitHeight

    Component {
        id: levelView

        OutputMeter_Level {
            peakDb: root.readings.peakDb !== undefined ? root.readings.peakDb : -120
            rmsDb: root.readings.rmsDb !== undefined ? root.readings.rmsDb : -120
            markerDb: settingsService.gainStagingTargetDb
            switchHint: root.switchHint
        }
    }

    Component {
        id: loudnessView

        LoudnessReadout {
            shortTermLufs: root.readings.shortTermLufs !== undefined ? root.readings.shortTermLufs : -70
            integratedLufs: root.readings.integratedLufs !== undefined ? root.readings.integratedLufs : -70
            switchHint: root.switchHint
        }
    }

    Loader {
        id: loader
        anchors.fill: parent
        sourceComponent: root.views[root.currentIndex].component
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        // Deliberately not hover-enabled: the loaded view has a hover area of its own for its
        // tooltip, and a hovering MouseArea on top of it would swallow that. The cursor shape does
        // not need hovering to be applied.
        cursorShape: Qt.PointingHandCursor
        onClicked: mouse => {
            if (mouse.button === Qt.RightButton) {
                viewMenu.popup();
            } else {
                root.viewChangeRequested(root.views[(root.currentIndex + 1) % root.views.length].id);
            }
        }
    }

    Menu {
        id: viewMenu
        // A popup does not inherit the theme of the dialog it was declared in, so it says what it
        // is itself -- otherwise the menu is the odd light-styled thing in a dark dialog.
        Universal.theme: Universal.Dark
        Universal.accent: themeService.accentColor

        Repeater {
            model: root.views

            // MenuItemDelegate rather than MenuItem for the same reason the rest of the application
            // uses it: a plain MenuItem drops clicks while the audio engine is running. It draws the
            // label itself and leaves no room for a check indicator, so the mark goes in the text --
            // with blanks on the others, which keeps the labels in one column either way.
            MenuItemDelegate {
                text: (modelData.id === root.views[root.currentIndex].id ? "✓ " : "    ") + modelData.label
                onTriggered: root.viewChangeRequested(modelData.id)
            }
        }
    }
}
