import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QtQuick.Controls.Styles 1.4

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Item {
    id: root
    property string assetName: ""

    Rectangle {
        id: sourceRectangle
        anchors.fill: parent
        color: "transparent"
        radius: 10

        LinearGradient {
            anchors.fill: parent
            start: Qt.point(0, 0)
            end: Qt.point(width, 0)

            source: Rectangle {
                width: sourceRectangle.width
                height: sourceRectangle.height
                color: "white"
                radius: 10
            }

            gradient: Gradient {
                GradientStop {
                    position: 1.0
                    color: SkinColors.botAssetImageBackground
                }
                GradientStop {
                    position: 0.5
                    color: SkinColors.botAssetImageBackground
                }
                GradientStop {
                    position: 0.0
                    color: "transparent"
                }
            }
        }
    }

    GlowImage {
        height: 0.55 * parent.width
        width: 0.5 * parent.width
        anchors.centerIn: parent
        source: assetName !== "" ? "qrc:/images/ICON_%1.svg".arg(assetName) : ""
        sourceSize: Qt.size(0.5 * parent.width, 0.55 * parent.width)
        color: "white"
    }
}
