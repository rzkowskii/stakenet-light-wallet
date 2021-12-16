import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.0

import Popups 1.0
import Pages 1.0
import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

import "../Components"
import "../Views"
import "../Popups"

Slider {
    id: control
    snapMode: Slider.SnapOnRelease
    property int assetID: -1
    property string assetName: ""
    property string assetSymbol: ""
    property double assetMinPayNodeCapacity: 0.0
    property real assetTotalBalance
    property real assetOnChainBalance
    property real assetNodeBalance
    property int assetConfirmationsForApproved: -1
    property PaymentNodeViewModel payNodeViewModel
    property string valueSymbol: ""

    property bool tooltipVisibility

    property var channCapacity

    property real lastValue: 0

    enabled: payNodeViewModel.stateModel && checkPaymentNodeState(
                 payNodeViewModel.stateModel) && assetTotalBalance != 0.0

    onAssetTotalBalanceChanged: {

        if (payNodeViewModel.stateModel) {
            refreshSliderBalance()
        }
    }

    onAssetNodeBalanceChanged: {
        refreshSliderBalance()
    }

    onHoveredChanged: {
        if (!control.enabled) {

        }
    }

    FontLoader {
        id: regularFont
        source: "qrc:/Rubik-Regular.ttf"
    }

    WalletAssetsListModel {
        id: assetModel
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
    }

    OpenChannelViewModel {
        id: openChannelViewModel
        currentAssetID: assetID
        Component.onCompleted: {
            initialize(ApplicationViewModel)
        }
        onChannelOpened: {
            showBubblePopup("Channel was successfully opened!")
        }
        onChannelOpeningFailed: {
            refreshSliderBalance()
            showBubblePopup("Failed to open channel: %1!".arg(errorMessage))
        }
        onRequestCreatingFailed: {
            refreshSliderBalance()
            showBubblePopup("Failed to open channel: %1!".arg(errorMessage))
        }
    }

    onMoved: {

        if (assetOnChainBalance < assetMinPayNodeCapacity) {
            control.value = lastValue
            showBubblePopup(
                        "Your onchain balance is insufficient to open channel.")
        } else {

            if (lastValue > control.value) {
                control.value = lastValue
                showBubblePopup(
                            "Lightning channels can't be closed using slider. You can only close channel at the Channels tab.")
            }
        }
    }

    onPressedChanged: {

        if (!pressed) {

            if (lastValue != control.value) {

                var lnPer = calculateBalancePercentage(assetMinPayNodeCapacity,
                                                       assetTotalBalance, 1)

                var channelCapacity

                if (control.value - lastValue < lnPer) {

                    channelCapacity = Utils.convertSatoshiToCoin(
                                assetMinPayNodeCapacity)
                    control.value = lastValue + lnPer
                    lastValue = control.value
                } else {

                    var totalBalanceChannelCapacity = ((control.value - lastValue)
                                                       * Utils.formatBalance(
                                                           assetTotalBalance)) / 100

                    channelCapacity = control.value
                            == 100 ? Utils.formatBalance(
                                         assetOnChainBalance) : totalBalanceChannelCapacity
                }

                if (checkPaymentNodeState(payNodeViewModel.stateModel)) {
                    channCapacity = channelCapacity
                } else {
                    lastValue = control.value
                }
            }
        } else {

            //            lastValue = control.value
        }
    }

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 320
        implicitHeight: 2
        width: control.availableWidth
        height: implicitHeight
        radius: 2
        color: SkinColors.secondaryText

        Rectangle {
            width: control.visualPosition * parent.width
            height: 12
            y: control.topPadding - control.availableHeight + 6
            color: "#0047ff"
            radius: 2
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 24
        implicitHeight: 24
        radius: 12
        color: control.pressed ? "#f0f0f0" : "#f6f6f6"
        border.color: "#bdbebf"
    }

    function refreshSliderBalance() {
        control.value = calculateBalancePercentage(
                    payNodeViewModel.stateModel.nodeBalance,
                    assetTotalBalance, 1)
        control.lastValue = calculateBalancePercentage(
                    payNodeViewModel.stateModel.nodeBalance,
                    assetTotalBalance, 1)
    }
}
