pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property double defaultWindowScale: 0.8
    readonly property int minWindowWidth: 1024
    readonly property int minWindowHeight: 768
    readonly property color lineNumberColumnBackgroundColor: "black"
    readonly property color lineNumberColumnBorderColor: "#222222"
    readonly property color lineNumberColumnTextColor: "orange"
    readonly property color noteColumnBackgroundColor: "black"
    readonly property color noteColumnBorderColor: "#222222"
    readonly property color noteColumnTextColor: "white"
    readonly property color trackHeaderBackgroundColor: "black"
    readonly property color trackHeaderBorderColor: "#222222"
    readonly property color trackHeaderTextColor: "orange"
}
