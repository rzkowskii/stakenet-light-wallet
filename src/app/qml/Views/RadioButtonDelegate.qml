import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0

import "../Components"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Item {
    id: root
    property string name: ""
    property string description: ""
    property var radioGroup: undefined
    property alias radioButton : radioBtn

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 17
            
            Item {
                Layout.fillHeight: true
                Layout.preferredWidth: 40
                
                CustomRadioButton {
                    id: radioBtn
                    height: 18
                    width: 18
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.topMargin: 15
                    ButtonGroup.group: root.radioGroup
                }
            }
            
            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                spacing: 5
                
                XSNLabel {
                    Layout.preferredHeight: 20
                    Layout.topMargin: 10
                    Layout.fillWidth: true
                    text: root.name
                    font.weight: Font.Medium
                }
                
                XSNLabel {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    text: root.description
                    color: SkinColors.secondaryText
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    Layout.alignment: Qt.AlignVCenter
                    Layout.maximumWidth: 700
                }
            }
        }
        
        Rectangle {
            Layout.preferredHeight: 1
            Layout.fillWidth: true
            Layout.rightMargin: 5
            Layout.leftMargin: 5
            color: SkinColors.popupFieldBorder
            opacity: 0.5
        }
    }
    
    PointingCursorMouseArea {
        onClicked: radioBtn.clicked()
    }
}
