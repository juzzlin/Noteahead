pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property double defaultWindowScale: 0.8
    readonly property int minWindowWidth: 1024
    readonly property int minWindowHeight: 768
    readonly property color lineNumberColumnBackgroundColor: "black"
    readonly property color lineNumberColumnBorderColor: "#444444"
    readonly property color lineNumberColumnCellBackgroundColor: "black"
    readonly property color lineNumberColumnCellBorderColor: "#222222"
    readonly property color lineNumberColumnTextColor: "orange"
    readonly property color noteColumnBackgroundColor: "black"
    readonly property color noteColumnBorderColor: "#444444"
    readonly property color noteColumnCellBackgroundColor: "black"
    readonly property color noteColumnCellBorderColor: "#222222"
    readonly property color noteColumnTextColor: "white"
    readonly property color trackBorderColor: "#888888"
    readonly property color trackHeaderBackgroundColor: "black"
    readonly property color trackHeaderBorderColor: "#222222"
    readonly property color trackHeaderTextColor: "orange"
}
