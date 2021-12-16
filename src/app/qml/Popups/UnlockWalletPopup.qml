import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import "../Components"
import "../Views"

import com.xsn.utils 1.0
import com.xsn.viewmodels 1.0

ActionDialog {
    id: root
    width: 650
    height: 250

    FontLoader {
        id: regularFont
        source: "qrc:/Rubik-Regular.ttf"
    }

    TransitionsStackView {
        id: stackView
        anchors.fill: parent
        anchors.margins: 20
        clip: true
        initialItem: Item {

            ColumnLayout {
                anchors.fill: parent
                spacing: 30

                ColumnLayout {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    spacing: 30

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        font.family: regularFont.name
                        font.pixelSize: 25
                        color: SkinColors.mainText
                        text: "Unlock your wallet"
                        font.capitalization: Font.Capitalize
                    }

                    StyledTextInput {
                        id: password
                        Layout.alignment: Qt.AlignHCenter
                        Layout.fillWidth: true
                        Layout.preferredHeight: 35
                        echoMode: TextInput.Password
                        placeholderText: "Type your password"
                        onAccepted: stackView.verifyPassword(password.text)
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter

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
                            close()
                            stackView.pop()
                        }
                    }

                    ActionButton {
                        Layout.preferredHeight: 40
                        Layout.preferredWidth: 180
                        btnBackground.activeStateColor: SkinColors.mainBackground
                        btnBackground.inactiveStateColor: SkinColors.secondaryBackground
                        opacity: enabled ? 1 : 0.7
                        text: "Unlock now"
                        font.pixelSize: 14
                        enabled: password.text !== ""
                        onClicked: stackView.verifyPassword(password.text)
                    }
                }
            }
        }

        function verifyPassword(password) {
            stackView.push("qrc:/Components/PopupProgressBarComponent.qml")
            //ApplicationViewModel.lockingViewModel.unlock(password);
        }
    }
}
