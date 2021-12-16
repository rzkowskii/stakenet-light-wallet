import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"
import "../Views"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Page {
    id: root
    property bool notificationErrorVisible: false
    property string notificationErrorText: ""
    property LockingViewModel lockingViewModel: ApplicationViewModel.lockingViewModel
    property WalletViewModel walletViewModel: ApplicationViewModel.walletViewModel

    background: Image {
        anchors.fill: parent
        source: "qrc:/images/ComingSoonPageBg.svg"
    }

    Connections {
        target: walletViewModel
        function onWalletLoaded() {
            replaceView(loadingPage, {
                            "appLoaded": true
                        })
        }
        function onLoadingWalletFailed(error) {
            notificationErrorText = error
            notificationErrorVisible = true
            stackView.pop()
        }
    }

    FontLoader {
        id: lightFont
        source: "qrc:/Rubik-Light.ttf"
    }

    Item {
        height: 650
        width: 800
        anchors.centerIn: parent

        TransitionsStackView {
            id: stackView
            anchors.fill: parent
            anchors.leftMargin: 60
            anchors.rightMargin: 60
            anchors.topMargin: 60
            anchors.bottomMargin: 100
            clip: true
            initialItem: initialComponent

            Component {
                id: initialComponent

                ColumnLayout {
                    id: layout
                    property alias passwordText: password.text
                    spacing: 50

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 90
                        Layout.alignment: Qt.AlignHCenter

                        Row {
                            anchors.centerIn: parent

                            Image {
                                source: SkinColors.walletSideMenuLogo
                                sourceSize: Qt.size(90, 90)
                            }

                            XSNLabel {
                                text: "STAKENET | DEX"
                                font.pixelSize: 24
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 55
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 15

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Welcome back"
                            font.capitalization: Font.Capitalize
                            color: SkinColors.mainText
                            font.pixelSize: 24
                            font.family: lightFont.name
                        }

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            text: "Stakenet desktop release %1".arg(appVersion)
                            color: SkinColors.secondaryText
                            font.pixelSize: 16
                            font.capitalization: Font.Capitalize
                            font.family: lightFont.name
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 10

                        StyledTextInput {
                            id: password
                            Layout.alignment: Qt.AlignHCenter
                            Layout.preferredWidth: 500
                            Layout.preferredHeight: 35
                            echoMode: TextInput.Password
                            placeholderText: "Type your password"
                            componentFocus: true
                            onTextChanged: {
                                if (notificationErrorVisible) {
                                    notificationErrorVisible = false
                                }
                            }
                            onAccepted: {
                                if (passwordText !== "") {
                                    unlockWallet(passwordText)
                                    password.text = ""
                                }
                            }
                        }

                        Text {
                            id: notification
                            Layout.alignment: Qt.AlignHCenter
                            text: "Click the button or press ENTER to unlock."
                            color: SkinColors.secondaryText
                            font.pixelSize: 12
                            font.family: lightFont.name
                            visible: passwordText !== ""
                                     && !notificationErrorVisible
                        }

                        Text {
                            id: notificationError
                            Layout.alignment: Qt.AlignHCenter
                            color: SkinColors.transactionItemSent
                            font.pixelSize: 12
                            font.family: lightFont.name
                            visible: notificationErrorVisible
                            text: notificationErrorText
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }

                    IntroButton {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredHeight: 42
                        Layout.preferredWidth: 160
                        font.pixelSize: 12
                        radius: 8
                        text: "Unlock"
                        buttonGradientColor: SkinColors.introBtnGradientColor
                        buttonColor: SkinColors.buttonBorderColor
                        enabled: passwordText != ""
                        onClicked: {
                            unlockWallet(passwordText)
                            password.text = ""
                        }
                    }
                }
            }
        }
    }

    function unlockWallet(password) {
        stackView.push("qrc:/Components/PopupProgressBarComponent.qml")
        walletViewModel.loadApp(password)
    }
}
