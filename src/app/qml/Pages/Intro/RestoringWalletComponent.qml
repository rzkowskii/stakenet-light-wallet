import QtQuick 2.12

import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0
import com.xsn.viewmodels 1.0

StackViewPage {
    id: loadingWalletPage
    backButton.visible: false
    skipButton.visible: false
    headerText: "STAKENET | DEX"
    headerSource: SkinColors.mainWalletLogo

    background: Image {
        anchors.fill: parent
        source: "qrc:/images/ComingSoonPageBg.svg"
    }

    Connections {
        target: ApplicationViewModel.walletViewModel

        function onRestoredChanged(isRestored) {
            _internal.finished = true
            _internal.restored = isRestored
            handleRestoreState()
        }
    }

    Item {
        anchors.fill: parent

        Row {
            id: label
            anchors.centerIn: parent
            spacing: 10

            XSNLabel {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Restoring wallet")
                font.pixelSize: 24
            }

            BusyIndicator {
                running: true
            }
        }
    }

    Timer {
        id: timer
        interval: 1000
        repeat: false
        running: true
        onTriggered: handleRestoreState()
    }

    QtObject {
        id: _internal
        property bool finished: false
        property bool restored: false
    }

    function handleRestoreState() {
        if (timer.running || !_internal.finished) {
            return
        }

        if (_internal.restored) {
            stackView.finishText = "Your wallet was successfully restored!"
            stackView.push(walletCreatedComponent, {
                               "isOpenRescanNotification": true,
                               "mainHeaderLbl": "Wallet Restored",
                               "secondaryHeaderText": "Your wallet has been successfully restored. Thank you for using Stakenet DEX!"
                           })
        } else {
            stackView.push(errorComponent)
        }
    }
}
