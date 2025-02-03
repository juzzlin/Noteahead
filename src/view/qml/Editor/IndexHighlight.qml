import QtQuick 2.15

Rectangle {
    property int index: 0
    readonly property int _beatLine1: editorService.linesPerBeat
    readonly property int _beatLine2: _beatLine1 % 3 ? _beatLine1 / 2 : _beatLine1 / 3
    readonly property int _beatLine3: _beatLine1 % 6 ? _beatLine1 / 4 : _beatLine1 / 6
    color: "white"
    opacity: {
        if (!(index % _beatLine1))
            return 0.25;
        if (!(index % _beatLine3) && !(index % _beatLine2))
            return 0.10;
        if (!(index % _beatLine3))
            return 0.05;
        return 0;
    }
    visible: opacity > 0
}
