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

// Opens the dialog belonging to an effect type, for whichever rack the EffectRackController is
// currently pointed at. Every rack dialog shares this: the mapping from a type to its dialog is the
// same wherever the rack lives, and having had a copy per dialog meant a new effect was only ever
// half added until every copy had been found.
//
// The dialogs of the effects that register themselves are created by the PluginDialogHost. The rest
// are instantiated once in Main.qml and are addressed here by their ids, the same way the rack
// dialogs addressed them when they each held their own copy of this.
QtObject {
    // Opens the effect in slot index of the current rack. An empty slot, or a type with no dialog of
    // its own, opens the gallery so the slot can be filled instead.
    function open(effectType: string, index: int): void {
        // An effect that registers itself names its own dialog
        const dialogFile = effectRackController.effectDialogFile(effectType);
        if (dialogFile !== "") {
            pluginDialogHost.open(dialogFile, {
                "effectIndex": index
            });
            return;
        }
        if (effectType === effectRackController.lufsMeterType) {
            lufsMeterDialog.effectIndex = index;
            lufsMeterDialog.open();
        } else if (effectType === effectRackController.dbtpMeterType) {
            dbtpMeterDialog.effectIndex = index;
            dbtpMeterDialog.open();
        } else if (effectType === effectRackController.stereoFieldMeterType) {
            stereoFieldMeterDialog.effectIndex = index;
            stereoFieldMeterDialog.open();
        } else if (effectType === effectRackController.rtaType) {
            rtaDialog.effectIndex = index;
            rtaDialog.open();
        } else {
            // An empty slot, and equally a type whose dialog is not listed above: either way there
            // is nothing to open but the gallery, which fills the slot.
            UiService.requestEffectsGalleryDialog(index);
        }
    }
}
