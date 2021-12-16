import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../Components"
import "../Views"
import "../Pages"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0


Item {
    id: root
    property string assetColor: ""
    property int assetID: -1
    property string name: ""
    property string symbol: ""
    property var balance: ""
    property var transactionListModel: undefined
    property bool addressTypeExist: false

    Component.onCompleted: ApplicationViewModel.walletViewModel.requestAddressType(assetID);

    Component {
        id: mobileSendPageComponent
        MobileSendPage {

        }
    }

    Component {
        id: mobileReceivePageComponent
        MobileReceivePage {

        }
    }
    Component {
        id: historyViewComponent
        HistoryView {

        }
    }

    RefreshDialogComponent {
        id: refreshDialogComp
    }

    Connections {
        target: ApplicationViewModel.walletViewModel
        function onRequestAddressTypeFinished(assetID, isExist) {
            addressTypeExist = !(assetID === root.assetID && !isExist)
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: addressTypeExist ? 0 : 1

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true

            FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

            LinearGradient {
                anchors.fill: parent
                opacity: 0.2
                start: Qt.point(0, 0)
                end: Qt.point(0, height)
                gradient: Gradient {
                    GradientStop { position: 1.0; color: "transparent" }
                    GradientStop { position: 0.5; color: "transparent" }
                    GradientStop { position: 0.0; color: SkinColors.walletAssetsBackgroundGradient}
                }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 40
                anchors.bottomMargin: 30
                spacing: 25

                Item {
                    id: header
                    Layout.fillWidth: true
                    Layout.preferredHeight: 135

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 15

                        ColumnLayout {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.fillWidth: true
                            spacing: 5

                            IconButton {
                                Layout.alignment: Qt.AlignCenter
                                source: hovered ? "qrc:/images/ic_refresh.svg" : name !== "" ? "qrc:/images/ICON_%1.svg".arg(name): ""
                                sourceSize: Qt.size(45, 50)
                                hoverEnabled: true

                                PointingCursorMouseArea {
                                    onClicked: {
                                        var activePopup = refreshDialogComp.createObject( root,{assetID: assetID, coinSymbol: symbol})
                                        activePopup.open();
                                    }
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                Text {
                                    id: assetsName
                                    text: name
                                    font.family: regularFont.name
                                    font.pixelSize: 14
                                    color: SkinColors.mainText
                                }

                                XSNLabel {
                                    text: qsTr(" - " + symbol)
                                    font.family: regularFont.name
                                    font.pixelSize: 14
                                    font.capitalization: Font.AllUppercase
                                    color: SkinColors.mainText
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignHCenter
                            spacing: 7

                            RowLayout {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.fillWidth: true
                                spacing: 5

                                XSNLabel {
                                    text: Utils.formatBalance(balance)
                                    font.pixelSize: 24
                                    color: assetColor
                                }

                                XSNLabel {
                                    text: symbol
                                    font.pixelSize: 24
                                    font.capitalization: Font.AllUppercase
                                    color: assetColor
                                }
                            }

                            SecondaryLabel {
                                Layout.alignment: Qt.AlignHCenter
                                font.family: regularFont.name
                                font.pixelSize: 12
                                text: "= %1 %2" .arg(ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol).arg(ApplicationViewModel.localCurrencyViewModel.convert(assetID, Utils.formatBalance(balance)))
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.alignment: Qt.AlignCenter

                    XSNLabel {
                        anchors.centerIn: parent
                        text: "Chart"
                        color: SkinColors.mainText
                        font.pixelSize: 20
                    }

                    ListView {
                        id: listView
                        anchors.fill: parent
                        spacing: parent.height / 3 - 33

                        model: 3
                        currentIndex: 1
                        delegate: MobileButton {
                            anchors.right: parent.right
                            anchors.rightMargin: 10
                            width: 73
                            height: 33
                            backgroundButton.border.color: ListView.isCurrentItem ? assetColor : "transparent"
                            backgroundButton.color: SkinColors.secondaryBackground
                            backgroundButton.radius: 16
                            title.text: "%1 %2" .arg(ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol) .arg("9,841.29")

                            onClicked: {
                                listView.currentIndex = index
                            }
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 30

                    ChoosingListView {
                        Layout.preferredHeight: 40
                        Layout.fillWidth: true
                        Layout.leftMargin: 10
                        Layout.rightMargin: 10
                        model: ["24H", "7D", "1M", "1Y", "MAX"]
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 15
                        Layout.rightMargin: 15
                        spacing: 12

                        MobileActionButton {
                            checkable: true
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40

                            source: "qrc:/images/SEND_MONEY.png"
                            buttonColor: assetColor
                            buttonText: qsTr("Send")

                            PointingCursorMouseArea {
                                onClicked: navigateToItem (mobileSendPageComponent, {assetID :  root.assetID })
                            }
                        }

                        MobileActionButton {
                            checkable: true
                            Layout.fillWidth: true

                            Layout.preferredHeight: 40

                            source: "qrc:/images/RECEIVE_MONEY.png"
                            buttonColor: assetColor
                            buttonText: qsTr("Receive")

                            PointingCursorMouseArea {
                                onClicked: navigateToItem (mobileReceivePageComponent, {assetID :  root.assetID })
                            }
                        }
                    }
                }

                MobileFooter {
                    Layout.leftMargin: 25
                    Layout.rightMargin: 25
                    Layout.preferredHeight: 40
                    leftButton.text: "back"
                    rightButton.text: "history"
                    onLeftButtonClicked: {
                        navigateBack()
                    }
                    onRightButtonClicked: {
                        navigateToItem(historyViewComponent, {transactionListModel : root.transactionListModel, assetColor : root.assetColor, assetID : root.assetID})
                    }
                }
            }
        }


        WalletAddressView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            onClicked: {
                ApplicationViewModel.walletViewModel.setAddressType(assetID,  type);

                walletViewModel.receiveTxViewModel.setAddressType(
                            currentAssetID, type)

                addressTypeExist = true
                //showBubblePopup("Applied");
            }
        }
    }
}
