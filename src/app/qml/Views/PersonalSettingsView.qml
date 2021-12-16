import QtQuick 2.6
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls.Styles 1.4

import "../Components"
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import Qt.labs.settings 1.0

Flickable {
    id: flickable
    contentHeight: height < contentFlickableHeight ? contentFlickableHeight : height
    boundsBehavior: Flickable.StopAtBounds
    clip: true

    ScrollBar.vertical: ScrollBar {
        anchors.top: flickable.top
        anchors.right: flickable.right
        anchors.bottom: flickable.bottom
        size: flickable.height / flickable.contentHeight
    }
    property int contentFlickableHeight: stackLayout.currentIndex === 0 ? 400 : 800
    property bool isWalletEncrypted: ApplicationViewModel.lockingViewModel.isEncrypted
    property bool createPassVisibility: !isWalletEncrypted || stackLayout.currentIndex === 1

    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        StackLayout {
            id: stackLayout
            Layout.fillWidth: true
            Layout.maximumHeight: stackLayout.currentIndex === 0 ? 105 : 580
            currentIndex: 0
            visible: createPassVisibility

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 90
                spacing: 15

                XSNLabel {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 50
                    text: "Create password"
                    font.pixelSize: 25
                }

                ActionButton {
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 180
                    btnBackground.activeStateColor: SkinColors.mainBackground
                    btnBackground.inactiveStateColor: SkinColors.secondaryBackground
                    Layout.alignment: Qt.AlignHCenter
                    text: "Create"
                    font.pixelSize: 14
                    onClicked: stackLayout.currentIndex = 1
                }
            }

            CreatePasswordView {
                Layout.fillWidth: true
                Layout.preferredHeight: 580
                isWalletEncrypted: flickable.isWalletEncrypted
            }
        }

        Rectangle {
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: SkinColors.secondaryText
            visible: createPassVisibility
        }

        XSNLabel {
            id: wordsSeedLbl
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            text: qsTr("Your 24 word seed phrase")
            font.pixelSize: 25
        }

        ColumnLayout {
            id: wordsSeedItem
            Layout.fillWidth: true
            visible: false
            spacing: 10

            WarningComponent {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                message: "For your safety, do not share this seed phrase with anyone and do not keep a copy of this on a device that has an online connection."
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                color: SkinColors.settingPagePersonalTabTextArea
                radius: 4

                TextField {
                    id: textArea
                    anchors.fill: parent
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    background: Rectangle {
                        color: "transparent"
                    }
                    color: SkinColors.mainText
                    wrapMode: Text.WordWrap
                    readOnly: true
                    font.pointSize: 14
                    selectByMouse: true
                    focus: true
                    Component.onCompleted: ApplicationViewModel.walletViewModel.requestMnemonic()

                    Connections {
                        target: ApplicationViewModel.walletViewModel
                        function onRequestMnemonicFinished(mnemonic) {
                            textArea.text = mnemonic
                        }
                    }

                    onSelectedTextChanged: {
                        styledMouseArea.selectStart = textArea.selectionStart
                        styledMouseArea.selectEnd = textArea.selectionEnd
                    }

                    onCursorPositionChanged: {
                        styledMouseArea.selectStart = 0
                        styledMouseArea.selectEnd = 0
                    }

                    StyledMouseArea {
                        id: styledMouseArea
                        anchors.fill: parent
                        onClicked: {
                            curPos = textArea.cursorPosition
                            contextMenu.x = mouse.x
                            contextMenu.y = mouse.y
                            contextMenu.open()
                            textArea.cursorPosition = curPos
                            textArea.select(selectStart, selectEnd)
                        }
                    }

                    Menu {
                        id: contextMenu

                        Action {
                            text: qsTr("Copy")
                            enabled: textArea.selectedText
                            onTriggered: textArea.copy()
                        }

                        delegate: MenuItemDelegate {
                            id: menuItem
                        }

                        background: Rectangle {
                            implicitWidth: 180
                            implicitHeight: 30
                            color: SkinColors.menuBackground
                            border.color: SkinColors.popupFieldBorder
                        }
                    }
                }

                Item {
                    width: 40
                    height: 40
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 5
                    anchors.bottomMargin: 5

                    Image {
                        anchors.fill: parent
                        anchors.margins: 10
                        anchors.centerIn: parent
                        source: "qrc:/images/COPY ICONS.png"
                    }

                    PointingCursorMouseArea {
                        id: copyMouseArea
                        anchors.fill: parent
                        onClicked: {
                            Clipboard.setText(textArea.text)
                            showBubblePopup("Copied")
                        }
                    }
                }
            }
        }

        ActionButton {
            id: showSeedBtn
            Layout.preferredHeight: 40
            Layout.preferredWidth: 180
            btnBackground.activeStateColor: SkinColors.mainBackground
            btnBackground.inactiveStateColor: SkinColors.secondaryBackground
            Layout.alignment: Qt.AlignHCenter
            font.pixelSize: 14
            text: wordsSeedItem.visible ? qsTr("Hide seed phrase") : qsTr(
                                              "Show seed phrase")
            onClicked: {
                if (!wordsSeedItem.visible) {
                    wordsSeedItem.visible = true
                } else {
                    wordsSeedItem.visible = false
                }
            }
        }

        Rectangle {
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: SkinColors.secondaryText
            visible: false
        }

        XSNLabel {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            text: qsTr("Clear orderbook webview cache")
            font.pixelSize: 25
            visible: false
        }

        ActionButton {
            Layout.preferredHeight: 40
            Layout.preferredWidth: 180
            btnBackground.activeStateColor: SkinColors.mainBackground
            btnBackground.inactiveStateColor: SkinColors.secondaryBackground
            Layout.alignment: Qt.AlignHCenter
            text: "Clear"
            font.pixelSize: 14
            visible: false
            //        onClicked:
        }

        Rectangle {
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: SkinColors.secondaryText
            visible: false
        }

        XSNLabel {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            text: qsTr("Reset wallet")
            font.pixelSize: 25
            visible: false
        }

        ActionButton {
            Layout.preferredHeight: 40
            Layout.preferredWidth: 180
            btnBackground.activeStateColor: SkinColors.mainBackground
            btnBackground.inactiveStateColor: SkinColors.secondaryBackground
            Layout.alignment: Qt.AlignHCenter
            text: "Reset"
            font.pixelSize: 14
            visible: false
            onClicked: openConfirmResetNotificationDialog()
        }

        Rectangle {
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            color: SkinColors.secondaryText
            visible: false
        }

        XSNLabel {
            visible: false
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            text: qsTr("The Onion Router (TOR)")
            font.pixelSize: 25
        }

        ColumnLayout {
            visible: false
            spacing: 10

            RowLayout {
                spacing: 7

                XSNLabel {
                    text: "Status:"
                    Layout.alignment: Qt.AlignVCenter
                    font.pixelSize: 16
                }

                XSNLabel {
                    id: status
                    text: TorManager.torActiveState ? "Active" : "Inactive"
                    color: TorManager.torActiveState ? SkinColors.transactionItemReceived : SkinColors.transactionItemSent
                    Layout.alignment: Qt.AlignVCenter
                    font.pixelSize: 16
                }
            }

            RowLayout {
                spacing: 5
                Layout.fillWidth: true
                Layout.preferredHeight: 40

                RoundCheckbox {
                    id: checkbox
                    checked: TorManager.torActiveState

                    PointingCursorMouseArea {
                        onClicked: TorManager.changeTorState()

                        onEntered: {
                            checkbox.scale = scale * 1.1
                        }
                        onExited: {
                            checkbox.scale = scale
                        }
                    }
                }

                XSNLabel {
                    text: "Use TOR for wallet"
                    Layout.alignment: Qt.AlignVCenter
                    font.pixelSize: 16
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
