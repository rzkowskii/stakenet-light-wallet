import QtQuick 2.12
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Button {
    id: actionButton
    property alias btnBackground: fadedRectangle
    signal clicked()
    padding: 15

    background: Item {

        FadedRectangle {
            id: fadedRectangle
            anchors.fill: parent
            activeBorderColor: SkinColors.introBtnHoveredColor
            inactiveBorderColor: SkinColors.introBtnColor
            border.width: 2
            radius: 30
            opacity: actionButton.opacity
        }
    }

    contentItem: XSNLabel {
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        FontLoader { id: localFont; source: "qrc:/Rubik-Medium.ttf" }
        font.family: localFont.name
        color: SkinColors.mainText
        font.pixelSize: actionButton.font.pixelSize
        font.capitalization: Font.AllUppercase
        text: actionButton.text
        opacity: actionButton.opacity
    }

    PointingCursorMouseArea {
        anchors.fill: parent
        onClicked: actionButton.clicked()
        onEntered: {
            fadedRectangle.startFade()
            fadedRectangle.borderStartFade()
        }
        onExited: {
            fadedRectangle.stopFade()
            fadedRectangle.borderStopFade()
        }
    }
}
