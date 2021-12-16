import QtQuick 2.15
import "../Components"
import "../Views"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Rectangle {
    property var nodeStatus

    property PaymentNodeViewModel payNodeViewModel
    radius: height / 2
    color: {
        if(payNodeViewModel.stateModel)
            return payNodeViewModel.stateModel.nodeStatusColor
        else
            return "#A7A8AE"
    }
    Rectangle {
        anchors.centerIn: parent
        height: parent.height * 2
        width: height
        radius: height / 2
        color: parent.color
        opacity: 0.3
    }
}
