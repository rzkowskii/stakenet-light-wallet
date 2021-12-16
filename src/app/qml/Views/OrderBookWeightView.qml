import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"

import com.xsn.models 1.0
import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

Item {
    id: root
    property alias firstItem: first
    property alias firstItemUnits: firstUnits
    property alias secondItem: second
    property alias secondtemUnits: secondUnits
    property string color: ""

    RowLayout {
        anchors.fill: parent
        anchors.rightMargin: 15
        anchors.leftMargin: 15
        spacing: 0
        
        Item {
            Layout.preferredWidth: parent.width * 0.25
            Layout.fillHeight: true
            
            Text {
                id: sum
                anchors.verticalCenter: parent.verticalCenter
                text: "Sum:"
                font.family: fontRegular.name
                font.pixelSize: 10
                color: root.color
            }
        }
        
        Item {
            Layout.preferredWidth: parent.width * 0.25
            Layout.fillHeight: true
            
            RowLayout {
                anchors.fill: parent
                anchors.right: parent.right
                spacing: 3

                Text {
                    id: first
                    Layout.fillWidth: true
                    color: SkinColors.secondaryText
                    font.family: fontRegular.name
                    font.pixelSize: 10
                    horizontalAlignment: Text.AlignRight
                }

                Text {
                    id: firstUnits
                    Layout.alignment: Qt.AlignRight
                    font.family: fontRegular.name
                    font.pixelSize: 10
                    font.capitalization: Font.AllUppercase
                    color: root.color
                }
            }
        }

        Item {
            Layout.preferredWidth: parent.width * 0.25
            Layout.fillHeight: true

            RowLayout {
                anchors.fill: parent
                anchors.right: parent.right
                spacing: 3

                Text {
                    id: second
                    Layout.fillWidth: true
                    color: SkinColors.secondaryText
                    font.family: fontRegular.name
                    font.pixelSize: 10
                    horizontalAlignment: Text.AlignRight
                }

                Text {
                    id: secondUnits
                    Layout.alignment: Qt.AlignRight
                    font.family: fontRegular.name
                    font.pixelSize: 10
                    font.capitalization: Font.AllUppercase
                    color: root.color
                }
            }
        }
        
        Item {
            Layout.preferredWidth: parent.width * 0.25
            Layout.fillHeight: true
        }
    }
}
