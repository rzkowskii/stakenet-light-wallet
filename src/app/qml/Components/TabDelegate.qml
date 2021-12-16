import QtQuick 2.0
import QtGraphicalEffects 1.0
import QtQuick.Controls 2.2
import "../Components"

import com.xsn.utils 1.0

Canvas {
        id: canvas
        contextType: "2d"

        property bool isCurrentTab
        onIsCurrentTabChanged: requestPaint()

        onPaint: {
            context.reset();

            context.strokeStyle = SkinColors.delegatesBackgroundLightColor
            context.lineWidth = 1

            context.moveTo(10, 0);
            context.arcTo(0, 0, 0, height, 10);
            context.lineTo(0, height);
            context.lineTo(width, height);
            context.lineTo(width - 33, 8)
            context.arcTo(width - 40, 0, 10, 0, 16)
            context.stroke()

            var gradient = context.createLinearGradient(0, 0, 0, height)
            gradient.addColorStop(0.0, !isCurrentTab ? SkinColors.delegatesBackgroundLightColor : SkinColors.walletPageBackgroundLightColor)
            gradient.addColorStop(1.0, !isCurrentTab ? SkinColors.delegatesBackgroundDarkColor : SkinColors.botTextFieldActiveBorderColor)

            context.fillStyle = gradient

            context.closePath();
            context.fill();
        }
}
