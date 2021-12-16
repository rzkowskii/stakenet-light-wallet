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
        var status = ""
        if(!stateModel){
            return ""
        }

        switch (stateModel.nodeStatus) {
        case ConnextStateModel.ConnextStatus.ConnextActive:
            return "L2 active"
        case ConnextStateModel.ConnextStatus.ConnextNotRunning:
            return "L2 offline"
        case ConnextStateModel.ConnextStatus.ConnextSyncing:
            return "L2 syncing"
        default:
            break
        }
        return ""
    }
    color: {
        if(!stateModel){
            return ""
        }
        switch (stateModel.nodeStatus) {
        case ConnextStateModel.ConnextStatus.ConnextActive:
            return "#42C451"
        case ConnextStateModel.ConnextStatus.ConnextNotRunning:
            return "#A7A8AE"
        case ConnextStateModel.ConnextStatus.ConnextSyncing:
            return "#F19E1E"
        }
        return ""
    }
}
