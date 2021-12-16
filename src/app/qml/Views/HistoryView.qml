import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.utils 1.0
import com.xsn.models 1.0
import com.xsn.viewmodels 1.0


Item {
    id: root
    property QtObject transactionListModel: undefined
    property int assetID: 0
    property string assetColor: ""

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    WalletAssetViewModel {
        id: walletViewModel
        Component.onCompleted: {
            initialize(ApplicationViewModel);
        }
        currentAssetID: assetID
    }

    Layout.fillHeight: true
    Layout.fillWidth: true

    Rectangle {
        anchors.fill: parent
        color: "transparent"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 35
        anchors.bottomMargin: 40
        spacing: 20

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 20

            MobileTitle {
                anchors.centerIn: parent
                text: "history"
            }
        }

        ListView {
            id: listView
            Layout.fillHeight: true
            Layout.fillWidth: true
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            model: QMLSortFilterListProxyModel {
                source: root.transactionListModel
                sortRole: "txDate"
                sortAsc: false
            }

            spacing: 5
            property string currentDate: model[0] !== undefined ? Utils.mobileFormatDate(model[0].txDate) : ""

            delegate: Column {
                id: transactionItem
                spacing: 0
                width: parent.width

                Item {
                    id: dateItem
                    width: parent.width
                    height: 30

                    Component.onCompleted: showDate(Utils.mobileFormatDate(txDate))

                    SecondaryLabel {
                        anchors.left: parent.left
                        anchors.leftMargin: 5
                        anchors.verticalCenter: parent.verticalCenter
                        font.family: regularFont.name
                        font.pixelSize: 12
                        text: Utils.mobileFormatDate(txDate)

                    }
                }

                Rectangle {
                    radius: 8
                    width: parent.width
                    height: transactionItem.ListView.isCurrentItem ? 290 : 49 //249
                    color: SkinColors.secondaryBackground

                    property var addresses: model.addresses

                    PointingCursorMouseArea {
                        onClicked: listView.currentIndex = index
                    }

                    ColumnLayout {
                        id: layout
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 0

                        RowLayout {
                            Layout.alignment: Qt.AlignVCenter
                            Layout.preferredHeight: 49
                            Layout.fillWidth: true
                            spacing: 5

                            Image {
                                Layout.alignment: Qt.AlignVCenter
                                source: model.activity === "received" ? "qrc:/images/RECEIVE_MONEY.png" : "qrc:/images/SEND_MONEY.png"
                                sourceSize.width: 25
                                sourceSize.height: 25
                            }

                            Text {
                                Layout.alignment: Qt.AlignVCenter
                                text: model.activity
                                color: SkinColors.mainText
                                font.pixelSize: 14
                                font.capitalization: Font.Capitalize
                                font.family: regularFont.name
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Text {
                                visible: !transactionItem.ListView.isCurrentItem
                                Layout.alignment: Qt.AlignVCenter
                                font.family: regularFont.name
                                font.pixelSize: 14
                                color: assetColor
                                text: "%1%2  %3" .arg(model.txAmount > 0 ? "+" : "-") .arg(model.delta) .arg(model.symbol)
                                font.capitalization: Font.Capitalize
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.bottomMargin: 15
                            Layout.preferredHeight: 241
                            visible: transactionItem.ListView.isCurrentItem
                            spacing: 12

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: SkinColors.mobileLineSeparator
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 12

                                Column {
                                    Layout.fillWidth: true
                                    spacing: 3

                                    SecondaryLabel {
                                        text: "Amount:"
                                        font.pixelSize: 11
                                        font.family: regularFont.name
                                    }

                                    XSNLabel {
                                        text: "%1%2 %3" .arg(model.txAmount > 0 ? "+" : "-") .arg(model.delta) .arg(model.symbol)
                                        font.pixelSize: 22
                                        color: assetColor
                                        font.capitalization: Font.AllUppercase
                                    }

                                }

                                Column {
                                    Layout.fillWidth: true
                                    spacing: 3

                                    SecondaryLabel {
                                        text: "From:"
                                        font.pixelSize: 11
                                        font.family: regularFont.name
                                    }

                                    XSNLabel {
                                        text: addresses[0]
                                        font.pixelSize: 12
                                    }
                                }

                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 15

                                Column {
                                    Layout.fillWidth: true
                                    spacing: 3

                                    SecondaryLabel {
                                        text: "Transactions ID:"
                                        font.pixelSize: 11
                                        font.family: regularFont.name
                                    }

                                    XSNLabel {
                                        width: layout.width / 2
                                        text: model.id
                                        font.pixelSize: 11
                                        wrapMode: Text.WrapAnywhere
                                    }

                                }

                                ColumnLayout {
                                    Layout.fillWidth: true

                                    Column {
                                        Layout.alignment: Qt.AlignTop
                                        spacing: 3

                                        SecondaryLabel {
                                            text: "Network Fee:"
                                            font.pixelSize: 11
                                            font.family: regularFont.name
                                        }

                                        XSNLabel {
                                            text: "%1 %2" .arg("0.02").arg(ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode)
                                            font.pixelSize: 12
                                        }
                                    }

                                    XSNLabel {
                                        Layout.alignment: Qt.AlignBottom
                                        text: "<a href=\"dummy\">Open in explorer</a>"
                                        font.underline: true
                                        font.pixelSize: 12
                                        visible: model.confirmations >= 0
                                        font.family: regularFont.name
                                        linkColor: SkinColors.menuItemText


                                        PointingCursorMouseArea{
                                            onClicked: {
                                                var link = walletViewModel.buildUrlForExplorer(model.id);
                                                console.log("Requested to open url for txid %1, url: %2".arg(model.id).arg(link));
                                                if(link.length > 0)
                                                {
                                                    Qt.openUrlExternally(link);
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: SkinColors.mobileLineSeparator
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 20
                                Layout.alignment: Qt.AlignHCenter

                                Row {
                                    anchors.centerIn: parent
                                    spacing: 7

                                    Item {
                                        width: 15
                                        height: 15

                                        ColorOverlayImage {
                                            anchors.fill: parent
                                            anchors.centerIn: parent
                                            imageSize: 15
                                            imageSource: "qrc:/images/mail.png"
                                            color: SkinColors.menuBackgroundGradientFirst
                                        }
                                    }

                                    Text {
                                        text: "email"
                                        font.pixelSize: 14
                                        color: SkinColors.menuBackgroundGradientFirst
                                        font.capitalization: Font.AllUppercase
                                    }
                                }

                                PointingCursorMouseArea {
                                    //onClicked
                                }
                            }
                        }
                    }
                }

                function showDate(date)
                {
                    if(listView.currentDate !== date)
                    {
                        dateItem.visible = true
                        listView.currentDate = date
                    }
                    else
                    {
                        dateItem. visible = false
                    }
                }
            }
        }

        MobileFooter {
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            leftButton.text: "back"
            rightButton.visible: false
            onLeftButtonClicked: {
                navigateBack()
            }
        }
    }
}
