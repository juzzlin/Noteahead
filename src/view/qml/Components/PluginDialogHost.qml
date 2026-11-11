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

// Creates the dialogs of the effects and devices that register themselves, which Main.qml no longer
// lists: each on first use, from the file name its PluginCatalog entry gives, and kept for the next
// time so that it opens as fast as the dialogs created up front.
QtObject {
    id: host
    // Where the dialogs are created, which is what they size themselves against
    property Item dialogParent
    property var _dialogs: ({})

    // The dialog in file, created if this is the first time it is asked for. Null if it cannot be.
    function dialog(file: string): var {
        if (!_dialogs[file]) {
            const component = Qt.createComponent(Qt.resolvedUrl("../Dialogs/" + file));
            if (component.status !== Component.Ready) {
                console.error("PluginDialogHost: cannot load", file, component.errorString());
                return null;
            }
            const created = component.createObject(dialogParent);
            created.anchors.centerIn = dialogParent;
            _dialogs[file] = created;
        }
        return _dialogs[file];
    }

    // Opens the dialog in file, setting the given properties on it first
    function open(file: string, properties: var): void {
        const created = dialog(file);
        if (created) {
            for (const name in properties) {
                created[name] = properties[name];
            }
            created.open();
        }
    }
}
