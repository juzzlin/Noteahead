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
// The dialogs themselves are instantiated once in Main.qml and are addressed here by their ids, the
// same way the rack dialogs addressed them when they each held their own copy of this.
QtObject {
    // Opens the effect in slot index of the current rack. An empty slot, or a type with no dialog of
    // its own, opens the gallery so the slot can be filled instead.
    function open(effectType: string, index: int): void {
        if (effectType === effectRackController.allPassFilterType) {
            allPassFilterDialog.effectIndex = index;
            allPassFilterDialog.open();
        } else if (effectType === effectRackController.lufsMeterType) {
            lufsMeterDialog.effectIndex = index;
            lufsMeterDialog.open();
        } else if (effectType === effectRackController.dbtpMeterType) {
            dbtpMeterDialog.effectIndex = index;
            dbtpMeterDialog.open();
        } else if (effectType === effectRackController.clipperType) {
            clipperDialog.effectIndex = index;
            clipperDialog.open();
        } else if (effectType === effectRackController.saturatorType) {
            saturatorDialog.effectIndex = index;
            saturatorDialog.open();
        } else if (effectType === effectRackController.analogFuzzType) {
            analogFuzzDialog.effectIndex = index;
            analogFuzzDialog.open();
        } else if (effectType === effectRackController.monitorType) {
            monitorDialog.effectIndex = index;
            monitorDialog.open();
        } else if (effectType === effectRackController.gainType) {
            gainDialog.effectIndex = index;
            gainDialog.open();
        } else if (effectType === effectRackController.bassGrinderType) {
            bassGrinderDialog.effectIndex = index;
            bassGrinderDialog.open();
        } else if (effectType === effectRackController.tubeStageType) {
            tubeStageDialog.effectIndex = index;
            tubeStageDialog.open();
        } else if (effectType === effectRackController.waveDesignerType) {
            waveDesignerDialog.effectIndex = index;
            waveDesignerDialog.open();
        } else if (effectType === effectRackController.stereoFieldMeterType) {
            stereoFieldMeterDialog.effectIndex = index;
            stereoFieldMeterDialog.open();
        } else if (effectType === effectRackController.earlyReflectionsType) {
            earlyReflectionsDialog.effectIndex = index;
            earlyReflectionsDialog.open();
        } else if (effectType === effectRackController.dimensionType) {
            dimensionDialog.effectIndex = index;
            dimensionDialog.open();
        } else if (effectType === effectRackController.stereoWidenerType) {
            stereoWidenerDialog.effectIndex = index;
            stereoWidenerDialog.open();
        } else if (effectType === effectRackController.stereoEnhancerType) {
            stereoEnhancerDialog.effectIndex = index;
            stereoEnhancerDialog.open();
        } else if (effectType === effectRackController.stereoExciterType) {
            stereoExciterDialog.effectIndex = index;
            stereoExciterDialog.open();
        } else if (effectType === effectRackController.driveType) {
            driveDialog.effectIndex = index;
            driveDialog.open();
        } else if (effectType === effectRackController.limiterType) {
            limiterDialog.effectIndex = index;
            limiterDialog.open();
        } else if (effectType === effectRackController.compressorType) {
            compressorDialog.effectIndex = index;
            compressorDialog.open();
        } else if (effectType === effectRackController.multibandCompressorType) {
            multibandCompressorDialog.effectIndex = index;
            multibandCompressorDialog.open();
        } else if (effectType === effectRackController.autoDuckerType) {
            autoDuckerDialog.effectIndex = index;
            autoDuckerDialog.open();
        } else if (effectType === effectRackController.delayType) {
            delayDialog.effectIndex = index;
            delayDialog.open();
        } else if (effectType === effectRackController.eq8BandParametricType) {
            eq8BandParametricDialog.effectIndex = index;
            eq8BandParametricDialog.open();
        } else if (effectType === effectRackController.vintagePassiveEqType) {
            vintagePassiveEqDialog.effectIndex = index;
            vintagePassiveEqDialog.open();
        } else if (effectType === effectRackController.airBandEqType) {
            airBandEqDialog.effectIndex = index;
            airBandEqDialog.open();
        } else if (effectType === effectRackController.simpleEqType) {
            simpleEqDialog.effectIndex = index;
            simpleEqDialog.open();
        } else if (effectType === effectRackController.pannerType) {
            pannerDialog.effectIndex = index;
            pannerDialog.open();
        } else if (effectType === effectRackController.autoPannerType) {
            autoPannerDialog.effectIndex = index;
            autoPannerDialog.open();
        } else if (effectType === effectRackController.autoFilterType) {
            autoFilterDialog.effectIndex = index;
            autoFilterDialog.open();
        } else if (effectType === effectRackController.phaserType) {
            phaserDialog.effectIndex = index;
            phaserDialog.open();
        } else if (effectType === effectRackController.chorusType) {
            chorusDialog.effectIndex = index;
            chorusDialog.open();
        } else if (effectType === effectRackController.reverbType) {
            reverbDialog.effectIndex = index;
            reverbDialog.open();
        } else if (effectType === effectRackController.endlessType) {
            endlessReverbDialog.effectIndex = index;
            endlessReverbDialog.open();
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
