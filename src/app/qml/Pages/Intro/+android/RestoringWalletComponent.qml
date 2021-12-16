import QtQuick 2.12

import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import Components 1.0

import com.xsn.viewmodels 1.0

Item {
    property bool restored: false

    Connections {
        target : ApplicationViewModel.walletViewModel
        function onRestoredChanged(isRestored) {
            restored = isRestored;
            checkRestoringState();
        }
    }

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

        Timer {
            id: timer
            interval: 1000
            repeat: false
            running: true

            onTriggered: {
//                checkRestoringState();
            }
        }
    }

    function checkRestoringState() {
        if(restored)
        {
            restoreStackView.push(successfullComponent)
        }
        else
        {
            restoreStackView.push(errorComponent)
        }
    }
}

