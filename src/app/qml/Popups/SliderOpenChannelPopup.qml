import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import "../Components"
import "../Views"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ActionDialog {
    id: root
    width: 430
    height: 300

    property int assetID: -1
    property string assetName: ""
    property string assetSymbol: ""
    property string localCurrency: ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode
    property real channelCapacity: 0.0
    property string capacityInCurrency: ApplicationViewModel.localCurrencyViewModel.convert(
                                            assetID, channelCapacity)
    property string confirmationsForApproved: ""
    property PaymentNodeViewModel paymentNodeViewModel
    property double recommendedNetworkFeeRate: 0
    property double networkFeeRate: 0

    signal openChannelCanceled

    onRecommendedNetworkFeeRateChanged: {
        root.networkFeeRate = calculateNetworkFeeRate(
                    networkFeeRateView.currentOption ? networkFeeRateView.currentOption : "Medium",
                    recommendedNetworkFeeRate)
    }

    onClosed: {
        openChannelCanceled()
    }

    WalletAssetsListModel {
        id: assetModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
            averageFeeForAsset(assetID)
        }
        onAverageFeeForAssetFinished: {
            networkFeeRate = recommendedNetworkFeeRate = value
        }
    }

    Connections {
        target: paymentNodeViewModel
        function onDepositChannelFinished(channelAddress) {
            showBubblePopup("Channel was successfully funded!")
            root.close()
        }

        function onDepositChannelFailed(errMsg) {
            showBubblePopup("Failed to fund channel: %1!".arg(errMsg))
            root.close()
        }
    }

    OpenChannelViewModel {
        id: openChannelViewModel
        currentAssetID: assetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }

        onRequestCreated: {
            openChannelViewModel.confirmRequest()
        }
        onChannelOpened: {
            showBubblePopup("Channel was successfully funded!")
            root.close()
        }
        onChannelOpeningFailed: {
            showBubblePopup("Failed to fund channel: %1!".arg(errorMessage))
            root.close()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 30
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        anchors.bottomMargin: 40
        spacing: 20

        XSNLabel {
            Layout.preferredWidth: 380
            font.family: regularFont.name
            font.pixelSize: 14
            textFormat: Text.RichText
            color: SkinColors.secondaryText
            wrapMode: Text.WordWrap
            text: 'You are about to move <font color="%1">%2 %3 (%4 %5)</font> on chain to off chain and you will need to wait
for <font color="%1">%7 confirmations (~ %8 minutes )</font>.'.arg(SkinColors.mainText).arg
            (channelCapacity).arg(assetSymbol).arg(capacityInCurrency.charAt
            (0) === '<' ? " ~ 0.01" : capacityInCurrency).arg(localCurrency).arg
            (confirmationsForApproved).arg(estimatedTimeForChannelOpening(assetID))
        }

            XSNLabel {
                font.family: regularFont.name
                font.pixelSize: 14
                horizontalAlignment: Text.AlignLeft
                color: SkinColors.secondaryText
                text: "Please confirm to continue"
            }

            NetworkFeeRateView {
                id: networkFeeRateView
                Layout.fillWidth: true
                feeRate: root.networkFeeRate
                onCurrentOptionFeeChanged: {
                    root.networkFeeRate = calculateNetworkFeeRate(
                                currentOption, recommendedNetworkFeeRate)
                }
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                Layout.fillHeight: true
                spacing: 5

                Item {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }

                SecondaryButton {
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 70
                    font.pixelSize: 12
                    borderColor: SkinColors.popupFieldBorder
                    hoveredBorderColor: SkinColors.headerText
                    activeStateColor: SkinColors.popupFieldBorder
                    font.capitalization: Font.MixedCase
                    text: "Cancel"

                    onClicked: {
                        openChannelCanceled()
                        root.close()
                    }
                }

                IntroButton {
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 70
                    font.pixelSize: 12
                    font.capitalization: Font.MixedCase
                    text: "OK"
                    onClicked: {
                        if(paymentNodeViewModel.stateModel.nodeType ===  Enums.PaymentNodeType.Lnd ) {
                            openChannelViewModel.createOpenChannelRequest(
                                        paymentNodeViewModel.hostModel.data(
                                            paymentNodeViewModel.hostModel.index(
                                                0, 0)),
                                        channelCapacity,
                                        networkFeeRate)
                        }
                        else {
                            if(paymentNodeViewModel.channelsModel.count > 0) {
                            // ToDo: consider all channels , add check for deposit to hub
                                paymentNodeViewModel.depositChannel(channelCapacity,
                                                          paymentNodeViewModel.channelsModel.getByIndex(0).channelID)
                            }
                            else {
                                 showBubblePopup("No channels to hub found, please open a channel first")
                            }
                        }
                    }
                }
            }
        }

        function estimatedTimeForChannelOpening(assetID) {
            switch (assetID) {
            case 0:
                return "20-30"
            case 384:
                return "3-5"
            case 2:
                return "3-5"
            default:
                return "2-5"
            }
        }
    }
