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
    readonly property color lineNumberColumnOverflowTextColor: "#444444"
    readonly property int lineNumberColumnWidth: 50
    readonly property int mainToolBarButtonSize: 32
    readonly property color mainMenuTextColor: "white"
    readonly property color mainToolBarGradientStartColor: "#303030"
    readonly property color mainToolBarGradientStopColor: "black"
    readonly property color mainToolBarSeparatorColor: "white"
    readonly property color mainToolBarTextColor: "white"
    readonly property color noteColumnBackgroundColor: "black"
    readonly property color noteColumnBorderColor: "#444444"
    readonly property color noteColumnCellBackgroundColor: "black"
    readonly property color noteColumnCellBorderColor: "#222222"
    readonly property color noteColumnTextColor: "white"
    readonly property color noteColumnTextColorEmpty: "#888888"
    readonly property color positionBarColor: "orange"
    readonly property color positionBarBorderColor: "white"
    readonly property color positionBarBorderColorEditMode: "red"
    readonly property int positionBarBorderWidth: 1
    readonly property double positionBarOpacity: 0.25
    readonly property color trackBorderColor: "#888888"
    readonly property color trackBorderFocusedColor: "orange"
    readonly property int trackBorderWidth: 1
    readonly property int trackBorderFocusedWidth: 4
    readonly property color trackHeaderBackgroundColor: "black"
    readonly property color trackHeaderBorderColor: "#222222"
    readonly property int trackHeaderHeight: 40
    readonly property int trackHeaderFontSize: 24
    readonly property color trackHeaderTextColor: "orange"
}
