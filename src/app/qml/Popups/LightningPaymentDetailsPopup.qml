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

    popUpText: "Lightning payment details"
    width: 500
    height: 420

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
                    text: "Lightning payment details"
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
            spacing: 10

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
                    text: qsTr("Payment hash:")
                    font.pixelSize: 12
                }

                CopiedField {
                    width: parent.width
                    height: 30
                    bgColor: SkinColors.mainBackground
                    text: transaction !== null ? transaction.txId : ""
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            Column {
                spacing: 5

                SecondaryLabel {
                    text: qsTr("Payment activity:")
                    font.pixelSize: 12
                }

                SelectedText {
                    text: transaction !== null ? transaction.txActivity : ""
                    font.pixelSize: 13
                    font.family: lightFont.name
                    font.capitalization: Font.AllUppercase
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
                spacing: 5

                SecondaryLabel {
                    anchors.leftMargin: 10
                    anchors.left: parent.left
                    text: qsTr("Status:")
                    font.pixelSize: 12
                }

                SecondaryLabel {
                    anchors.leftMargin: 10
                    anchors.left: parent.left
                    font.pixelSize: 13
                    font.capitalization: Font.AllUppercase
                    color: transaction !== null ? transaction.statusColor : SkinColors.mainText
                    text: transaction !== null ? transaction.status : ""
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
        }
    }
}

