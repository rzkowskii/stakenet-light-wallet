import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import com.xsn.utils 1.0

Item {
    id: root
    signal menuItemClicked
    property bool isCurrentItem: false
    property string name: ""
    property string imageSource: ""

    property color currentColor: isCurrentItem ? SkinColors.menuItemSelectedText : mouseArea.containsMouse ? SkinColors.mainText : SkinColors.menuItemText

    property alias itemMouseArea: mouseArea

    RoundedRectangle {
        corners.topLeftRadius: 10
        corners.bottomLeftRadius: 10
        width: 3
        height: parent.height
        color: '#447ddc'
        visible: isCurrentItem
    }

    RoundedRectangle {
        anchors.fill: parent
        corners.topLeftRadius: 10
        corners.bottomLeftRadius: 10
        visible: isCurrentItem
        opacity: 0.3

        customGradient: {
            "vertical": false,
            "colors": [{
                           "position": 0.0,
                           "color": SkinColors.walletAssetHighlightColor
                       }, {
                           "position": 1.0,
                           "color": "transparent"
                       }]
        }
    }

    RowLayout {
        anchors {
            fill: parent
            leftMargin: 22
        }
        spacing: 18

        Image {
            source: imageSource
            sourceSize: Qt.size(20, 20)
        }

        XSNLabel {
            FontLoader {
                id: fontRegular
                source: "qrc:/Rubik-Regular.ttf"
            }
            Layout.fillWidth: true
            Layout.alignment: Text.AlignLeft
            text: name
            font.pixelSize: 14
            color: currentColor
            font.family: fontRegular.name
        }
    }

    PointingCursorMouseArea {
        id: mouseArea
        onClicked: menuItemClicked()
    }
}
