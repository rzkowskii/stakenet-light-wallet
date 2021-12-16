import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ColumnLayout {
    id: root

    property double accountBalance: -1
    property bool isApproximatelyEqual: false

    XSNLabel {
        Layout.preferredHeight: 50
        verticalAlignment: Text.AlignVCenter
        text: "Portfolio"
    }

    Rectangle {
        Layout.preferredHeight: 100
        Layout.fillWidth: true
        color: SkinColors.secondaryBackground

        Item {
            anchors.fill: parent
            anchors.margins: 20

            Row {
                spacing: 5

                XSNLabel {
                    id: coinsAmount
                    text: isApproximatelyEqual ? "~ %1".arg(
                                                     accountBalance.toFixed(
                                                         2)) : mainPage.balanceVisible ? accountBalance.toFixed(2) : hideBalance(accountBalance.toFixed(2))
                }

                XSNLabel {
                    text: ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode
                    anchors.top: coinsAmount.top
                    font.pixelSize: coinsAmount.font.pixelSize * 0.5
                    font.capitalization: Font.AllUppercase
                }
            }
        }
    }
}
