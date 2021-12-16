import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

Canvas {
    id: control
    property color color

    onColorChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d");
        ctx.save();
        ctx.strokeStyle = control.color;
        ctx.lineWidth = 1;

        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(2.5, 2.5);
        ctx.lineTo(0, 5);

        ctx.stroke();
    }

}

