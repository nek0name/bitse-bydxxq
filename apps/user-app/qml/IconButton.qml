import QtQuick
import QtQuick.Controls

Button {
  id: control
  property string iconName: ''
  property string label: ''
  implicitWidth: Theme.touchSize
  implicitHeight: Theme.touchSize
  padding: 12
  focusPolicy: Qt.StrongFocus
  Accessible.name: label
  Accessible.onPressAction: clicked()
  contentItem: AppIcon {
    name: control.iconName
    opacity: control.enabled ? 1 : 0.4
  }
  background: Rectangle {
    radius: Theme.heroRadius
    color: !control.enabled ? 'transparent' : control.down ? Theme.primarySoftPressed : (control.hovered || control.visualFocus) ? Theme.primaryLight : 'transparent'
  }
}
