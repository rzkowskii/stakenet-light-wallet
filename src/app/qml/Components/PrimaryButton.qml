import QtQuick 2.12
import QtQuick.Controls 2.2

import com.xsn.utils 1.0

Button {
    id: control
    contentItem: XSNLabel {
        text: control.text
        font: control.font
        color: SkinColors.mainText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        color: "#2C80FF"
        radius: 5
        opacity: enabled ? 1.0 : 0.4
    }
}
