import QtQuick 2.3
import QtQuick.Controls 2.3
import QtQuick.Layouts 2.3
import ".."

Rectangle {
    id: rootItem
    color: "#222222"
    ScrollView {
        id: songScrollView
        anchors.fill: parent
        ScrollBar.vertical.policy: ScrollBar.AlwaysOff
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded
        Row {
            id: patternRow
            spacing: 2
            property real patternSize: rootItem.height
            property real contentWidth: patternSize * editorService.songLength + spacing * (editorService.songLength - 1)
            width: contentWidth
            height: rootItem.height
            Repeater {
                model: editorService.songLength
                delegate: Rectangle {
                    id: patternRect
                    width: patternRow.patternSize
                    height: patternRow.patternSize
                    color: index % 4 < 2 ? "#4A4A4A" : "#5A5A5A"
                    border.color: "#888888"
                    border.width: 1
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
                        onClicked: if (!UiService.isPlaying()) {
                            editorService.setSongPosition(index);
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
            }
        }
    }
}
