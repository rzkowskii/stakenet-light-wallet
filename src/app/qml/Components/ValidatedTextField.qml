import QtQuick 2.12
import QtQuick.Controls 2.5
import com.xsn.utils 1.0

TextInput {
    id: textInput
    property var customValidator 
    property string placeholderText: ""

    font.family: regularFont.name
    font.pixelSize: 14
    color: SkinColors.mainText

    Label {
        text: textInput.placeholderText
        anchors.centerIn: parent.Center
        visible: textInput.text.length === 0
        color: SkinColors.comboboxIndicatorColor
    }

    QtObject {
        id: internal
        property string mementoText: ""

        function saveState() {
            mementoText = textInput.text;
        }

        function restoreState() {
            textInput.text = mementoText;
        }
    }

    onTextChanged: {
        if(internal.mementoText === textInput.text) {
            return;
        }

        if(internal.mementoText.length < textInput.text.length) {
            if(customValidator !== undefined && customValidator(textInput.text) === false) {
                internal.restoreState();
                return;
            }
        }

        internal.saveState();
    }
}
