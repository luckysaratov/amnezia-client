import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    // host -> last test result text; host -> whether a test is currently running
    property var testResults: ({})
    property var testBusy: ({})

    Connections {
        target: ReconnectController
        function onHostTestStarted(host) {
            var b = root.testBusy; b[host] = true; root.testBusy = b
            var r = root.testResults; delete r[host]; root.testResults = r
        }
        function onHostTestFinished(host, result) {
            var b = root.testBusy; b[host] = false; root.testBusy = b
            var r = root.testResults; r[host] = result; root.testResults = r
        }
    }

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            HeaderTypeWithSwitcher {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Auto-reconnect")
                descriptionText: qsTr("Periodically pings the hosts below and reconnects the VPN when they are unreachable")

                showSwitcher: true
                switcher {
                    checked: ReconnectController.enabled
                }
                switcherFunction: function(checked) {
                    ReconnectController.enabled = checked
                }
            }

            //
            // Interval and failure threshold
            //
            TextFieldWithHeaderType {
                id: intervalField
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Check interval, seconds")
                textField.text: ReconnectController.intervalSeconds
                textField.validator: IntValidator { bottom: 1; top: 3600 }
                textFieldEditable: true
            }

            TextFieldWithHeaderType {
                id: thresholdField
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Failed checks in a row before reconnecting")
                textField.text: ReconnectController.failThreshold
                textField.validator: IntValidator { bottom: 1; top: 100 }
                textFieldEditable: true
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Save")

                clickedFunc: function() {
                    var seconds = parseInt(intervalField.textField.text)
                    if (isNaN(seconds) || seconds < 1) {
                        seconds = 1
                    }
                    var threshold = parseInt(thresholdField.textField.text)
                    if (isNaN(threshold) || threshold < 1) {
                        threshold = 1
                    }
                    ReconnectController.intervalSeconds = seconds
                    ReconnectController.failThreshold = threshold
                    intervalField.textField.text = ReconnectController.intervalSeconds
                    thresholdField.textField.text = ReconnectController.failThreshold
                    PageController.showNotificationMessage(qsTr("Settings saved"))
                }
            }

            //
            // Reconnect condition
            //
            Header2Type {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Reconnect when")
            }

            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                checked: ReconnectController.failMode === 0
                text: qsTr("All hosts are unreachable")
                descriptionText: qsTr("Safer: as long as one host answers, the connection is considered alive")

                onClicked: function() {
                    ReconnectController.failMode = 0
                }

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()
            }

            DividerType {}

            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                checked: ReconnectController.failMode === 1
                text: qsTr("Any host is unreachable")
                descriptionText: qsTr("More aggressive: reconnect as soon as a single host stops answering")

                onClicked: function() {
                    ReconnectController.failMode = 1
                }

                Keys.onEnterPressed: this.clicked()
                Keys.onReturnPressed: this.clicked()
            }

            DividerType {}

            SwitcherType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Ping in random order")
                descriptionText: qsTr("Shuffles the host order on every check")

                checked: ReconnectController.randomOrder
                onToggled: function() {
                    if (checked !== ReconnectController.randomOrder) {
                        ReconnectController.randomOrder = checked
                    }
                }
            }

            //
            // Stuck-connection recovery
            //
            Header2Type {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Stuck-connection recovery")
                descriptionText: qsTr("If a reconnect hangs while connecting, abort after the timeout, wait, then retry")
            }

            TextFieldWithHeaderType {
                id: stuckTimeoutField
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Connecting timeout, seconds")
                textField.text: ReconnectController.stuckTimeoutSeconds
                textField.validator: IntValidator { bottom: 10; top: 3600 }
                textFieldEditable: true
            }

            TextFieldWithHeaderType {
                id: pauseField
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Pause before retry, seconds")
                textField.text: ReconnectController.pauseSeconds
                textField.validator: IntValidator { bottom: 5; top: 3600 }
                textFieldEditable: true
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Save recovery settings")

                clickedFunc: function() {
                    var st = parseInt(stuckTimeoutField.textField.text)
                    if (isNaN(st) || st < 10) { st = 10 }
                    ReconnectController.stuckTimeoutSeconds = st
                    stuckTimeoutField.textField.text = ReconnectController.stuckTimeoutSeconds

                    var ps = parseInt(pauseField.textField.text)
                    if (isNaN(ps) || ps < 5) { ps = 5 }
                    ReconnectController.pauseSeconds = ps
                    pauseField.textField.text = ReconnectController.pauseSeconds

                    PageController.showNotificationMessage(qsTr("Recovery settings saved"))
                }
            }

            //
            // Hosts
            //
            Header2Type {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Hosts to ping")
            }

            Repeater {
                id: hostsRepeater
                model: ReconnectController.hosts

                delegate: ColumnLayout {
                    width: content.width
                    spacing: 0

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.topMargin: 12
                        Layout.bottomMargin: 12
                        spacing: 12

                        CaptionTextType {
                            Layout.fillWidth: true
                            text: modelData
                            color: AmneziaStyle.color.paleGray
                            elide: Qt.ElideRight
                        }

                        BasicButtonType {
                            implicitHeight: 32
                            Layout.preferredWidth: 84

                            defaultColor: AmneziaStyle.color.transparent
                            hoveredColor: AmneziaStyle.color.translucentWhite
                            pressedColor: AmneziaStyle.color.sheerWhite
                            textColor: AmneziaStyle.color.goldenApricot
                            borderWidth: 1

                            text: (root.testBusy[modelData] === true) ? qsTr("Testing…") : qsTr("Test")
                            enabled: root.testBusy[modelData] !== true

                            clickedFunc: function() {
                                ReconnectController.testHost(modelData)
                            }
                        }

                        ImageButtonType {
                            image: "qrc:/images/controls/trash.svg"
                            imageColor: AmneziaStyle.color.paleGray
                            implicitWidth: 32
                            implicitHeight: 32

                            onClicked: function() {
                                ReconnectController.removeHost(index)
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 12
                        Layout.preferredHeight: resultText.implicitHeight + 24
                        radius: 8
                        color: AmneziaStyle.color.onyxBlack
                        border.width: 1
                        border.color: AmneziaStyle.color.slateGray

                        visible: typeof root.testResults[modelData] === "string"

                        CaptionTextType {
                            id: resultText
                            anchors.fill: parent
                            anchors.margins: 12
                            font.family: "Courier New"
                            color: AmneziaStyle.color.mutedGray
                            wrapMode: Text.Wrap
                            textFormat: Text.PlainText
                            text: root.testResults[modelData] !== undefined ? root.testResults[modelData] : ""
                        }
                    }

                    DividerType {}
                }
            }

            TextFieldWithHeaderType {
                id: newHostField
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Add host")
                textField.placeholderText: qsTr("IP address or domain, e.g. 1.1.1.1")
                textFieldEditable: true
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Add host")

                clickedFunc: function() {
                    var host = newHostField.textField.text.trim()
                    if (host === "") {
                        return
                    }
                    ReconnectController.addHost(host)
                    newHostField.textField.text = ""
                }
            }

            //
            // Status / manual test
            //
            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 32

                text: qsTr("Status")
                descriptionText: ReconnectController.statusText
                rightImageSource: ""
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.preferredHeight: detailsText.implicitHeight + 24
                radius: 8
                color: AmneziaStyle.color.onyxBlack
                border.width: 1
                border.color: AmneziaStyle.color.slateGray

                visible: ReconnectController.lastCheckDetails.length > 0

                CaptionTextType {
                    id: detailsText
                    anchors.fill: parent
                    anchors.margins: 12
                    font.family: "Courier New"
                    color: AmneziaStyle.color.mutedGray
                    wrapMode: Text.Wrap
                    textFormat: Text.PlainText
                    text: ReconnectController.lastCheckDetails
                }
            }

            DividerType {}

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.bottomMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                text: qsTr("Check now")

                clickedFunc: function() {
                    ReconnectController.checkNow()
                }
            }

            //
            // Event log
            //
            Header2Type {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Event log")
                descriptionText: qsTr("Timestamped .txt log. Choose which events to record — applied instantly.")
            }

            SwitcherType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Connect / disconnect")
                descriptionText: qsTr("Manual connection changes (you clicked)")
                checked: ReconnectController.logConnection
                onToggled: function() {
                    if (checked !== ReconnectController.logConnection) {
                        ReconnectController.logConnection = checked
                    }
                }
            }

            DividerType {}

            SwitcherType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Auto-reconnects")
                descriptionText: qsTr("Watchdog reconnects, with the reason")
                checked: ReconnectController.logAutoReconnect
                onToggled: function() {
                    if (checked !== ReconnectController.logAutoReconnect) {
                        ReconnectController.logAutoReconnect = checked
                    }
                }
            }

            DividerType {}

            SwitcherType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Ping checks")
                descriptionText: qsTr("Result of each periodic host check")
                checked: ReconnectController.logPingCheck
                onToggled: function() {
                    if (checked !== ReconnectController.logPingCheck) {
                        ReconnectController.logPingCheck = checked
                    }
                }
            }

            DividerType {}

            SwitcherType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Host tests")
                descriptionText: qsTr("Results of the manual Test button")
                checked: ReconnectController.logHostTest
                onToggled: function() {
                    if (checked !== ReconnectController.logHostTest) {
                        ReconnectController.logHostTest = checked
                    }
                }
            }

            DividerType {}

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Open log file")
                descriptionText: ReconnectController.logFilePath
                rightImageSource: "qrc:/images/controls/chevron-right.svg"

                clickedFunction: function() {
                    ReconnectController.openLogFile()
                }
            }

            DividerType {}

            BasicButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.bottomMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                text: qsTr("Clear log")

                clickedFunc: function() {
                    ReconnectController.clearLog()
                    PageController.showNotificationMessage(qsTr("Log cleared"))
                }
            }
        }
    }
}
