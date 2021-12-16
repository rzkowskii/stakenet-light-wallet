import QtQuick.Controls 2.5
import QtQuick 2.12
import QtQuick.Layouts 1.3

import com.xsn.utils 1.0

Dialog {
    id: popUpComponent
    property string popUpText: ""
    property alias dialogBackground: backgroundRoot
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        id: backgroundRoot
        color: SkinColors.popupsBgColor
        border.color: SkinColors.popupFieldBorder
        border.width: 1
    }

    onVisibleChanged: {
        if(!visible) {
            activePopup = undefined;
            destroy();
        }
    }

    Overlay.modal: Rectangle {
        color: "#AA000000"
    }

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    modal: true

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: 400
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: 200
        }
    }
}

