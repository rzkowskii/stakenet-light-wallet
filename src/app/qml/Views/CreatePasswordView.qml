import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.utils 1.0
import com.xsn.viewmodels 1.0

Item {
    id: root
    property bool isWalletEncrypted

    FontLoader {
        id: regularFont
        source: "qrc:/Rubik-Regular.ttf"
    }

    Connections {
        target: ApplicationViewModel.walletViewModel

        function onWalletEncrypted() {
            nextStep()
            stackView.push(resultComponent, {
                               "message": "Password successfully %1!".arg(
                                              isWalletEncrypted ? "changed" : "created")
                           })
            timerStart.start()
        }

        function onEncryptWalletFailed(error) {
            stackView.push(resultComponent, {
                               "message": error})
        }
    }

    Timer {
        id: timerStart
        interval: 3000
        repeat: false
        onTriggered: {
            stepNumbersView.currentStep = 0
            stackView.clear()
            stackView.push(createPasswordComponent)
            stackLayout.currentIndex = 0
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 15

        Item {
            Layout.preferredWidth: 300
            Layout.preferredHeight: 70
            Layout.alignment: Qt.AlignHCenter

            StepNumbersView {
                id: stepNumbersView
                anchors.fill: parent
                model: ["Password", "Confirm", "Finish"]
            }
        }

        StackView {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: createPasswordComponent

            Component {
                id: createPasswordComponent

                ColumnLayout {
                    spacing: 50

                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 40

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            font.family: regularFont.name
                            font.pixelSize: 25
                            color: SkinColors.mainText
                            text: "%1 your password".arg(
                                      isWalletEncrypted ? "Change" : "Create")
                        }

                        Text {
                            lineHeight: 1.2
                            font.family: regularFont.name
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                            color: SkinColors.secondaryText
                            text: "You must remember your password - it can NOT be recovered. Your password protects your\nwallet, it should be different from your other passwords in case someone gets access to your\ncomputer."
                        }
                    }

                    StyledTextInput {
                        id: password
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 600
                        Layout.preferredHeight: 35
                        echoMode: TextInput.Password
                        placeholderText: "Type your unique password"
                    }

                    PasswordRequirementsComponent {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        Layout.leftMargin: 120
                        passLenImgSource: password.text.length
                                    > 7 ? "qrc:/images/activeChannel.png" : "qrc:/images/closingChannel.png"
                        passNumberImgSource: /\d/.test(
                                              password.text) ? "qrc:/images/activeChannel.png" : "qrc:/images/closingChannel.png"
                        passLetterImgSource: /[A-Z]/.test(
                                              password.text) && /[a-z]/.test(password.text) ? "qrc:/images/activeChannel.png" : "qrc:/images/closingChannel.png"
                    }

                    ActionButton {
                        Layout.preferredHeight: 40
                        Layout.preferredWidth: 180
                        btnBackground.activeStateColor: SkinColors.mainBackground
                        btnBackground.inactiveStateColor: SkinColors.secondaryBackground
                        opacity: enabled ? 1 : 0.7
                        Layout.alignment: Qt.AlignHCenter
                        text: "Next"
                        font.pixelSize: 14
                        enabled: validatePassword(password.text)
                        onClicked: {
                            nextStep()
                            stackView.push(memorizePasswordComponent, {
                                               "password": password.text
                                           })
                        }
                    }
                }
            }

            Component {
                id: memorizePasswordComponent

                ColumnLayout {
                    spacing: 50
                    property string password: ""

                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter
                        spacing: 40

                        Text {
                            Layout.alignment: Qt.AlignHCenter
                            font.family: regularFont.name
                            font.pixelSize: 25
                            color: SkinColors.mainText
                            text: "Memorize your password"
                        }

                        Text {
                            lineHeight: 1.2
                            font.family: regularFont.name
                            font.pixelSize: 16
                            horizontalAlignment: Text.AlignHCenter
                            color: SkinColors.secondaryText
                            text: "It is time to practice your password. Remember, if you forget your password there is no way to\nget it back! Make sure you have got your password memorized and it is different than any of\nyour other passwords."
                        }
                    }

                    StyledTextInput {
                        id: passwordMemorize
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredWidth: 600
                        Layout.preferredHeight: 35
                        echoMode: TextInput.Password
                        placeholderText: "Type your password again"
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillHeight: true
                        Layout.preferredHeight: 45
                        spacing: 20

                        SecondaryButton {
                            Layout.preferredHeight: 40
                            Layout.preferredWidth: 180
                            font.pixelSize: 12
                            borderColor: SkinColors.popupFieldBorder
                            hoveredBorderColor: SkinColors.headerText
                            activeStateColor: SkinColors.popupFieldBorder
                            text: "Cancel"
                            font.capitalization: Font.AllUppercase

                            onClicked: {
                                previousStep()
                                stackView.pop()
                            }
                        }

                        ActionButton {
                            Layout.preferredHeight: 40
                            Layout.preferredWidth: 180
                            btnBackground.activeStateColor: SkinColors.mainBackground
                            btnBackground.inactiveStateColor: SkinColors.secondaryBackground
                            Layout.alignment: Qt.AlignHCenter
                            text: "Next"
                            font.pixelSize: 14
                            enabled: password === passwordMemorize.text
                            opacity: enabled ? 1 : 0.7
                            onClicked: {
                                nextStep();
                                ApplicationViewModel.walletViewModel.encryptWallet(
                                            password);
                                stackView.push(progressComponent);
                               }
                        }
                    }
                }
            }

            Component {
                id: progressComponent

                Item {

                    PopupProgressBarComponent {
                        anchors.centerIn: parent
                        width: 500
                    }
                }
            }

            Component {
                id: resultComponent

                Item {
                    property string message: ""

                    Text {
                        anchors.centerIn: parent
                        font.family: regularFont.name
                        font.pixelSize: 25
                        color: SkinColors.mainText
                        text: message
                    }
                }
            }
        }
    }

    function nextStep() {
        stepNumbersView.currentStep += 1
    }
    function previousStep() {
        stepNumbersView.currentStep -= 1
    }

    function validatePassword(password) {
        if (password.length < 8) {
            return false
        } else {
            var hasNumber = /\d/
            var hasSmallLetter = /[a-z]/
            var hasUpperLetter = /[A-Z]/

            if (!hasNumber.test(password) || !hasSmallLetter.test(password)
                    || !hasUpperLetter.test(password)) {
                return false
            } else {
                return true
            }
        }
    }
}
