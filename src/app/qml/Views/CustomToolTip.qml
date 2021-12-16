import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../Components"
import "../Popups"

import com.xsn.viewmodels 1.0
import com.xsn.utils 1.0

ToolTip {
    id: control
    delay: 200
    y: -45
    property alias contentItemText: content
    property int tailPosition: 0

    FontLoader { id: localFont; source: "qrc:/Rubik-Light.ttf" }

    contentItem: Text {
        id: content
        bottomPadding: 9
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: control.text
        font.family: localFont.name
        font.pixelSize: 14
        color: SkinColors.mainText
    }
    enter: Transition {
        NumberAnimation { property: "opacity"; duration: 400; to: 1}
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; duration: 350; to: 0}
    }
    
    background: Canvas {
        id: canvas
        
        Connections {
            target: parent
            function onVisibleChanged() {
                canvas.requestPaint()
            }
        }
        
        onPaint: {
            var context = getContext("2d");
            context.reset();
            context.moveTo(0, 0);
            context.lineTo(width, 0);
            context.lineTo(width, height - 9);
            context.lineTo(tailPosition + 7 , height - 9);
            context.lineTo(tailPosition, height);
            context.lineTo(tailPosition - 7 , height - 9);
            context.lineTo(0, height - 9);
            context.closePath();
            context.fillStyle = SkinColors.secondaryBackground;
            context.strokeStyle = SkinColors.popupFieldBorder;
            context.fill();
            context.stroke();
        }
    }
}
