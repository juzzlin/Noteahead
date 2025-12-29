import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Item {
    id: rootItem
    opacity: 0
    visible: false
    property alias value: control.value
    z: 100
    function reset(): void {
        fadeOutAnimation.stop();
        rootItem.opacity = 1;
        rootItem.visible = true;
        control.value = 0;
    }
    function fadeOut(): void {
        fadeOutAnimation.start();
    }
    Rectangle {
        id: shadowRect
        anchors.fill: parent
        anchors.margins: -20
        color: Constants.progressBarBackgroundColor
        radius: 10
        opacity: 0.9
    }
    ProgressBar {
        id: control
        anchors.fill: parent
        value: 0
        from: 0
        to: 1
        background: Rectangle {
            implicitWidth: 200
            implicitHeight: 6
            color: Constants.progressBarBackgroundColor
            radius: 3
        }
        contentItem: Item {
            implicitWidth: 200
            implicitHeight: 6
            Rectangle {
                width: control.visualPosition * parent.width
                height: parent.height
                radius: 3
                color: Constants.progressBarForegroundColor
            }
        }
    }
    NumberAnimation {
        id: fadeOutAnimation
        target: rootItem
        property: "opacity"
        to: 0
        duration: 500
        easing.type: Easing.InCubic
        onStopped: rootItem.visible = false
    }
}
