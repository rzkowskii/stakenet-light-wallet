import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick.Controls 2.2
import QtQuick 2.12
import QtQuick.Controls 2.12

import "../Components"
import com.xsn.utils 1.0
import com.xsn.viewmodels 1.0
import com.xsn.models 1.0

Rectangle {
    id: root
    radius: 10
    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: SkinColors.delegatesBackgroundLightColor
        }
        GradientStop {
            position: 1.0
            color: SkinColors.botTextFieldActiveBorderColor
        }
    }

    property string assetColor: ""
    property int assetID: -1
    property string name: ""
    property string symbol: ""
    property string balance: ""
    property var totalBalance: Utils.formatBalance(balance)
    property double balanceOnChain: 0
    property double nodeBalance: 0
    property double minLndCapacity: 0
    property string confirmationsForApproved: ""
    property int averageSycBlockForSec
    property int chainID: -1
    property var syncStateProvider: ApplicationViewModel.syncService.assetSyncProvider(
                                        assetID)
    property QtObject transactionListModel: undefined
    property string confirmationsForChannelApproved: ""
    property bool balanceVisible
    property var nodeStatus: {
        if (!paymentNodeViewModel.stateModel)
            return undefined
        else
            return paymentNodeViewModel.stateModel.nodeStatus
    }

    signal assetClicked

    PaymentNodeViewModel {
        id: paymentNodeViewModel
        currentAssetID: assetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    RowLayout {
        anchors {
            fill: parent
            leftMargin: 5
            rightMargin: 10
            topMargin: 5
            bottomMargin: 5
        }
        spacing: 10

        Item {
            Layout.preferredHeight: 63
            Layout.preferredWidth: 63

            FadedImage {
                anchors.centerIn: parent
                fadeColor: assetColor
                imageSource: "qrc:/images/ICON_%1.svg".arg(name)
                imageSize: Qt.size(32, 36)
                visible: syncStateProvider && !(syncStateProvider.syncing
                                                || syncStateProvider.scanning)
            }

            WalletBusyIndicator {
                id: syncingIndicator
                anchors.centerIn: parent
                color: SkinColors.mainText
                visible: syncStateProvider && (syncStateProvider.syncing
                                               || syncStateProvider.scanning)
            }
        }

        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 8

            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true

                Text {
                    id: assetsName
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    text: "%1 - %2".arg(name).arg(symbol)
                    font.family: fontMedium.name
                    font.pixelSize: 14
                    color: SkinColors.mainText
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    font.family: fontRegular.name
                    font.pixelSize: 12
                    text: "%1 - %2".arg(
                              balanceVisible ? totalBalance : hideBalance(
                                                   totalBalance)).arg(symbol)
                    color: SkinColors.mainText
                    width: 180
                }

                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignLeft
                    color: SkinColors.secondaryText
                    font.pixelSize: 10
                    font.family: fontRegular.name
                    text: "%1 %2".arg(
                              balanceVisible ? ApplicationViewModel.localCurrencyViewModel.convert(
                                                   assetID,
                                                   totalBalance) : hideBalance(
                                                   (ApplicationViewModel.localCurrencyViewModel.convert(
                                                        assetID,
                                                        totalBalance)))).arg(
                              ApplicationViewModel.localCurrencyViewModel.currentCurrencySymbol)
                }
            }
        }

        NodeStatusIndicator {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 5
            Layout.preferredHeight: 6
            Layout.preferredWidth: 6
            //            nodeStatus: root.nodeStatus
            payNodeViewModel: paymentNodeViewModel

            CustomToolTip {
                x: -20
                y: -40
                height: 35
                width: 100
                tailPosition: 22
                visible: mouseArea.containsMouse
                text: {
                    if (!paymentNodeViewModel.stateModel)
                        return ""
                    else
                        return paymentNodeViewModel.stateModel.nodeStatusString
                }
            }
        }
    }

    PointingCursorMouseArea {
        id: mouseArea
        onClicked: {
            listView.actualIndex = index
            assetClicked()
            //            animationGradientColor.stop()
            //            fadeBackground.stopFade()
        }
        //        onEntered: {
        //            if (!asset.ListView.isCurrentItem) {
        //                animationGradientTransparent.stop()
        //                animationGradientColor.start()
        //                fadeBackground.startFade()
        //            }
        //        }
        //        onExited: {
        //            if (!asset.ListView.isCurrentItem) {
        //                animationGradientColor.stop()
        //                animationGradientTransparent.start()
        //                fadeBackground.stopFade()
        //            }
        //        }
    }
}
