import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0

import "../Views"
import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.models 1.0
import com.xsn.utils 1.0

Slider {
    id: control
    snapMode: Slider.SnapAlways
    property string valueSymbol: ""
    property bool valueSymbolBefore: false
    property string secondValueSymbol: ""
    property int secondStep: 0

    FontLoader {
        id: regularFont
        source: "qrc:/Rubik-Regular.ttf"
    }

    background: Item {
        implicitWidth: 320
        implicitHeight: 24
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: implicitHeight

        Rectangle {
            anchors.fill: parent
            color: SkinColors.botSliderBackground
            opacity: 0.15
            radius: 12
        }

        Rectangle {
            x: control.leftPadding + 0.5 * (control.availableWidth - cusHandle.width)
            anchors.topMargin: -15
            anchors.top: parent.top
            width: 2
            height: 56
            color: "transparent"

            LinearGradient {
                anchors.fill: parent
                height: 12
                width: 400
                start: Qt.point(0, 0)
                end: Qt.point(0, height)
                gradient: Gradient {
                    GradientStop {
                        position: 1.0
                        color: "transparent"
                    }

                    GradientStop {
                        position: 0.61
                        color: "white"
                    }

                    GradientStop {
                        position: 0.6
                        color: "transparent"
                    }

                    GradientStop {
                        position: 0.36
                        color: "transparent"
                    }

                    GradientStop {
                        position: 0.35
                        color: "white"
                    }

                    GradientStop {
                        position: 0.0
                        color: "transparent"
                    }
                }
            }
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 7
            anchors.left: parent.left
            anchors.rightMargin: 7
            anchors.right: parent.right
            width: parent.width - 7
            height: 1
            color: SkinColors.mainText

            Item {
                anchors.left: parent.left
                anchors.leftMargin: -1
                anchors.verticalCenter: parent.verticalCenter
                width: control.value / 100 * parent.width
                height: 12

                LinearGradient {
                    anchors.fill: parent
                    start: Qt.point(0, 0)
                    end: Qt.point(width, 0)
                    source: Rectangle {
                        width: control.visualPosition * control.width
                        height: 12
                        radius: 6
                    }
                    gradient: Gradient {
                        GradientStop {
                            position: 1.0
                            color: SkinColors.botSliderBackgroundColorSecond
                        }
                        GradientStop {
                            position: 0.0
                            color: SkinColors.botSliderBackgroundColorFirst
                        }
                    }
                }
            }
        }
    }

    handle: Item {
        id: cusHandle
        x: control.leftPadding + control.value / 100 * (control.availableWidth - width) - 5
        y: control.topPadding + control.availableHeight / 2 - circle.height / 2
        implicitWidth: 24
        implicitHeight: 24

        Rectangle {
            id: circle
            implicitWidth: 24
            implicitHeight: 24
            width: control.height
            height: control.height
            radius: control.height / 2
            color: control.pressed ? SkinColors.botSliderHandlePressedColor : SkinColors.botSliderHandleInactiveColor
            border.color: SkinColors.botSliderHandleBorderColor

            RowLayout {
                anchors.centerIn: parent
                spacing: 3

                Rectangle {
                    Layout.preferredHeight: 8
                    Layout.preferredWidth: 1
                    color: SkinColors.botSliderHandleLines
                }

                Rectangle {
                    Layout.preferredHeight: 12
                    Layout.preferredWidth: 1
                    color: SkinColors.botSliderHandleLines
                }

                Rectangle {
                    Layout.preferredHeight: 8
                    Layout.preferredWidth: 1
                    color: SkinColors.botSliderHandleLines
                }
            }
        }
    }
}
