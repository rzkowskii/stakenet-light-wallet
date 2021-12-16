import QtQuick 2.4
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

import "../../Components"
import "../../Pages"

StackViewPage {
    id: root
    skipButton.visible: false
    backButton.visible: false
    headerText: "Restore from backup"
    headerSource: "qrc:/images/ICON_BKUP_WALLET_SMALL.png"
    backButton.onClicked: {
        stackView.pop();
    }

    background:  Image {
        anchors.fill: parent
        source: "qrc:/images/BG MAIN 2.png"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 50
        anchors.rightMargin: 50
        anchors.topMargin: 50
        anchors.bottomMargin: 30

        Item {
            Layout.preferredHeight: 120
            Layout.preferredWidth: 120
            Layout.alignment: Qt.AlignHCenter

            ColorOverlayImage {
                anchors.fill: parent
                anchors.horizontalCenter: parent.horizontalCenter
                imageSize: 150
                color: SkinColors.mainText
                imageSource: "qrc:/images/SUCCESS_ICON.png"
            }
        }

        XSNLabel {
            Layout.alignment: Qt.AlignHCenter
            text: stackView.finishText
            font.pixelSize: 30
        }

        IntroButton {
            Layout.preferredWidth: 180
            Layout.preferredHeight: 45
            text: qsTr("OK")
            Layout.alignment: Qt.AlignHCenter
            onClicked: replaceView(mainPage, {isOpenRescanNotificationPopup : true});
        }
    }
}
