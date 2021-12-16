import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0
import Popups 1.0
import Pages 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

import "../"

Page {
    id: root
    property bool isRestoreFromExistWallet: false

    background: Rectangle {
        Image {
            anchors.fill: parent
            source: "qrc:/images/mainbg.png"
        }
    }

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }


    StackView {
        id: restoreStackView
        anchors.fill: parent
        anchors.topMargin: 35
        anchors.bottomMargin: 30
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        clip: true
        initialItem: verifyPasswordComponent
        property string password: ""
        property var mnemonic: []

        Component {
            id: restoringWalletComponent
            RestoringWalletComponent {
            }
        }

        Component {
            id: errorComponent
            ErrorComponent {
            }
        }

        Component {
            id: successfullComponent
            SuccessfullComponent {
            }
        }

        Component {
            id: verifyPasswordComponent

            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 48

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Making sure its you"
                    font.family: regularFont.name
                    font.pixelSize: 14
                    color: SkinColors.mainText
                    font.weight: Font.Medium
                }

                PasswordTextField {
                    id: password
                    Layout.preferredHeight: 49
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    beckgroundItem.color: SkinColors.mobileSecondaryBackground
                    beckgroundItem.radius: 8
                    beckgroundItem.border.width: 0
                    color: SkinColors.mainText
                    placeholderTextColor: SkinColors.mainText
                    placeholderText: "Password..."
                    wrapMode: Text.WordWrap
                }

                SecondaryLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Type your current password above"
                    font.family: regularFont.name
                    font.pixelSize: 12
                }

                Item {
                    Layout.fillHeight: true
                }

                MobileFooter {
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    leftButton.visible: !isRestoreFromExistWallet
                    leftButton.text: "back"
                    rightButton.text: "next"
                    onLeftButtonClicked: {
                       navigateBack();
                    }

                    onRightButtonClicked: {
                        restoreStackView.password = password.text
                        restoreStackView.push(verifyMnemonicComponent)
                    }
                }
            }
        }

        Component {
            id: verifyMnemonicComponent

            ColumnLayout {
                id: layout
                Layout.alignment: Qt.AlignHCenter
                spacing: 0

                Text {
                    Layout.bottomMargin: 27
                    Layout.alignment: Qt.AlignHCenter
                    text: "Enter your 24 word seed phrase"
                    font.family: regularFont.name
                    font.pixelSize: 14
                    color: SkinColors.mainText
                    font.weight: Font.Medium
                }

                ListView {
                    id: listView
                    Layout.preferredHeight: 441
                    Layout.fillWidth: true
                    boundsBehavior: Flickable.StopAtBounds
                    flickableDirection: Flickable.VerticalFlick
                    clip: true
                    spacing: 7
                    model: 24

                    delegate: Item {
                        id: delegateItem
                        width: parent.width
                        height: 49

                        Rectangle {
                            anchors.fill: parent
                            color: SkinColors.mobileSecondaryBackground
                            radius: 8
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 5

                            Text {
                                Layout.alignment: Qt.AlignVCenter
                                Layout.preferredWidth: 20
                                text: "%1.".arg(index + 1)
                                font.family: regularFont.name
                                font.pixelSize: 14
                                color: SkinColors.mainText
                            }

                            TextField {
                                text: restoreStackView.mnemonic[index] !== undefined ? restoreStackView.mnemonic[index] : ""
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                background: Rectangle {
                                    color: "transparent"
                                }
                                color: SkinColors.mainText
                                wrapMode: Text.WordWrap
                                onTextChanged: {
                                    restoreStackView.mnemonic[index] = text
                                }
                            }
                        }
                    }
                }

                Text {
                    id: notification
                    Layout.alignment: Qt.AlignHCenter
                    font.pixelSize: 12
                    font.family: regularFont.name
                    Layout.preferredHeight: 40
                    color: SkinColors.transactionItemSent
                }

                MobileFooter {
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.preferredHeight: 25
                    leftButton.text: "back"
                    rightButton.text: "next"
                    onLeftButtonClicked: {
                        restoreStackView.pop()
                        restoreStackView.mnemonic = []
                    }

                    onRightButtonClicked: {
                        for(var index = 0; index < 24; index++)
                        {
                            if(restoreStackView.mnemonic[index] === undefined)
                            {
                                notification.text = "Enter all words!";
                                return;
                            }
                        }

                        restoreStackView.push(reviewMnemonicComponent)
                    }
                }
            }
        }

        Component {
            id: reviewMnemonicComponent

            ColumnLayout {
                id: layout
                Layout.alignment: Qt.AlignHCenter
                spacing: 35

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Review your 24 word seed phrase"
                    font.family: regularFont.name
                    font.pixelSize: 14
                    color: SkinColors.mainText
                    font.weight: Font.Medium
                }

                SecondaryLabel {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Text.AlignHCenter
                    font.family: regularFont.name
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    text: "bellow are the words that you`re enter as your 24 word seed phrase, click confirm, to continue"
                }

                Flow {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 7

                    Repeater {
                        id: elements
                        model: restoreStackView.mnemonic

                        delegate: Item {
                            width: mnemonicWord.contentWidth + 30
                            height: 41

                            Rectangle {
                                anchors.fill: parent
                                color: SkinColors.mobileSecondaryBackground
                                opacity: 0.6
                                radius: 20
                            }

                            SecondaryLabel {
                                id: mnemonicWord
                                text: modelData !== undefined ? modelData : ""
                                anchors.centerIn: parent
                                font.family: regularFont.name
                                font.pixelSize: 12
                            }

                            PointingCursorMouseArea {
                                onClicked: {
                                    restoreStackView.push(changeMnemonic, {word : modelData, index : index})
                                }
                            }
                        }
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignCenter
                    font.family: regularFont.name
                    font.pixelSize: 12
                    color: SkinColors.mobileRestoreAdviceText
                    text: "To make a correction for words above simply click on it."
                }

                MobileFooter {
                    Layout.alignment: Qt.AlignBottom
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.preferredHeight: 25
                    leftButton.text: "back"
                    rightButton.text: "confirm"
                    onLeftButtonClicked: {
                        restoreStackView.pop();
                        restoreStackView.mnemonic = [];
                    }

                    onRightButtonClicked: {
                        ApplicationViewModel.walletViewModel.restoreWallet(restoreStackView.mnemonic.join(" "), password.text);
                        restoreStackView.mnemonic = []
                        restoreStackView.pop()
                        restoreStackView.push(restoringWalletComponent, {isRestoreFromExistWallet : root.isRestoreFromExistWallet});
                    }
                }
            }
        }

        Component {
            id: changeMnemonic

            ColumnLayout {
                id: layout
                Layout.alignment: Qt.AlignHCenter
                spacing: 50

                property string word: ""
                property int index: 0

                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 30

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Correction"
                        font.family: regularFont.name
                        font.pixelSize: 14
                        color: SkinColors.mainText
                        font.weight: Font.Medium
                    }

                    SecondaryLabel {
                        Layout.alignment: Qt.AlignHCenter
                        text: "Type your desired words below"
                        font.family: regularFont.name
                        font.pixelSize: 12
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 18

                    TextField {
                        id: textField
                        Layout.fillWidth: true
                        Layout.preferredHeight: 49
                        background: Rectangle {
                            radius: 8
                            color: SkinColors.mobileSecondaryBackground
                        }
                        color: SkinColors.mainText
                        font.family: regularFont.name
                        font.pixelSize: 14
                        text: layout.word
                    }

                    SecondaryLabel {
                        text : "Current word: %1".arg(layout.word)
                        font.family: regularFont.name
                        font.pixelSize: 12
                    }
                }

                Text {
                    id: notification
                    Layout.alignment: Qt.AlignHCenter
                    font.pixelSize: 12
                    font.family: regularFont.name
                    Layout.preferredHeight: 15
                    color: SkinColors.transactionItemSent
                }

                Item {
                    Layout.fillHeight: true
                }

                MobileFooter {
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.preferredHeight: 40
                    leftButton.text: "cancel"
                    rightButton.text: "done"
                    onLeftButtonClicked: {
                        restoreStackView.pop()
                    }

                    onRightButtonClicked: {
                        if(textField.text)
                        {
                            restoreStackView.mnemonic[index] = textField.text
                            restoreStackView.pop()
                            restoreStackView.replace(reviewMnemonicComponent)
                        }
                        else
                        {
                            notification.text = "Enter the word!"
                        }
                    }
                }
            }
        }
    }
}
