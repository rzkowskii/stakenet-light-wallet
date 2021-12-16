import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

import "../Components"
import "../Views"
import "../Popups"

Page {
    id: root
    property int assetID
    property string assetName
    property var openInExplorer
    property LightningViewModel lnViewModel
    property int currentIndex: -1
    property string symbol: assetModel.get(currentIndex).symbol
    property int confirmationsForChannelApproved: assetModel.get(currentIndex).confirmationsForChannelApproved

    FontLoader { id: mediumFont; source: "qrc:/Rubik-Medium.ttf" }
    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }

    LightningChannelsListModel {
        id: channelsModel
        currentAssetID: assetID
        Component.onCompleted: {
            initialize(assetID, ApplicationViewModel)
        }
    }

    Connections {
        target: lnViewModel

        function onChannelOpened() {
            channelsModel.refresh();
        }

        function onChannelClosed() {
            channelsModel.refresh();
        }

        function onAllChannelsClosed() {
            channelsModel.refresh()
        }
    }

    Connections  {
        target: ApplicationViewModel.lndViewModel

        function onNoFreeSlots(lnds) {
            var popup = openDialog(chooseLightningComponent, {lightningsList : lnds, assetModel : assetModel, assetID : root.assetID,  assetName: root.assetName});

        }

        function onLndActivityChanged() {
            lightningChannelsView.isActiveLightning = true;
        }
    }

    Component {
        id: closeChannelDialogComponent
        CloseChannelPopup {

        }
    }

    Component {
        id: openChannelDialogComponent
        OpenChannelPopup {

        }
    }

    Component {
        id: chooseLightningComponent
        ChooseLightningPopup {

        }
    }

    Component {
        id: lnBackupsDialogComponent
        LightningBackupPopup {

        }
    }

    Component {
        id: channelDetailsComponent
        ChannelDetailsPopup {

        }
    }

    Component {
        id: rentalChannelDialogComponent
        RentChannelPopup {

        }
    }

    Component {
        id: apSettingsComponent
        APSettingsPopup {
        }
    }

    Component {
        id: extenderRentalComponent
        ExtenderRentalPopup {
        }
    }

    Component {
        id: rentalNotificationComponent

        ConfirmationPopup {
            width: 480
            height: 280
            cancelButton.visible: false
            confirmButton.text: "Ok"
            onConfirmClicked: {
                close();
            }
        }
    }

    WalletAssetsListModel {
        id: assetModel
        Component.onCompleted: {
            initialize(ApplicationViewModel);
            root.currentIndex = getInitial(assetID)
        }
    }

    background: Rectangle {
        color: SkinColors.mainBackground
    }

    LightningChannelsView {
        id: lightningChannelsView
        anchors.fill: parent
        anchors.rightMargin: 60
        currentSymbol: symbol
        currentAssetID: assetID
        currentAssetName: assetModel.get(currentIndex).name
        currentConfirmationsForChannelApproved: confirmationsForChannelApproved
    }
}
