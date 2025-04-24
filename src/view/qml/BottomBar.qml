import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import ".."

Rectangle {
    id: rootItem
    color: "black"
    property string _statusText: ""
    property var _statusQueue: []
    property bool _isDisplaying: false
    property string _tag: "BottomBar"
    function setStatusText(text: string): void {
        uiLogger.debug(_tag, `Pushing new text '${text}'`);
        _statusQueue.push(text);
        if (!_isDisplaying) {
            _displayNextText();
        }
    }
    function setPosition(position: var): void {
        contentSwitcher.setPosition(position);
    }
    function _displayNextText(): void {
        if (_statusQueue.length > 0) {
            _isDisplaying = true;
            _statusText = _statusQueue.shift();
            _fadeOutText();
            uiLogger.debug(_tag, `Displaying text '${_statusText}'`);
        } else {
            _isDisplaying = false;
        }
    }
    function _fadeOutText(): void {
        fadeAnimation.running = true;
    }
    Item {
        id: contentSwitcher
        anchors.fill: parent
        function setPosition(position: var): void {
            noteVisualizer.setPosition(position);
        }
        NoteVisualizer {
            id: noteVisualizer
            anchors.fill: parent
            visible: UiService.isPlaying()
        }
        Label {
            id: statusText
            text: _statusText
            anchors.centerIn: parent
            color: "white"
            font.pixelSize: height
            opacity: 1
            visible: !UiService.isPlaying()
            NumberAnimation {
                id: fadeAnimation
                target: statusText
                property: "opacity"
                from: 1
                to: 0
                duration: 2500
                easing.type: Easing.InQuad
                running: false
                onRunningChanged: {
                    uiLogger.debug(_tag, `onRunningChanged: ${running} ${opacity}`);
                    if (!running) {
                        _displayNextText();
                    }
                }
            }
        }
    }
    Component.onCompleted: setStatusText(qsTr("Press <b>ESC</b> to edit, <b>SPACE</b> to play, letter keys are notes"))
}
