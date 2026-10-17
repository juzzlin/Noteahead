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
import Noteahead 1.0

// The screenshot tour behind --screenshots: the example song's main view, then every dialog, each
// written to a PNG, and then the application quits. What a UI change is reviewed against, at whatever
// --window-size the run is given.
//
// Addresses the dialogs by their ids in Main.qml, as EffectDialogLauncher does.
Item {
    id: tour

    //! Slot the devices and the master insert effects are tried in, out of the way of the example's own
    readonly property int spareSlot: 15
    property var steps: []
    property int index: -1

    function safeName(name: string): string {
        return name.replace(/[^A-Za-z0-9]+/g, "_").replace(/^_|_$/g, "");
    }

    function buildSteps(): void {
        const list = [{ name: "main", action: () => {} }];
        const general = { "about": aboutDialog, "shortcuts": shortcutsDialog, "effects_gallery": effectsGalleryDialog, "device_gallery": deviceGalleryDialog, "recent_files": recentFilesDialog, "device_rack": deviceRackDialog, "mixer": mixerDialog, "master_effects": masterEffectsDialog, "song_metadata": songMetadataDialog, "song_notes": songNotesDialog, "song_overview": songOverviewDialog, "song_settings": songSettingsDialog, "settings": settingsDialog, "manual": manualDialog, "whats_new": whatsNewDialog, "add_parameter_automation": addParameterAutomationDialog, "edit_parameter_automations": editParameterAutomationsDialog, "add_midi_cc_automation": addMidiCcAutomationDialog, "edit_midi_cc_automations": editMidiCcAutomationsDialog, "delay_calculator": delayCalculatorDialog, "gain_converter": gainConverterDialog, "note_frequency": noteFrequencyDialog, "audio_render": audioRenderDialog, "midi_export": midiExportDialog };
        for (const name in general) {
            const dialog = general[name];
            list.push({ name: "dialog_" + name, action: () => dialog.open() });
        }
        for (const device of deviceRackController.availableDevices()) {
            list.push({ name: "device_" + safeName(device.name), action: () => {
                deviceRackController.setDevice(tour.spareSlot, device.typeId);
                deviceRackController.openDevice(tour.spareSlot);
            } });
        }
        for (const effect of effectRackController.availableEffects()) {
            list.push({ name: "effect_" + safeName(effect.name), action: () => {
                effectRackController.targetDeviceName = "";
                effectRackController.isInsertRack = true;
                effectRackController.setEffect(tour.spareSlot, effect.typeId);
                effectDialogLauncher.open(effectRackController.effectType(tour.spareSlot), tour.spareSlot);
            } });
        }
        steps = list;
    }

    //! Takes the shot of the step just shown, closes it, and waits for it to be gone before the next
    function next(): void {
        if (index >= 0) {
            const name = steps[index].name;
            console.info("Screenshot", name, applicationService.saveScreenshot(name) ? "saved" : "FAILED");
            applicationService.closeAllPopups();
        }
        index++;
        if (index >= steps.length) {
            // Not Qt.quit(): that asks the window to close, and the tour has left the example
            // edited, so it would stop at the unsaved changes question
            Qt.exit(0);
            return;
        }
        // A closing dialog fades out; opening the next one at once would show the two together
        closeWait.start();
    }

    Timer {
        id: closeWait
        interval: Constants.dialogExitTransitionDuration + 200
        onTriggered: {
            tour.steps[tour.index].action();
            shotWait.start();
        }
    }

    Timer {
        id: shotWait
        // Long enough for a dialog's open animation and a first render of anything it draws
        interval: Constants.dialogEnterTransitionDuration + 700
        onTriggered: tour.next()
    }

    Timer {
        // Opening the example while the application is still starting up is ignored, so it waits
        interval: 2000
        running: true
        onTriggered: {
            applicationService.requestOpenExample();
            exampleWait.start();
        }
    }

    Timer {
        id: exampleWait
        interval: 2500
        onTriggered: {
            tour.buildSteps();
            tour.next();
        }
    }
}
