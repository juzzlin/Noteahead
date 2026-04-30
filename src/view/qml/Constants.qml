pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property int columnHeaderHeight: 40
    readonly property double defaultWindowScale: 0.8
    readonly property double defaultDialogScale: 0.7
    readonly property int minWindowWidth: 1024
    readonly property int minWindowHeight: 768

    readonly property int lineNumberColumnWidth: 50
    readonly property int mainToolBarButtonSize: 32
    readonly property int toolTipDelay: 1000
    readonly property int toolTipTimeout: 5000
    readonly property int trackBorderWidth: 1
    readonly property int trackBorderFocusedWidth: 2
    readonly property int trackHeaderHeight: 40

    readonly property int transposeMin: -48
    readonly property int transposeMax: 48

    readonly property int positionBarBorderWidth: 1
    readonly property double positionBarOpacity: 0.25

    readonly property int maxEventDelay: 10000
}
