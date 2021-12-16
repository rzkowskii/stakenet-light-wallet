import QtQuick 2.12
import QtQuick.Controls 2.2

import com.xsn.utils 1.0

Switch {
    id: control
    width: 33
    height: 16

    indicator: Rectangle {
        implicitWidth: 33
        implicitHeight: 16
        y: parent.height / 2 - height / 2
        radius: 8
        color: control.checked ? SkinColors.mobileActiveSwitchBackground : SkinColors.mobileDisactiveSwitchBackground

        Rectangle {
            x: control.checked ? parent.width - width : 0
            y: (parent.height - height) / 2
            width: 17
            height: 17
            radius: 13
            color: control.checked ? SkinColors.switchActiveIndicatorColor : SkinColors.mobileDisactiveSwitchIndicator
            border.color: control.checked ? SkinColors.switchActiveIndicatorBorderColor : SkinColors.mobileDisactiveSwitchIndicator
        }
    }

    PointingCursorMouseArea {
        anchors.fill: parent
        onClicked: {
            control.clicked()
            checked = !checked;
        }
    }
}
