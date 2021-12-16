import QtQuick 2.12
import QtQuick.Layouts 1.3

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

RowLayout {
    property PaymentNodeViewModel lightningView
    property alias status: title
    property alias activeImage: lndActiveImg
    spacing: 10

    AnimatedImage {
        id: lndActiveImg
        Layout.preferredWidth: 30
        Layout.preferredHeight: 30
        source: apLndStatusImage()
        onStatusChanged: playing = (status == AnimatedImage.Ready)
    }

    XSNLabel {
        id: title
        Layout.preferredHeight: text.length > 0 ? 40 : 0
        Layout.alignment: Qt.AlignLeft
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 16
        text: {
            switch(lightningView.stateModel.autopilotStatus) {
            case LnDaemonStateModel.AutopilotStatus.AutopilotActive: return "Autopilot active";
            case LnDaemonStateModel.AutopilotStatus.AutopilotDisabled: return "Autopilot disabled";
            case LnDaemonStateModel.AutopilotStatus.AutopilotNoPeers: return "Autopilot waiting for peers";
            case LnDaemonStateModel.AutopilotStatus.AutopilotNotEnoughCoins: return "Autopilot insufficient balance";
            case LnDaemonStateModel.AutopilotStatus.AutopilotChainSyncing: return "Autopilot chain syncing";
            case LnDaemonStateModel.AutopilotStatus.AutopilotGraphSyncing: return "Autopilot graph syncing";
            default:
                break;
            }

            switch(lightningView.stateModel.nodeStatus) {
            case LnDaemonStateModel.LndStatus.LndActive: return "Lightning active";
            case LnDaemonStateModel.LndStatus.LndChainSyncing: return "L2 chain syncing";
            case LnDaemonStateModel.LndStatus.LndGraphSyncing: return "L2 graph syncing";
            case LnDaemonStateModel.LndStatus.WaitingForPeers: status = "L2 waiting for peers"; break;
            case LnDaemonStateModel.LndStatus.LndNotRunning: return "L2 is not running";
            }

            return "";
        }

        color: {
            switch(lightningView.stateModel.autopilotStatus) {
            case LnDaemonStateModel.AutopilotStatus.AutopilotActive: return "#00F8FD";
            case LnDaemonStateModel.AutopilotStatus.AutopilotNoPeers:
            case LnDaemonStateModel.AutopilotStatus.AutopilotNotEnoughCoins:
            case LnDaemonStateModel.AutopilotStatus.AutopilotDisabled: return "#6F727C";
            case LnDaemonStateModel.AutopilotStatus.AutopilotGraphSyncing:
            case LnDaemonStateModel.AutopilotStatus.AutopilotChainSyncing: return SkinColors.mainText;
            }


            switch(lightningView.stateModel.nodeStatus) {
            case LnDaemonStateModel.LndStatus.LndActive: return "#00F8FD";
            case LnDaemonStateModel.LndStatus.LndChainSyncing:
            case LnDaemonStateModel.LndStatus.LndGraphSyncing: return SkinColors.mainText;
            case LnDaemonStateModel.LndStatus.WaitingForPeers: return SkinColors.mainText;
            case LnDaemonStateModel.LndStatus.LndNotRunning: return "#6F727C";
            }

            return "transparent";
        }
    }


    function apLndStatusImage() {
        switch(lightningView.stateModel.autopilotStatus) {
        case LnDaemonStateModel.AutopilotStatus.AutopilotActive: return "qrc:/images/LN-ACTIVE.gif";
        case LnDaemonStateModel.AutopilotStatus.AutopilotDisabled: return "qrc:/images/inactive_static.png";
        case LnDaemonStateModel.AutopilotStatus.AutopilotNoPeers: return "qrc:/images/inactive_static.png";
        case LnDaemonStateModel.AutopilotStatus.AutopilotNotEnoughCoins:return "qrc:/images/inactive_static.png";
        case LnDaemonStateModel.AutopilotStatus.AutopilotChainSyncing: return "qrc:/images/ACTIVE-CH-LN-SYNC-LOADING.gif";
        case LnDaemonStateModel.AutopilotStatus.AutopilotGraphSyncing: return "qrc:/images/ACTIVE-CH-LN-SYNC-LOADING.gif";
        default:
            break;
        }

        switch(lightningView.stateModel.nodeStatus) {
        case LnDaemonStateModel.LndStatus.LndActive: return "qrc:/images/LN-ACTIVE.gif";
        case LnDaemonStateModel.LndStatus.LndChainSyncing: return "qrc:/images/ACTIVE-CH-LN-SYNC-LOADING.gif";
        case LnDaemonStateModel.LndStatus.LndGraphSyncing: return "qrc:/images/ACTIVE-CH-LN-SYNC-LOADING.gif";
        case LnDaemonStateModel.LndStatus.WaitingForPeers: return "qrc:/images/ACTIVE-CH-LN-SYNC-LOADING.gif";
        case LnDaemonStateModel.LndStatus.LndNotRunning: return "qrc:/images/inactive_static.png";
        }

        return false;
    }
}
