import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

StackViewPage {
    id: loadingWalletPage
    skipButton.visible: true
    skipButton.onClicked: stackView.push(walletWarningComponent)
    headerText: "STAKENET | DEX"
    headerSource: SkinColors.mainWalletLogo
    backButton.onClicked: {
        stackView.pop()
    }

    background: Image {
        anchors.fill: parent
        source: "qrc:/images/ComingSoonPageBg.svg"
    }

    RowLayout {
        anchors.fill: parent
        anchors.topMargin: 30
        anchors.bottomMargin: 52
        spacing: 75

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Rectangle {
            Layout.preferredWidth: 520
            Layout.preferredHeight: 579
            Layout.alignment: Qt.AlignVCenter
            color: "#99151B30"

            ColumnLayout {
                anchors.fill: parent
                spacing: 15

                anchors.bottomMargin: 60

                Image {
                    source: "qrc:/images/ic_pwdbig.svg"
                    sourceSize: Qt.size(170, 193)
                    Layout.alignment: Qt.AlignHCenter
                }

                XSNLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Create Wallet Password"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 19
                    font.bold: true
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    XSNLabel {
                        Layout.preferredHeight: 20
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15
                        color: "#7377A5"
                        text: "Password"
                    }

                    PasswordTextField {
                        id: password
                        Layout.preferredHeight: 35
                        Layout.preferredWidth: 440
                        Layout.alignment: Qt.AlignHCenter
                        color: SkinColors.mainText
                        KeyNavigation.tab: retypedPassword
                        onTextChanged: {
                            validated = validatePassword(
                                        text) ? Enums.ValidationState.Validated : Enums.ValidationState.Failed
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5

                    XSNLabel {
                        Layout.preferredHeight: 20
                        Layout.fillWidth: true
                        font.pixelSize: 15
                        Layout.alignment: Qt.AlignHCenter
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        color: "#7377A5"
                        text: "Retype password"
                    }

                    PasswordTextField {
                        id: retypedPassword
                        Layout.preferredHeight: 35
                        Layout.preferredWidth: 440
                        Layout.alignment: Qt.AlignHCenter
                        color: "white"
                        KeyNavigation.tab: password
                        onTextChanged: {
                            validated = validatePassword(text)
                                    && password.text === text ? Enums.ValidationState.Validated : Enums.ValidationState.Failed
                        }
                    }
                }

                PasswordRequirementsComponent {
                    Layout.preferredHeight: 80
                    Layout.preferredWidth: 440
                    Layout.alignment: Qt.AlignHCenter

                    passLenImgSource: password.text.length > 7 ? "qrc:/images/activeChannel.png" : "qrc:/images/closingChannel.png"
                    passNumberImgSource: /\d/.test(
                                             password.text) ? "qrc:/images/activeChannel.png" : "qrc:/images/closingChannel.png"
                    passLetterImgSource: /[A-Z]/.test(password.text)
                                         && /[a-z]/.test(
                                             password.text) ? "qrc:/images/activeChannel.png" : "qrc:/images/closingChannel.png"
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 54
                    Layout.topMargin: 20
                    spacing: 17

                    IntroButton {
                        Layout.leftMargin: 30
                        Layout.preferredWidth: 211
                        Layout.preferredHeight: 48
                        Layout.bottomMargin: 40
                        radius: 8
                        text: "Back"
                        textColor: "white"
                        borderColor: "transparent"
                        buttonColor: "#331254DD"
                        buttonGradientColor: "#331D96EC"
                        buttonHoveredColor: "#331254DD"
                        buttonGradientHoveredColor: "#331D96EC"

                        onClicked: {
                            stackView.pop()
                        }
                    }

                    IntroButton {
                        Layout.rightMargin: 30
                        Layout.preferredWidth: 211
                        Layout.preferredHeight: 48
                        Layout.bottomMargin: 40
                        radius: 8
                        text: "Continue"
                        textColor: "white"
                        borderColor: "transparent"
                        buttonColor: "#1254DD"
                        buttonGradientColor: "#1D96EC"
                        buttonHoveredColor: "#1254DD"
                        buttonGradientHoveredColor: "#1D96EC"

                        enabled: password.text === retypedPassword.text
                                 && password.validated === Enums.ValidationState.Validated

                        onClicked: {
                            stackView.push(creatingWalletComponent, {
                                               "password": password.text
                                           })
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
