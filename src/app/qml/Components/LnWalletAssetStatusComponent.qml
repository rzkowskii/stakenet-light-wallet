import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import "../Components"
import com.xsn.utils 1.0
import com.xsn.viewmodels 1.0
import com.xsn.models 1.0

Text {
    FontLoader {
        id: fontRegular
        source: "qrc:/Rubik-Regular.ttf"
    }

    font.family: fontRegular.name
    font.pixelSize: 11
    font.capitalization: Font.AllUppercase
    text: {
        if(!stateModel){
            return ""
        }
        var isLndPendingChannel = stateModel.isPendingChannel
        var status = ""

        switch (stateModel.nodeStatus) {
        case LnDaemonStateModel.LndStatus.LndActive:
            status = "L2 active"
            break
        case LnDaemonStateModel.LndStatus.LndNotRunning:
            status = "L2 offline"
            break
        case LnDaemonStateModel.LndStatus.LndChainSyncing:
            status = "L2 syncing"
            break
        case LnDaemonStateModel.LndStatus.LndGraphSyncing:
            status = "L2 syncing"
            break
        case LnDaemonStateModel.LndStatus.WaitingForPeers:
            status = "L2 waiting for peers"
            break
        default:
            break
        }
        return "%1 %2".arg(status).arg(
                    isLndPendingChannel
                    && channelModel.recentPendingConfirmation
                    <= confirmationsForApproved ? " %1/%2".arg(
                                                      channelModel.recentPendingConfirmation).arg(
                                                      confirmationsForApproved) : "")
    }
    color: {
        if(!stateModel){
            return ""
        }
        switch (stateModel.nodeStatus) {
        case LnDaemonStateModel.LndStatus.LndActive:
            return "#42C451"
        case LnDaemonStateModel.LndStatus.LndNotRunning:
            return "#A7A8AE"
        case LnDaemonStateModel.LndStatus.LndChainSyncing:
            return "#F19E1E"
        case LnDaemonStateModel.LndStatus.LndGraphSyncing:
            return "#F19E1E"
        case LnDaemonStateModel.LndStatus.WaitingForPeers:
            return "#F19E1E"
        }
        return ""
    }
}
