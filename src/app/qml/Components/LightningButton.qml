import QtQuick 2.12
import QtQuick.Controls 2.2

import com.xsn.utils 1.0

Button {
    id: control
    property string color: ""
    checkable: true
    font.pixelSize: 12

    contentItem: XSNLabel {
        text: control.text
        font: control.font
        opacity: enabled ? 1.0 : 0.3
        color: control.checked ? parent.color : SkinColors.mainText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        implicitWidth: 45
        implicitHeight: 30
        color: "transparent"
        opacity: enabled ? 1 : 0.3
        radius: 5
    }
}
