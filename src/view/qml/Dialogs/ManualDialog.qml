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
import "../Components"

AnimatedDialog {
    id: rootItem
    title: qsTr("User Manual")
    modal: true
    visible: false

    readonly property string _tag: "ManualDialog"

    footer: DialogButtonBox {
        AppButton {
            text: qsTr("Ok")
            implicitWidth: Constants.defaultButtonWidth
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
    }

    //! Scrolls to the section carrying the given anchor.
    //!
    //! Every section is an item of its own, so this is the item's position rather than a guess at
    //! where a heading's text ended up inside one enormous document.
    function _scrollTo(anchor: string): void {
        const sections = manualService.sections;
        for (let i = 0; i < sections.length; i++) {
            if (sections[i].anchor === anchor) {
                const item = sectionRepeater.itemAt(i);
                if (!item) {
                    return;
                }
                // The item's position is inside the column, which sits at the top margin
                const target = sectionColumn.y + item.y;
                scrollAnimation.stop();
                scrollAnimation.to = Math.max(0, Math.min(target, contentFlickable.contentHeight - contentFlickable.height));
                scrollAnimation.start();
                return;
            }
        }
        uiLogger.warning(_tag, `No section found for anchor '${anchor}'`);
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

        // One Text per section rather than the whole manual in a single TextArea. A text editor
        // holding a document this size renders it lazily against the flickable's viewport and keeps
        // a cursor it insists on scrolling to, which is what left blank space above the text and
        // threw the scroll position away. A section is an ordinary item: it is laid out once, its
        // position is known, and there is no cursor to chase.
        Flickable {
            id: contentFlickable
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: width // No horizontal scrolling: the text wraps instead
            contentHeight: sectionColumn.height + 2 * sectionColumn.y

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            NumberAnimation {
                id: scrollAnimation
                target: contentFlickable
                property: "contentY"
                duration: 200
                easing.type: Easing.OutCubic
            }

            Column {
                id: sectionColumn
                x: 20
                y: 20
                width: contentFlickable.width - 40

                Repeater {
                    id: sectionRepeater
                    model: manualService.sections

                    // Each section is a document of its own, and a document drops the top margin of
                    // its first block, so the air above a heading has to be real space between the
                    // items rather than a margin in the stylesheet. A major section gets more of it
                    // than a sub-heading, which is what makes the hierarchy readable while
                    // scrolling. The first one needs none: the column already starts below the top.
                    delegate: Item {
                        required property var modelData
                        required property int index
                        readonly property int topMargin: index === 0 ? 0 : (modelData.level <= 2 ? 28 : 16)

                        width: sectionColumn.width
                        height: sectionText.height + topMargin

                        Text {
                            id: sectionText
                            y: parent.topMargin
                            width: parent.width
                            textFormat: Text.RichText
                            wrapMode: Text.WordWrap
                            color: themeService.noteColumnTextColor
                            font.pointSize: 12
                            text: parent.modelData.html
                            onLinkActivated: link => {
                                // In-document cross references scroll the view; everything else is a real URL
                                if (link.startsWith("#")) {
                                    rootItem._scrollTo(link.substring(1));
                                } else {
                                    Qt.openUrlExternally(link);
                                }
                            }
                        }
                    }
                }
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
