import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

import "../Components"
import "../Views"

Item {
    property string currentSymbol: ""

    ColumnLayout {
        anchors.leftMargin: 60
        anchors.fill: parent
        spacing: 15

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 35

            XSNLabel {
                anchors.verticalCenter: parent.verticalCenter
                font.family: mediumFont.name
                font.pixelSize: 24
                text: "Transaction History"
                color: SkinColors.mainText
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 45

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10

                    Repeater {
                        id: repeater
                        model: ListModel {
                            ListElement { name: "TX HASH"; size: 0.20}
                            ListElement { name: "TYPE"; size: 0.1}
                            ListElement { name: "RESULT"; size: 0.1}
                            ListElement { name: "AMOUNT"; size: 0.15}
                            ListElement { name: "FEE"; size: 0.1}
                            ListElement { name: "TIME"; size: 0.15}
                        }

                        delegate: Item {
                            Layout.preferredWidth: parent.width * model.size
                            Layout.fillHeight: true

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: model.name
                                font.capitalization: Font.AllUppercase
                                font.family: regularFont.name
                                font.pixelSize: 14
                                color: SkinColors.secondaryText
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }
            }

            ListView {
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true
                spacing: 0
                boundsBehavior: Flickable.StopAtBounds

                model: QMLSortFilterListProxyModel {
                    source: paymentsModel
                    sortRole: "timestamp"
                    sortCaseSensitivity: Qt.CaseInsensitive
                    sortAsc: false
                }

                delegate:  Item {
                    id: delegate
                    width: parent.width
                    height: 50

                    property string paymentHash: model.paymentHash
                    property var type: model.type
                    property var status: model.status
                    property double amount: model.amount
                    property double fee: model.fee
                    property string timestamp: model.timestamp

                    Rectangle {
                        anchors.fill: parent
                        color: index % 2 === 1  ? "transparent" : SkinColors.secondaryBackground
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 15
                        anchors.rightMargin: 15

                        CopiedField {
                            Layout.preferredWidth: parent.width * 0.2
                            textColor: SkinColors.menuBackgroundGradientFirst
                            Layout.preferredHeight: 40
                            text: delegate.paymentHash
                            bgColor: index % 2 === 1 ? SkinColors.secondaryBackground : SkinColors.mainBackground
                        }

                        Text {
                            Layout.preferredWidth: parent.width * 0.1
                            text: delegate.type
                            color: SkinColors.mainText
                            font.family: regularFont.name
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }

                        Row {
                            Layout.preferredWidth: parent.width * 0.1
                            spacing: 5

                            Image {
                                anchors.verticalCenter: parent.verticalCenter
                                sourceSize: Qt.size(15, 15)
                                source: {
                                    switch(delegate.status) {
                                    case "Success": return "qrc:/images/green-checkmark.png";
                                    case "Fail": return "qrc:/images/cross-red-.png";
                                    }

                                    return "";
                                }
                            }

                            Text {
                                Layout.preferredWidth: parent.width * 0.15
                                text: delegate.status === "InProgress" ? "In Progress" : delegate.status
                                color: SkinColors.mainText
                                font.family: regularFont.name
                                font.pixelSize: 12
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Text {
                            Layout.preferredWidth: parent.width * 0.15
                            text: "%1 %2" .arg(Utils.formatBalance(delegate.amount)).arg(currentSymbol)
                            color: SkinColors.mainText
                            font.family: regularFont.name
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }

                        Text {
                            Layout.preferredWidth: parent.width * 0.1
                            text: "%1 %2" .arg(Utils.formatBalance(delegate.fee)).arg(currentSymbol)
                            color: SkinColors.mainText
                            font.family: regularFont.name
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }

                        Text {
                            Layout.preferredWidth: parent.width * 0.15
                            text: Utils.formatDate(delegate.timestamp)
                            color: SkinColors.mainText
                            font.family: regularFont.name
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }
    }
}
