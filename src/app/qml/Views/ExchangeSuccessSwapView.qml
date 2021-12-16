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

    property string receiveAssetSymbol: ""
    property double receivedAmount: 0
    property alias closeButton: swapSuccecsBtn

    signal closeButtonClicked

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
            source: "qrc:/images/exchange_success.svg"
            sourceSize: Qt.size(90, 100)
        }

        Text {
            Layout.fillWidth: true
            Layout.topMargin: 25
            horizontalAlignment: Text.AlignHCenter
            font.family: fontRegular.name
            font.pixelSize: 26
            color: SkinColors.dexDisclaimerTextColor
            text: "Exchange Success!"
        }

        Text {
            id: willReceiveText
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            font.family: fontRegular.name
            font.pixelSize: 18
            color: SkinColors.dexDisclaimerTextColor
            opacity: 0.8
            text: "You have successfully received %1 %2".arg(
                      Utils.formatBalance(receivedAmount)).arg(
                      receiveAssetSymbol)
        }

        SwapActionButton {
            id: swapSuccecsBtn
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
            Layout.preferredHeight: item.height / 2 * 0.14
            Layout.maximumHeight: 50
            Layout.preferredWidth: item.width / 5
            Layout.topMargin: 35
            borderColor: SkinColors.popupFieldBorder
            radius: 10
            text: "Close"
            onClicked: closeButtonClicked()
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }
    }
}
