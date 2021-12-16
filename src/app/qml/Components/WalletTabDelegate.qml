import QtQuick 2.0
import QtGraphicalEffects 1.0
import QtQuick.Controls 2.2
import "../Components"

import com.xsn.utils 1.0

TabDelegate {
       signal selected()

        Text {
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: modelData
            font.pixelSize: 11
            font.family: fontMedium.name
            font.capitalization: Font.AllUppercase
            color: SkinColors.mainText
        }

        PointingCursorMouseArea {
            id: mouseArea
            onClicked: selected()
        }
    }
