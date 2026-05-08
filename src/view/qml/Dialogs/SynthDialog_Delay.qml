import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Universal 2.15
import Noteahead 1.0
import "../Components"

ColumnLayout {
    Universal.theme: Universal.Dark
    Universal.accent: themeService.accentColor
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    Label {
        text: qsTr("Delay Effect")
        font.bold: true
        font.pixelSize: 16
        color: themeService.accentColor
        Layout.alignment: Qt.AlignLeft
        Layout.topMargin: 10
    }

    RowLayout {
        ComboBox {
            model: ["Stereo", "Mono", "PingPong", "Tape"]
            currentIndex: synthController.delayType
            onActivated: i => synthController.delayType = i
            Layout.fillWidth: true
        }
    }
    RowLayout {
        CheckBox {
            text: qsTr("Sync")
            checked: synthController.delaySync
            onToggled: synthController.delaySync = checked
        }
        StackLayout {
            currentIndex: synthController.delaySync ? 0 : 1
            Layout.fillWidth: true
            Layout.preferredHeight: delayTimeKnob.implicitHeight
            SyncSlider {
                label: qsTr("Time")
                value: synthController.delaySyncDivision
                onMoved: v => synthController.delaySyncDivision = v
                Layout.fillWidth: true
            }
            TimeKnob {
                id: delayTimeKnob
                label: qsTr("Time")
                from: 1
                to: 10000
                suffix: "ms"
                value: synthController.delayTime
                onMoved: v => synthController.delayTime = v
                Layout.fillWidth: true
            }
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Knob {
            label: qsTr("Feedback")
            value: synthController.delayFeedback
            onMoved: v => synthController.delayFeedback = v
            Layout.fillWidth: true
        }
        FilterKnob {
            label: qsTr("LPF")
            controller: synthController
            value: synthController.delayFeedbackLpf
            onMoved: v => synthController.delayFeedbackLpf = v
            Layout.fillWidth: true
        }
        FilterKnob {
            label: qsTr("HPF")
            controller: synthController
            value: synthController.delayFeedbackHpf
            isHpf: true
            onMoved: v => synthController.delayFeedbackHpf = v
            Layout.fillWidth: true
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Knob {
            label: qsTr("Depth")
            value: synthController.delayDepth
            onMoved: v => synthController.delayDepth = v
            Layout.fillWidth: true
        }
        Knob {
            label: qsTr("Mix")
            value: synthController.delayMix
            onMoved: v => synthController.delayMix = v
            Layout.fillWidth: true
        }
    }
}
