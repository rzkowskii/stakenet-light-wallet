import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.1
import QtGraphicalEffects 1.0

import Components 1.0
import Popups 1.0
import Pages 1.0

import com.xsn.viewmodels 1.0

Page {
    id: root
    property var activePopup: undefined

    background: Image {
        anchors.fill: parent
        source: isMobile ? "qrc:/images/mainbg.png" : ""
    }

    FastBlur {
        anchors.fill: parent
        z: 1000
        source: stackView
        radius: 32
        visible: activePopup !== undefined && activePopup.visible
    }

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: mainIntroPageComponent
        property string finishText: ""
    }

    Component {
        id: mainIntroPageComponent
        MainIntroPageComponent {}
    }

    Component {
        id: getStartedPageComponent
        GetStartedPageComponent {}
    }

    Component {
        id: walletCreatedComponent
        WalletCreatedComponent {}
    }

    Component {
        id: restoreFromBackupComponent
        RestoreFromBackupComponent {}
    }

    Component {
        id: creatingWalletComponent
        CreatingWalletComponent {}
    }

    Component {
        id: inputPassword

        StackViewPage {
            skipButton.visible: false
            backButton.onClicked: {
                stackView.pop()
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 10

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Label {
                        anchors.centerIn: parent
                        text: qsTr("Enter password:")
                    }
                }

                TextField {
                    Layout.preferredHeight: 50
                    Layout.preferredWidth: 400
                    Layout.alignment: Qt.AlignHCenter
                    background: Rectangle {
                        border.width: 1
                    }
                }

                Button {
                    text: qsTr("OK")
                    Layout.preferredWidth: 200
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }

    Component {
        id: successfullComponent
        SuccessfullComponent {}
    }

    Component {
        id: backupLaterComponent
        BackupLaterComponent {}
    }

    Component {
        id: backupWalletComponent
        BackupWalletComponent {}
    }

    Component {
        id: verifyMnemonicComponent
        VerifyMnemonicComponent {}
    }

    Component {
        id: setupNewWalletComponent
        SetupNewWalletComponent {}
    }

    Component {
        id: walletWarningComponent
        WalletWarningComponent {}
    }

    Component {
        id: restoringWalletComponent
        RestoringWalletComponent {}
    }

    Component {
        id: errorComponent
        ErrorComponent {}
    }

    MessageDialog {
        id: messageDialog
        title: "Wallet"
        onAccepted: messageDialog.close()
    }
    Component {
        id: notificationPopup
        NotificationPopup {}
    }

    function openNotificationDialog(title, explanation) {
        activePopup = notificationPopup.createObject(root, {
                                                         "headerName": title,
                                                         "explanation": explanation
                                                     })
        activePopup.open()
    }
}
