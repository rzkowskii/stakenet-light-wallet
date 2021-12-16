import QtQuick 2.12
import QtQuick.Controls 2.4

TextField {
    id: root
    wrapMode: Text.WordWrap
    property int maxWordCount: 24


    QtObject {
        id: internal
        property string oldText: root.text

        function endsWithSpace(str) {
            return str.length > 0 && str[str.length - 1] === ' ';
        }

        function restoreOldState() {
            root.text = internal.oldText;
            root.cursorPosition = text.length;
        }
        function isAtEnd(str) {
            return str.split(' ').length > maxWordCount;
        }
    }

    onTextChanged: {
        if(internal.oldText === text) {
            return;
        }

        if(internal.endsWithSpace(text) && internal.endsWithSpace(internal.oldText)) {
            internal.restoreOldState();
        }

        if(internal.isAtEnd(text)) {
            var newText = text.substring(0, text.length - 1);
            var isValid = internal.endsWithSpace(text) && !internal.isAtEnd(newText);
            if(isValid) {
                internal.oldText = newText;
            }

            internal.restoreOldState();
        }

        internal.oldText = text;
    }
}
