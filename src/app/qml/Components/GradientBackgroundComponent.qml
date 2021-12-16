import QtQuick 2.15
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.15
import QtQuick.Controls.Styles 1.4

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import com.xsn.models 1.0

Rectangle {
    id: headerBackground

    property string firstColor: ""
    property string secondColor: ""
    property string thirdColor: ""
    property alias gradientOpacity: grad.opacity
    property alias gradientSourceRadius: gradientSource.radius

    color: "transparent"
    radius: 10

    LinearGradient {
        id: grad
        anchors.fill: parent
        start: Qt.point(0, 0)
        end: Qt.point(width, 0)
        source: Rectangle {
            id: gradientSource
            width: headerBackground.width
            height: headerBackground.height
            color: "white"
        }

        gradient: Gradient {
            GradientStop {
                position: 1.0
                color: thirdColor
            }
            GradientStop {
                position: 0.5
                color: secondColor
            }
            GradientStop {
                position: 0.0
                color: firstColor
            }
        }
    }
}
