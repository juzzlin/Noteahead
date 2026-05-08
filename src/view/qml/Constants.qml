pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property int columnHeaderHeight: 40
    readonly property double defaultWindowScale: 0.8
    readonly property double defaultDialogScale: 0.7
    readonly property double largeDialogScale: 0.85
    readonly property int minWindowWidth: 1024
    readonly property int minWindowHeight: 768

    readonly property int lineNumberColumnWidth: 50
    readonly property int mainToolBarButtonSize: 32
    readonly property int defaultButtonWidth: 100
    readonly property int toolTipDelay: 1000
    readonly property int toolTipTimeout: 5000
    readonly property int trackBorderWidth: 1
    readonly property int trackBorderFocusedWidth: 2
    readonly property int trackHeaderHeight: 40

    readonly property int transposeMin: -48
    readonly property int transposeMax: 48

    readonly property int positionBarBorderWidth: 1
    readonly property double positionBarOpacity: 0.25

    readonly property real maxEventDelay: 10000

    readonly property real uiInternalScaling: 1000.0

    readonly property var syncLabels: ["1/1", "3/4", "1/2", "3/8", "1/3", "1/4", "3/16", "1/6", "1/8", "3/32", "1/12", "1/16", "3/64", "1/24", "1/32", "1/64"]
    readonly property var syncDivisions: [1.0, 0.75, 0.5, 0.375, 1.0/3.0, 0.25, 0.1875, 1.0/6.0, 0.125, 0.09375, 1.0/12.0, 0.0625, 0.046875, 1.0/24.0, 0.03125, 0.015625]
}
