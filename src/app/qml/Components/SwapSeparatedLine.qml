import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.15

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0
import com.xsn.models 1.0

Item {
    property bool isHorizontal: true

    LinearGradient {
        anchors.fill: parent
        opacity: 0.3
        start: Qt.point(0, 0)
        end: Qt.point(isHorizontal ? width : 0, isHorizontal ? 0 : height)
        gradient: Gradient {
            GradientStop {
                position: 0.95
                color:  "transparent"
            }
            GradientStop {
                position: 0.7
                color: SkinColors.secondaryText
            }
            GradientStop {
                position: 0.5
                color: "white"
            }
            GradientStop {
                position: 0.3
                color: SkinColors.secondaryText
            }
            GradientStop {
                position: 0.05
                color: "transparent"
            }
        }
    }
}
