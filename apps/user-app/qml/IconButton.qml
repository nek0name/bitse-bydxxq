import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
  id: control
  property string iconName: ''
  property string label: ''
  implicitWidth: Theme.touchSize
  implicitHeight: Theme.touchSize
  Layout.minimumWidth: Theme.touchSize
  Layout.minimumHeight: Theme.touchSize
  leftPadding: 12
  rightPadding: 12
  topPadding: 12
  bottomPadding: 12
  leftInset: 0
  rightInset: 0
  topInset: 0
  bottomInset: 0
  focusPolicy: Qt.StrongFocus
  Accessible.name: label
  Accessible.onPressAction: clicked()
  contentItem: Item {
    implicitWidth: Theme.iconSize
    implicitHeight: Theme.iconSize
    AppIcon {
      anchors.centerIn: parent
      name: control.iconName
      opacity: control.enabled ? 1 : 0.4
    }
  }
  background: Rectangle {
    radius: Theme.heroRadius
    color: !control.enabled ? 'transparent' : control.down ? Theme.primarySoftPressed : (control.hovered || control.visualFocus) ? Theme.primaryLight : 'transparent'
  }
}
