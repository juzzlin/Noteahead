import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import ".."

GroupBox {
    title: qsTr("Generic MIDI CC Settings")
    Layout.fillWidth: true
    width: parent.width
    ColumnLayout {
        spacing: 8
        width: parent.width
        Repeater {
            id: midiCcRepeater
            model: trackSettingsModel.midiCcSlots
            MidiCcSelector {
            }
            onItemAdded: (index, item) => {
                item.index = index;
                _midiCcSelectors.push(item);
                item.settingsChanged.connect(() => {
                        trackSettingsModel.setMidiCcEnabled(item.index, item.enabled());
                        trackSettingsModel.setMidiCcController(item.index, item.controller());
                        trackSettingsModel.setMidiCcValue(item.index, item.value());
                        rootItem._requestApplyAll();
                    });
            }
        }
    }
}
