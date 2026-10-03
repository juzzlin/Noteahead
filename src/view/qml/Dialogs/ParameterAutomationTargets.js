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

// Naming of parameter automation targets, shared by the add form and the edit rows so that an
// effect is called the same thing wherever it is shown. The targets themselves come from
// propertyService.automationTargets().

//! What the target is and where it sits: "Device 2 (Synth)" for a device, and for an effect e.g.
//! "Device 2 (Synth) / Insert 3: Auto Filter".
function targetLabel(target) {
    if (!target) {
        return qsTr("Missing target");
    }
    if (target.owner === "device") {
        return qsTr("Device %1 (%2)").arg(target.deviceSlot + 1).arg(target.deviceTypeName);
    }
    // By type rather than type id: that is what the effect gallery keys its names by
    const name = effectRackController.effectDisplayName(target.type);
    switch (target.rack) {
    case 0:
        return qsTr("Device %1 (%2) / Insert %3: %4").arg(target.deviceSlot + 1).arg(target.deviceTypeName).arg(target.effectSlot + 1).arg(name);
    case 1:
        return qsTr("Device %1 (%2) / Rack %3, insert %4: %5").arg(target.deviceSlot + 1).arg(target.deviceTypeName).arg(target.rackIndex + 1).arg(target.effectSlot + 1).arg(name);
    case 2:
        return qsTr("Master / Insert %1: %2").arg(target.effectSlot + 1).arg(name);
    case 3:
        return qsTr("Send %1: %2").arg(target.rackIndex + 1).arg(name);
    case 4:
        return qsTr("Send %1 / Insert %2: %3").arg(target.rackIndex + 1).arg(target.effectSlot + 1).arg(name);
    }
    return name;
}

function findTarget(targets, owner, ownerId) {
    return targets[findTargetIndex(targets, owner, ownerId)] || null;
}

//! Index of the target in @p targets, or -1 when it is not there.
function findTargetIndex(targets, owner, ownerId) {
    for (let i = 0; i < targets.length; i++) {
        if (targets[i].owner === owner && targets[i].ownerId === ownerId) {
            return i;
        }
    }
    return -1;
}

function findParameter(target, key) {
    if (target) {
        for (let i = 0; i < target.parameters.length; i++) {
            if (target.parameters[i].key === key) {
                return target.parameters[i];
            }
        }
    }
    return null;
}

//! What a knob showing @p parameter in @p mode is set up with: the parameter's own readout when
//! absolute, a plain percentage when relative, which is how far towards the knob the control goes.
function knobSetup(parameter, relative) {
    if (!parameter || relative) {
        return { suffix: "%", mapping: "linear", min: 0, max: 100 };
    }
    return { suffix: parameter.unit, mapping: parameter.mapping, min: parameter.min, max: parameter.max };
}

//! The value a relative automation at @p fraction reaches with the knob where it is now, read out in
//! the parameter's own units.
function relativeReadout(parameter, fraction) {
    if (!parameter) {
        return "";
    }
    const position = parameter.rest + (parameter.knob - parameter.rest) * fraction;
    return knobController.format(position, parameter.mapping, parameter.unit, parameter.min, parameter.max);
}
