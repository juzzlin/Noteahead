import QtQuick 2.15

Item {
    property int index: 0
    readonly property int _beatLine1: editorService.linesPerBeat
    readonly property int _beatLine2: _beatLine1 % 3 ? _beatLine1 / 2 : _beatLine1 / 3
    readonly property int _beatLine3: _beatLine1 % 6 ? _beatLine1 / 4 : _beatLine1 / 6
    Rectangle {
        visible: !(index % _beatLine1)
        anchors.fill: parent
        color: "white"
        opacity: 0.25
    }
    Rectangle {
        visible: !(index % _beatLine3) && !(index % _beatLine2) && (index % _beatLine1)
        anchors.fill: parent
        color: "white"
        opacity: 0.10
    }
    Rectangle {
        visible: !(index % _beatLine3) && (index % _beatLine2) && (index % _beatLine1)
        anchors.fill: parent
        color: "white"
        opacity: 0.05
    }
}
