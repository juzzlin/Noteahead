import QtQuick 2.15

QtObject {
    property int trackIndex: 0
    property string portName
    property int channel: 0
    property bool patchEnabled: false
    property int patch: 0
    property bool bankEnabled: false
    property int bankLsb: 0
    property int bankMsb: 0
    property bool bankByteOrderSwapped: false

    function toMap() {
        let map = {};
        for (let key in this) {
            if (key !== "objectName" && typeof this[key] !== "function") {
                map[key] = this[key];
            }
        }
        return map;
    }

    function fromMap(map) {
        for (let key in map) {
            if (this.hasOwnProperty(key)) {
                this[key] = map[key];
            }
        }
    }
}
