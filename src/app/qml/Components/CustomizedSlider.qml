import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

import "../Components"

import com.xsn.utils 1.0

Slider {
    id: control
    snapMode: Slider.SnapAlways
    property alias startText: startLabel.text
    property alias finishText: finishLabel.text
    property string valueSymbol: ""
    property bool valueSymbolBefore: false
    property string secondValueSymbol: ""
    property int secondStep: 0

    FontLoader { id: regularFont; source: "qrc:/Rubik-Regular.ttf" }
    
    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 320
        implicitHeight: 2
        width: control.availableWidth
        height: implicitHeight
        radius: 2
        color: "#818398"
        
        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            color: SkinColors.menuBackgroundGradienRightLine
            radius: 2
        }
        
        XSNLabel {
            id: startLabel
            font.family: regularFont.name
            font.pixelSize: 12
            anchors.top: parent.top
            anchors.right: parent.left
            anchors.rightMargin: -contentWidth / 2
            anchors.topMargin: -contentHeight - 7
            color: SkinColors.secondaryText
            font.capitalization: Font.AllUppercase
        }
        
        XSNLabel {
            id: finishLabel
            font.family: regularFont.name
            font.pixelSize: 12
            anchors.left: parent.right
            anchors.leftMargin: -contentWidth / 2
            anchors.top: parent.top
            anchors.topMargin: -contentHeight - 7
            color: SkinColors.secondaryText
            font.capitalization: Font.AllUppercase
        }
    }
    
    handle: Item {
        id: cusHandle
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - circle.height / 2
        implicitWidth: 18
        implicitHeight: 25
        
        Rectangle {
            id: circle
            width: 18
            height: 18
            radius: 9
            color: control.pressed ? "#f0f0f0" : "#f6f6f6"
            border.color: "#bdbebf"
        }
        
        XSNLabel {
            id: currentValue
            text: valueSymbolBefore ? "%1 %2".arg(control.valueSymbol).arg(control.value)
                                    : (control.secondValueSymbol !== "" ? (Math.floor(control.value / secondStep) > 0 && control.value % secondStep > 0 ? "%1 %2 %3 %4".arg(Math.floor(control.value / secondStep)).arg(control.secondValueSymbol).arg(control.value % secondStep).arg(control.valueSymbol)
                                                                                                                                        : "%1 %2".arg(Math.floor(control.value / secondStep)).arg(control.secondValueSymbol))
                                                                        : "%1 %2".arg(control.value).arg(control.valueSymbol))
            font.pixelSize: 12
            anchors.top: cusHandle.bottom
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
