import QtQuick 2.1
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import "../Components"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root
    property int assetID
    property var transaction
    property string symbol: ""
    property int bitcoinID: 0

    signal openUrlRequested(string txid)

    popUpText: "Transaction details"
    width: 500
    height: 500

    SendTransactionViewModel {
        id: sendTxViewModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
            requestAddressDetails(root.assetID)
        }
        onTransactionResubmitted: {
            showBubblePopup("Transaction " + txId + " succesfully resubmitted")
        }
        onTransactionResubmitFailed: {
            showBubblePopup("Failed to resubmit transaction " + errorMessage)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 22
        anchors.bottomMargin: 22
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 20

        FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }
        FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
        FontLoader { id: lightFont; source: "qrc:/Rubik-Light.ttf" }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 20

            RowLayout {
                Layout.fillWidth: true

                XSNLabel {
                    font.family: mediumFont.name
                    font.pixelSize: 20
                    text: "Transaction Details"
                    color: SkinColors.mainText
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                CloseButton {
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 30
                    Layout.alignment: Qt.AlignRight | Qt.AlignTop
                    onClicked: root.close()
                }
            }

            Rectangle {
                Layout.preferredHeight: 1
                Layout.fillWidth: true
                color: SkinColors.secondaryText
            }

        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 150
            spacing: 6

            SecondaryLabel {
                text: qsTr("Amount")
                font.pixelSize: 12
            }

            RowLayout {
                Layout.fillWidth: true

                SelectedText {
                    text: transaction !== null ? "%1 %2 %3".arg((transaction.txAmount > 0 ? "+" : "-")).arg(transaction.delta).arg(symbol) : ""
                    color: transaction !== null ? (transaction.txAmount > 0 ? "#3FBE4D" : "#D34947") : ""
                    font.pixelSize: 24
                    font.family: lightFont.name
                }

                Item {
                    Layout.fillWidth: true
                }

                Item {
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    Layout.alignment: Qt.AlignRight

                    Image {
                        anchors.centerIn: parent
                        source: transaction !== null ? (transaction.txAmount > 0 ? "qrc:/images/IC_RECEIVE_DETAILS.png" : "qrc:/images/IC_SEND_DETAILS.png" ) : ""
                        sourceSize: Qt.size(40, 40)
                    }
                }
            }

            SelectedText {
                text: transaction !== null ? ("= %1 %2".arg(transaction !== null ? ApplicationViewModel.localCurrencyViewModel.convert(assetID, transaction.delta) : "").arg(ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)) : ""
                font.pixelSize: 12
                font.family: lightFont.name
                color: SkinColors.mainText
            }

            SecondaryLabel {
                text: qsTr("Fee")
                font.pixelSize: 12
            }

            SelectedText {
                text: transaction !== null ? "%1 ETH".arg(transaction.fee) : ""
                font.pixelSize: 12
                font.family: lightFont.name
                color: SkinColors.mainText
            }

            SelectedText {
                text: transaction !== null ? ("= %1 %2".arg(transaction !== null ? ApplicationViewModel.localCurrencyViewModel.convert(60, transaction.fee) : "").arg(ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)) : ""
                font.pixelSize: 12
                font.family: lightFont.name
                color: SkinColors.mainText
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            Column {
                spacing: 5

                SecondaryLabel {
                    text: qsTr("Date:")
                    font.pixelSize: 12
                }

                SelectedText {
                    text: transaction !== null ? Utils.formatDate(transaction.txDate) : ""
                    font.pixelSize: 13
                    font.family: lightFont.name
                    color: SkinColors.mainText
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Column {
                Layout.fillHeight: true
                Layout.preferredWidth: 220
                Layout.alignment: Qt.AlignRight

                SecondaryLabel {
                    anchors.leftMargin: 10
                    anchors.left: parent.left
                    text: qsTr("Transaction Id:")
                    font.pixelSize: 12
                }

                CopiedField {
                    width: parent.width
                    height: 30
                    bgColor: SkinColors.mainBackground
                    text: transaction !== null ? transaction.txId : ""
                }

                XSNLabel {
                    anchors.right: parent.right
                    text: "<a href=\"dummy\">Open in explorer</a>"
                    font.underline: true
                    font.pixelSize: 12
                    visible: transaction.confirmations > 0 || assetID === bitcoinID
                    font.family: mediumFont.name
                    linkColor: SkinColors.mainText

                    PointingCursorMouseArea{
                        onClicked: {
                            openUrlRequested(transaction.txId);
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            Column {
                spacing: 5

                SecondaryLabel {
                    text: qsTr("Nonce:")
                    font.pixelSize: 12
                }

                SelectedText {
                    text: transaction.nonce
                    font.pixelSize: 13
                    font.family: lightFont.name
                    color: SkinColors.mainText
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            Column {
                Layout.fillHeight: true
                Layout.preferredWidth: 220
                Layout.alignment: Qt.AlignRight

                SecondaryLabel {
                    text: qsTr("Block hash:")
                    anchors.leftMargin: 10
                    anchors.left: parent.left
                    font.pixelSize: 12
                }

                CopiedField {
                    width: parent.width
                    height: 30
                    bgColor: SkinColors.mainBackground
                    text: transaction.blockHash
                }
            }

        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            Column {
                spacing: 5

                SecondaryLabel {
                    text: qsTr("Status:")
                    font.pixelSize: 12
                }

                Row {
                    spacing: 10
                    SelectedText {
                        text: transaction !== null ? transaction.status : ""
                        font.pixelSize: 13
                        font.capitalization: Font.AllUppercase
                        font.family: lightFont.name
                        color: transaction.statusColor
                    }

                    Button {
                        visible: false
                        text: "Retry"
                        onClicked: sendTxViewModel.resubmit(transaction.txId)
                    }
                }

                SelectedText {
                    text: {
                        if (transaction.status === "conflicted") {
                            return "";
                        } else {
                            return qsTr("Confirmations: %1").arg(transaction.status === "pending" ? "%1/%2".arg(transaction.confirmations).arg(transaction.confirmationsForApproved) :
                                                                                                    formatConfirmations(transaction.confirmations))
                        }

                    }
                    font.pixelSize: 13
                    font.family: lightFont.name
                    color: SkinColors.mainText
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            Column {
                Layout.fillHeight: true
                Layout.preferredWidth: 220
                Layout.alignment: Qt.AlignRight

                SecondaryLabel {
                    text: qsTr("Address:")
                    anchors.leftMargin: 10
                    anchors.left: parent.left
                    font.pixelSize: 12
                }

                CopiedField {
                    width: parent.width
                    height: 30
                    bgColor: SkinColors.mainBackground
                    text: transaction.addresses[0]
                }

                XSNLabel {
                    anchors.right: parent.right
                    text: qsTr("See more addresses")
                    font.underline: true
                    font.pixelSize: 14
                    visible: transaction !== null ? transaction.addresses.size > 1 : false
                    font.family: mediumFont.name
                    color: mouseArea.containsMouse ? SkinColors.mainText : SkinColors.secondaryText

                    PointingCursorMouseArea {
                        id: mouseArea
                        onClicked: openTransactionAddressesDialog({model: transaction.addresses})
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
        }
    }

    function formatConfirmations(conf) {
        if(conf < 50) {
            return conf;
        } else if(conf < 100) {
            return "50+";
        } else if(conf < 500) {
            return "100+";
        } else if(conf < 1000) {
            return "500+";
        } else {
            return "1000+";
        }
    }
}

