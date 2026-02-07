import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 2.15
import ".."

Rectangle {
    id: rootItem
    color: "#222222"
    RowLayout {
        anchors.fill: parent
        spacing: 2
        ListView {
            id: songListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: ListView.Horizontal
            spacing: 2
            model: editorService.songLength
            clip: true
            currentIndex: editorService.songPosition
            onCurrentIndexChanged: {
                if (songListView.currentIndex >= 0) {
                    songListView.positionViewAtIndex(songListView.currentIndex, ListView.Contain);
                }
            }
            delegate: Rectangle {
                id: patternRect
                width: songListView.height
                height: songListView.height
                color: index % 4 < 2 ? "#4A4A4A" : "#5A5A5A"
                border.color: "#888888"
                border.width: 1
                ToolTip.visible: patternMouseArea.containsMouse
                ToolTip.text: editorService.patternName(Number(textField.text))
                ToolTip.delay: Constants.toolTipDelay
                ToolTip.timeout: Constants.toolTipTimeout
                Text {
                    id: textField
                    anchors.centerIn: parent
                    text: editorService.patternAtSongPosition(index)
                    color: editorService.songPosition === index ? "orange" : "white"
                    font.pixelSize: 14
                    font.bold: true
                }
                MouseArea {
                    id: patternMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        UiService.jumpToSongPosition(index);
                    }
                    states: [
                        State {
                            when: patternMouseArea.containsMouse
                            PropertyChanges {
                                target: patternRect
                                color: "#777777"
                            }
                        }
                    ]
                }
                Connections {
                    target: editorService
                    function onPatternAtCurrentSongPositionChanged() {
                        if (editorService.songPosition === index) {
                            textField.text = editorService.patternAtSongPosition(index);
                        }
                    }
                }
            }
            ScrollBar.horizontal: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
        }
        Rectangle {
            Layout.preferredWidth: rootItem.height
            Layout.fillHeight: true
            color: "transparent"
            Image {
                id: addPatternIcon
                anchors.fill: parent
                source: "../Graphics/add_box.svg"
                fillMode: Image.PreserveAspectFit
                opacity: addPatternMouseArea.containsMouse ? 1.0 : 0.7
            }
            MouseArea {
                id: addPatternMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: editorService.setSongLength(editorService.songLength + 1)
            }
            ToolTip.visible: addPatternMouseArea.containsMouse
            ToolTip.text: qsTr("Increase song length")
            ToolTip.delay: Constants.toolTipDelay
            ToolTip.timeout: Constants.toolTipTimeout
        }
    }
}