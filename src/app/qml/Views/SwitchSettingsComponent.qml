import QtQuick 2.6
import QtQuick.Controls 2.2

import com.xsn.utils 1.0

Switch {
    id: control

    property string enabledText: ""
    property string disabledText: ""

    font.pixelSize: 16

    indicator: Rectangle {
        implicitWidth: 48
        implicitHeight: 26
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 13
        color: control.checked ? SkinColors.menuBackgroundGradienRightLine : "#ffffff"
        border.color: control.checked ? SkinColors.menuBackgroundGradienRightLine : "#cccccc"

        Rectangle {
            x: control.checked ? parent.width - width : 0
            width: 26
            height: 26
            radius: 13
            color: control.down ? "#cccccc" : "#ffffff"
            border.color: control.checked ? SkinColors.menuBackgroundGradienRightLine : "#999999"
        }
    }

    contentItem: Text {
        text: control.checked ? enabledText : disabledText
        font: control.font
        opacity: enabled ? 1.0 : 0.3
        color: control.checked  ? SkinColors.transactionItemReceived : SkinColors.transactionItemSent
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }
}
