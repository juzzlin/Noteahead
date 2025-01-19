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
    property double _hoverScale: 1.0
    function setImageSource(imageSource) {
        background.source = imageSource;
    }
    function setScale(scale) {
        _scale = scale;
        _hoverScale = scale * 1.1;
    }
    background: Image {
        sourceSize: Qt.size(parent.width, parent.height) // Makes the SVG look "good"
        width: parent.width * _scale
        height: parent.height * _scale
        anchors.centerIn: rootItem
        fillMode: Image.PreserveAspectFit
        opacity: rootItem.enabled ? 1.0 : 0.5
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
}
