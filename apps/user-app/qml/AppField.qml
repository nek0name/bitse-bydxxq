import QtQuick
import QtQuick.Controls

TextField {
  id: control
  implicitHeight: 56
  font.pixelSize: Theme.bodyLargeSize
  color: enabled ? Theme.ink : Theme.disabledText
  placeholderTextColor: Theme.muted
  leftPadding: Theme.cardPadding
  rightPadding: Theme.cardPadding
  selectByMouse: true
  selectionColor: Theme.primaryLight
  selectedTextColor: Theme.ink
  Accessible.name: placeholderText
  background: Rectangle {
    color: control.enabled ? Theme.card : Theme.disabled
    radius: 8
    Rectangle {
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.bottom: parent.bottom
      height: 2
      color: Theme.primary
      visible: control.activeFocus
    }
  }
}
