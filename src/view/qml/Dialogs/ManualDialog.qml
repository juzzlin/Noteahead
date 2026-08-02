// This file is part of Noteahead.
// Copyright (C) 2026 Jussi Lind <jussi.lind@iki.fi>
//
// Noteahead is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Noteahead is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Noteahead. If not, see <http://www.gnu.org/licenses/>.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import QtQuick.Layouts
import Noteahead 1.0
import ".."

AnimatedDialog {
    id: rootItem
    title: qsTr("User Manual")
    modal: true
    visible: false

    readonly property string _tag: "ManualDialog"
    //! Guards the lookup below: the manual is only searched once it has actually been laid out.
    property bool _contentReady: false

    footer: DialogButtonBox {
        Button {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    //! Character offset of a heading in the laid-out manual.
    //!
    //! Headings are searched in document order and each search starts where the previous heading
    //! was found, so a title that also occurs in the prose cannot capture a later section.
    function _headingPosition(anchor: string): int {
        const contents = manualService.tableOfContents;
        const plainText = contentText.getText(0, contentText.length);
        let searchFrom = 0;
        for (let i = 0; i < contents.length; i++) {
            const found = plainText.indexOf(contents[i].title, searchFrom);
            if (found < 0) {
                continue;
            }
            searchFrom = found + contents[i].title.length;
            if (contents[i].anchor === anchor) {
                return found;
            }
        }
        return -1;
    }

    function _scrollTo(anchor: string): void {
        if (!_contentReady) {
            return;
        }
        const position = _headingPosition(anchor);
        if (position < 0) {
            uiLogger.warning(_tag, `No heading found for anchor '${anchor}'`);
            return;
        }
        const target = contentText.positionToRectangle(position).y;
        const flickable = contentScrollView.contentItem;
        scrollAnimation.stop();
        scrollAnimation.to = Math.max(0, Math.min(target, flickable.contentHeight - flickable.height));
        scrollAnimation.start();
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Table of contents. Stays put while the manual scrolls, which is the point of having it
        // here rather than as a list at the top of the document.
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: Math.round(rootItem.availableWidth * 0.3)
            color: themeService.manualCodeBackgroundColor

            ListView {
                id: tableOfContents
                anchors.fill: parent
                anchors.margins: 8
                clip: true
                model: manualService.tableOfContents
                currentIndex: -1
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                delegate: ItemDelegate {
                    width: tableOfContents.width - 8
                    height: implicitHeight
                    highlighted: tableOfContents.currentIndex === index
                    onClicked: {
                        tableOfContents.currentIndex = index;
                        rootItem._scrollTo(modelData.anchor);
                    }
                    contentItem: Text {
                        // Sub-headings are indented under the section they belong to
                        leftPadding: (modelData.level - 1) * 12
                        text: modelData.title
                        color: modelData.level <= 2 ? themeService.accentColor : themeService.mainMenuTextColor
                        font.pixelSize: modelData.level <= 2 ? 16 : 14
                        font.bold: modelData.level <= 2
                        elide: Text.ElideRight
                        wrapMode: Text.NoWrap
                    }
                    Universal.theme: Universal.Dark
                    Universal.accent: themeService.accentColor
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: themeService.manualRuleColor
        }

        // ScrollView with a TextArea, rather than a TextEdit inside a hand-rolled Flickable: a
        // read-only TextEdit still tracks a cursor and forces its Flickable to scroll to it on
        // every click, which threw the position away and left gaps where it had not rendered.
        // TextArea cooperates with the ScrollView's flickable instead.
        ScrollView {
            id: contentScrollView
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            contentWidth: availableWidth // No horizontal scrolling: the text wraps instead
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            NumberAnimation {
                id: scrollAnimation
                target: contentScrollView.contentItem
                property: "contentY"
                duration: 200
                easing.type: Easing.OutCubic
            }

            TextArea {
                id: contentText
                readOnly: true
                selectByMouse: true
                textFormat: TextEdit.RichText
                wrapMode: Text.WordWrap
                color: themeService.noteColumnTextColor
                text: manualService.html
                font.pointSize: 12
                background: null
                leftPadding: 20
                rightPadding: 20
                topPadding: 20
                bottomPadding: 20
                onLinkActivated: link => {
                    // In-document cross references scroll the view; everything else is a real URL
                    if (link.startsWith("#")) {
                        rootItem._scrollTo(link.substring(1));
                    } else {
                        Qt.openUrlExternally(link);
                    }
                }
                onTextChanged: rootItem._contentReady = text.length > 0
            }
        }
    }

    Connections {
        target: manualService
        function onLoadFailed(reason) {
            uiLogger.error(rootItem._tag, `Failed to load the user manual: ${reason}`);
        }
    }

    Component.onCompleted: manualService.load(Qt.resolvedUrl("../Manual.html"))
}
