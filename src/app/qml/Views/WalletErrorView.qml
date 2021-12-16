import QtQuick 2.12
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Rectangle {
    id: root
    visible: false
    FontLoader { id: lightFont; source: "qrc:/Rubik-Light.ttf" }
    color: SkinColors.errorViewBackground

    Connections {
        target: AppUpdater
        function onDownloadFailed(error) {
            showError(error);
        }
        function onUpdaterStateChanged() {
            root.visible = false
        }
    }

    Connections {
        target: ApplicationViewModel.syncService
        function onSyncFailed(error)  {
            showError(error);
        }
        function onSyncTaskFinished() {
            root.visible = false
        }
    }


    Item {
        anchors.fill: parent
        anchors.topMargin: 10
        anchors.leftMargin: isMobile ? 15 : 20
        anchors.rightMargin: isMobile ? 15 : 20

        XSNLabel {
            id: message
            anchors.centerIn: parent
            width: parent.width
            font.pixelSize: 14
            font.family: lightFont.name
            wrapMode: Text.WrapAnywhere
        }
    }

    function showError(error)
    {
        root.visible = true
        message.text = error;
    }
}
