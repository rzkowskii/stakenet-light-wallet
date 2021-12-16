import QtQuick 2.12
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../Components"
import com.xsn.viewmodels 1.0
import com.xsn.models 1.0

ColumnLayout {
    spacing: 20

    Column {
        Layout.fillWidth: true
        Layout.preferredHeight: 50
        spacing: 10

        XSNLabel {
            text: "Currency"
            font.pixelSize: 25
        }

        XSNLabel {
            text: "Set your preferred local currency"
            font.pixelSize: 16
        }
    }

    CurrencyComboBox {
        id: currencyComboBox
        Layout.preferredHeight: 40
        Layout.preferredWidth: 300

        model: CurrencyModel {
            id: currencyModel
            Component.onCompleted: initialize(ApplicationViewModel)
            onModelReset: {
                currencyComboBox.currentIndex = getInitial(ApplicationViewModel.localCurrencyViewModel.currentCurrencyCode);
            }
        }
        onCurrentIndexChanged: ApplicationViewModel.localCurrencyViewModel.changeLocalCurrency(currencyModel.getCode(currentIndex))
    }
}
