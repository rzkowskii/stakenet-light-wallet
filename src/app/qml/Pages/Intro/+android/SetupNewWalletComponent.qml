import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0
import Popups 1.0
import Pages 1.0

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0


ColumnLayout {
    Layout.alignment: Qt.AlignHCenter
    spacing: 48

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    Text {
        Layout.topMargin: 35
        Layout.alignment: Qt.AlignHCenter
        text: "Create New Wallet"
        font.family: regularFont.name
        font.pixelSize: 14
        color: SkinColors.mainText
        font.weight: Font.Medium
    }


    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        spacing: 15

        TextField {
            id: walletName
            Layout.preferredHeight: 49
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            color: SkinColors.mainText
            wrapMode: Text.WordWrap
            placeholderTextColor: SkinColors.mainText
            placeholderText: "Wallet Name"
            background: Rectangle {
                color: SkinColors.mobileSecondaryBackground
                radius: 8
            }

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
            placeholderText: "Password"
            wrapMode: Text.WordWrap
            validated: {
                if (text.length === 0) {
                    return Enums.ValidationState.None
                }

                return Enums.ValidationState.Validated
            }
        }

        PasswordTextField {
            id: retypePassword
            Layout.preferredHeight: 49
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            beckgroundItem.color: SkinColors.mobileSecondaryBackground
            beckgroundItem.radius: 8
            beckgroundItem.border.width: 0
            color: SkinColors.mainText
            placeholderTextColor: SkinColors.mainText
            placeholderText: "Retype Password"
            wrapMode: Text.WordWrap
            validated: {
                if (text.length === 0) {
                    return Enums.ValidationState.None
                }

                return Enums.ValidationState.Validated
            }
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
        Layout.bottomMargin: 30
        Layout.leftMargin: 25
        Layout.rightMargin: 25
        leftButton.text: "back"
        rightButton.text: "next"
        onLeftButtonClicked: {
            stackView.pop();
        }

        onRightButtonClicked: {
            if(password.text ===  retypePassword.text)
            {
                if(password.text !== "")
                {
                    if(password.validated === Enums.ValidationState.Validated)
                    {
                        ApplicationViewModel.walletViewModel.setPassphrase(password.text);
                        stackView.push(creatingWalletComponent);
                    }
                }
                else
                {
                    stackView.push(walletWarningComponent);
                }
            }
            else
            {
                password.text = ""
                retypePassword.text = ""
                notification.text = "Different passwords !"
            }
        }
    }
}
