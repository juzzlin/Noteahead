import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts 2.15
import ".."

Button {
    id: rootItem
    width: Constants.mainToolBarButtonSize
    height: width
    property double _scale: 1.0
    property bool _toggled: false
    property double _hoverScale: 1.0
    property string toolTipText
    property string _toggleColor: "red"
    signal leftClicked
    signal rightClicked
    function setImageSource(imageSource: string): void {
        backgroundImage.source = imageSource;
    }
    function setScale(scale: double): void {
        _scale = scale;
        _hoverScale = scale * 1.1;
    }
    function isToggled(): bool { // Note!!: toggled() already in use in Qt 6.8
        return _toggled;
    }
    function setToggled(toggled: bool): void {
        _toggled = toggled;
    }
    function setToggleColor(toggleColor: color): void {
        _toggleColor = toggleColor;
    }
    background: Rectangle {
        id: borderRect
        color: "transparent"
        border.color: rootItem._toggled ? _toggleColor : "transparent"
        border.width: rootItem._toggled ? 4 : 0
        radius: 4
        opacity: rootItem.enabled ? 1.0 : 0.5
        Image {
            id: backgroundImage
            sourceSize: Qt.size(parent.width, parent.height)
            width: Math.floor(parent.width * rootItem._scale)
            height: Math.floor(parent.height * rootItem._scale)
            anchors.centerIn: parent
            fillMode: Image.PreserveAspectFit
            opacity: rootItem.enabled ? 1.0 : 0.5
        }
    }
    states: State {
        name: "hovered"
        when: rootItem.hovered
        PropertyChanges {
            target: rootItem
            _scale: rootItem._hoverScale
        }
    }
    transitions: Transition {
        from: ""
        to: "hovered"
        NumberAnimation {
            properties: "_scale"
            duration: 50
        }
    }
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onClicked: mouse => {
            if (mouse.button === Qt.LeftButton) {
                rootItem.leftClicked();
                rootItem.clicked();
            } else if (mouse.button === Qt.RightButton) {
                rootItem.rightClicked();
            }
        }
    }
    ToolTip.delay: Constants.toolTipDelay
    ToolTip.timeout: Constants.toolTipTimeout
    ToolTip.visible: hovered
    ToolTip.text: toolTipText
}
