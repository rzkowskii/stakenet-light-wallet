import QtQuick 2.15
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.15

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import com.xsn.models 1.0

Item {
    id: exchangeSuccessfulView

    property string swapErrorMsg: ""

    property alias cancelButton: closeSwapBtn
    property alias retryButton: retrySwapBtn

    signal cancelButtonClicked
    signal retryButtonClicked

    GradientBackgroundComponent {
        anchors.fill: parent
        gradientOpacity: 0.7
        firstColor: SkinColors.swapPageBackgroundFooterFirst
        secondColor: SkinColors.swapPageBackgroundFooterSecond
        thirdColor: SkinColors.swapPageBackgroundFooterThird
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 30
        anchors.bottomMargin: 30
        anchors.leftMargin: 30
        anchors.rightMargin: 30
        spacing: root.height * 0.03

        Image {
            id: name
            Layout.topMargin: 50
            Layout.alignment: Qt.AlignHCenter
            width: 90
            height: 100
            source: "qrc:/images/exchange_failed_artwork.svg"
            sourceSize: Qt.size(90, 100)
        }

        Text {
            Layout.fillWidth: true
            Layout.topMargin: 25
            horizontalAlignment: Text.AlignHCenter
            font.family: fontRegular.name
            font.pixelSize: 26
            color: SkinColors.dexDisclaimerTextColor
            text: "Exchange Failed!"
        }

        Text {
            id: willReceiveText
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            font.family: fontRegular.name
            font.pixelSize: 15
            color: "#EC6D75"
            text: swapErrorMsg
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
            Layout.maximumHeight: 50
            Layout.fillWidth: true
            Layout.topMargin: 35

            SwapActionButton {
                id: closeSwapBtn
                Layout.preferredHeight: item.height / 2 * 0.14
                Layout.preferredWidth: item.width / 5
                borderColor: SkinColors.popupFieldBorder
                radius: 10
                text: "Close"
                onClicked: cancelButtonClicked()
            }

            SwapActionButton {
                id: retrySwapBtn
                Layout.preferredHeight: item.height / 2 * 0.14
                Layout.preferredWidth: item.width / 5
                borderColor: SkinColors.popupFieldBorder
                radius: 10
                text: "Retry"
                onClicked: retryButtonClicked()
                enabled: false
                visible: false
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}
