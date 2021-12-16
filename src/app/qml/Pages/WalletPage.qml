import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Page {
    id: root
    property var currentAssetIdNodeStatus: assetsListView.currentItem.nodeStatus
    property int currentAssetID: assetsListView.currentItem ? assetsListView.currentItem.assetID : initialModel.get(
                                                                  assetsListView.actualIndex).id
    property var syncStateProvider: ApplicationViewModel.syncService.assetSyncProvider(
                                        currentAssetID)
    property string currentCurrencyCode: ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode
    property string currentCurrencySymbol: ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol
    property bool addressTypeExist: false
    property string accountBalance: portfolioPage.accountBalance
    property alias rootLayout: layout

    Connections {
        target: walletViewModel.receiveTxViewModel
        ignoreUnknownSignals: true

        function onRequestAddressTypeFinished(assetID, isExist) {
            addressTypeExist = !(assetID === currentAssetID && !isExist)
        }
    }

    ChainViewModel {
        id: chainViewModel
        assetID: walletViewModel.assetInfo.chainId

        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    PaymentNodeViewModel {
        id: payNodeViewModel
        currentAssetID: root.currentAssetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }

        onLnInvoiceReady: {
            Clipboard.setText(invoice)
            showBubblePopup("Copied")
        }
    }

    RefreshDialogComponent {
        id: refreshDialogComp
    }

    Component {
        id: apSettingsComponent
        APSettingsPopup {}
    }

    Component {
        id: chooseLightningComponent
        ChooseLightningPopup {}
    }

    background: Rectangle {
        color: SkinColors.delegatesBackgroundLightColor
    }

    WalletAssetViewModel {
        id: walletViewModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
            if (walletViewModel.receiveTxViewModel.isUTXOType) {
                walletViewModel.receiveTxViewModel.requestAddressType()
            }
        }
        currentAssetID: root.currentAssetID

        onCurrentAssetIDChanged: {
            if (walletViewModel.receiveTxViewModel) {
                if (walletViewModel.receiveTxViewModel.isUTXOType) {
                    walletViewModel.receiveTxViewModel.requestAddressType()
                }
            }
        }
    }

    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }
    FontLoader {
        id: fontMedium
        source: "qrc:/Rubik-Medium.ttf"
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            color: SkinColors.menuBackground

            TabHeaderListView {
                id: walletTabListView
                model: ["Transactions", "Channels"]
                anchors {
                    left: parent.left
                    bottom: parent.bottom
                }

                height: 57
                width: 480
                delegateWidth: 210

                currentIndex: 0
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.bottomMargin: 7
            Layout.rightMargin: 14
            spacing: 24

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 270
                Layout.fillWidth: isMobile
                color: "transparent"

                Item {
                    id: content
                    anchors.fill: parent
                    anchors.leftMargin: isMobile ? 15 : 24
                    anchors.rightMargin: isMobile ? 15 : 0
                    anchors.bottomMargin: isMobile ? 10 : 0

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 8

                        WalletHeader {
                            id: walletHeader
                            Layout.fillWidth: true
                        }

                        WalletAssetsListView {
                            id: assetsListView
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            balanceVisible: mainPage.balanceVisible

                            model: QMLSortFilterListProxyModel {
                                source: QMLSortFilterListProxyModel {
                                    source: WalletAssetsListModel {
                                        id: initialModel
                                        Component.onCompleted: initialize(
                                                                   ApplicationViewModel)
                                        onModelReset: {
                                            assetsListView.currentIndex = assetsListView.actualIndex
                                        }
                                    }
                                    filterRole: "isActive"
                                    filterString: "1"
                                    filterCaseSensitivity: Qt.CaseInsensitive
                                }
                                sortRole: "name"
                                filterRole: "nameSymbol"
                                filterString: isMobile ? searchButton.filterString : walletHeader.searchArea.text
                                filterCaseSensitivity: Qt.CaseInsensitive
                                sortCaseSensitivity: Qt.CaseInsensitive
                            }
                        }
                    }
                }

                MobileSearchButton {
                    id: searchButton
                    anchors.bottom: content.bottom
                    anchors.right: content.right
                    visible: isMobile
                }
            }

            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 12

                Text {
                    Layout.alignment: Qt.AlignLeft
                    text: qsTr("%1 Wallet").arg(
                              walletViewModel.assetInfo.symbol)
                    font.pixelSize: 15
                    font.family: fontRegular.name
                    color: SkinColors.mainText
                }

                StackLayout {
                    id: stackLayout
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    currentIndex: addressTypeExist
                                  || !walletViewModel.receiveTxViewModel.isUTXOType ? 0 : 1
                    visible: !isMobile

                    Item {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        Rectangle {
                            anchors.fill: parent
                            radius: 10
                            color: SkinColors.headerBackground
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop {
                                    position: 0.0
                                    color: SkinColors.delegatesBackgroundLightColor
                                }
                                GradientStop {
                                    position: 1.0
                                    color: SkinColors.delegatesBackgroundDarkColor
                                }
                            }
                        }

                        WalletPageHeaderView {
                            id: header
                            anchors.fill: parent
                            paymentNodeViewModel: payNodeViewModel
                            walletViewModel: walletViewModel
                            coinID: currentAssetID
                            balanceVisible: mainPage.balanceVisible
                            currentAssetNodeStatus: currentAssetIdNodeStatus
                            onSendCoinsRequested: openSendDialog(currentAssetID)
                            onReceiveCoinsRequested: openReceiveDialog(
                                                         currentAssetID)
                            onRefreshRequested: openDialog(refreshDialogComp, {
                                                               "assetID": currentAssetID,
                                                               "coinName": payNodeViewModel.stateModel.nodeType === Enums.PaymentNodeType.Lnd ? walletViewModel.assetInfo.name : "Ethereum",
                                                               "averageSyncTime": chainViewModel.chainHeight / walletViewModel.assetInfo.averageSycBlockForSec
                                                           })
                            onOpenInExplorerRequested: {
                                openInExplorer(txid)
                            }
                            onApChannelCapacityRequested: {
                                openDialog(apSettingsComponent, {
                                               "assetID": currentAssetID
                                           })
                            }
                        }
                    }

                    //                            Label {
                    //                                Layout.preferredHeight: 30
                    //                                Layout.fillWidth: true
                    //                                Layout.alignment: Qt.AlignBottom
                    //                                color: SkinColors.mainText
                    //                                text: "Headers: %1".arg(
                    //                                          chainViewModel.chainHeight)
                    //                                Layout.leftMargin: 5
                    //                                verticalAlignment: Text.AlignVCenter
                    //                                visible: !(syncStateProvider
                    //                                           && syncStateProvider.scanning)
                    //                                         || payNodeViewModel.stateModel.nodeType
                    //                                         === Enums.PaymentNodeType.Connext
                    //                            }
                    //                        }
                    WalletAddressView {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter

                        onClicked: {
                            if (walletViewModel.receiveTxViewModel.isUTXOType) {
                                walletViewModel.receiveTxViewModel.setAddressType(
                                            type)
                                addressTypeExist = true
                                showBubblePopup("Applied")
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    StackLayout {
                        anchors.fill: parent
                        currentIndex: walletTabListView.currentIndex

                        TransactionsListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            transactionListModel: walletViewModel.transactionsListModel
                            confirmationsForApproved: walletViewModel.assetInfo.сonfirmationsForApproved
                            onOpenInExplorerRequested: {
                                openInExplorer(txid)
                            }

                            onLnInvoicePayReqRequested: {
                                payNodeViewModel.invoicePaymentRequestByPreImage(rHash)
                            }
                        }

                        ChannelsView {
                            id: channelsView
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            payNodeViewModel: payNodeViewModel
                            assetID: root.currentAssetID
                            currentSymbol: walletViewModel.assetInfo.symbol
                            currentAssetName: walletViewModel.assetInfo.name
                            confirmationsForChannelApproved: walletViewModel.assetInfo.сonfirmationsForApproved
                        }
                    }
                }

                RescaningView {
                    Layout.alignment: Qt.AlignBottom
                    Layout.preferredHeight: 40
                    Layout.fillWidth: true
                    visible: syncStateProvider && syncStateProvider.scanning
                             && payNodeViewModel.stateModel.nodeType === Enums.PaymentNodeType.Lnd
                    stateProvider: syncStateProvider
                }
            }
        }
    }

    function openInExplorer(txid) {
        var link = walletViewModel.buildUrlForExplorer(txid)
        console.log("Requested to open url for txid %1, url: %2".arg(
                        txid).arg(link))
        if (link.length > 0) {
            Qt.openUrlExternally(link)
        }
    }
}
