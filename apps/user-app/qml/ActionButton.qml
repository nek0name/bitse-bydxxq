import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
  id: control
  property string variant: 'primary'
  property bool selected: false
  property color textColor: variant === 'primary' ? Theme.primaryForeground : selected ? Theme.primaryText : Theme.ink
  property string leadingIcon: ''
  property string trailingIcon: ''
  implicitHeight: Theme.touchSize
  implicitWidth: Math.max(Theme.touchSize, contentItem.implicitWidth + leftPadding + rightPadding)
  horizontalPadding: variant === 'primary' ? 16 : variant === 'chip' ? 12 : 0
  verticalPadding: 8
  font.pixelSize: Theme.bodySize
  font.weight: variant === 'primary' || selected ? Font.Medium : Font.Normal
  focusPolicy: Qt.StrongFocus
  Accessible.name: text
  Accessible.selected: selected
  Accessible.onPressAction: clicked()
  contentItem: RowLayout {
    spacing: Theme.space
    AppIcon {
      color: control.textColor
      name: control.leadingIcon
      visible: !!control.leadingIcon
      Layout.preferredWidth: 20
      Layout.preferredHeight: 20
    }
    AppText {
      Layout.fillWidth: true
      text: control.text
      font: control.font
      color: !control.enabled ? Theme.disabledText : control.variant === 'text' && (control.hovered || control.down) ? Theme.ink : control.textColor
      horizontalAlignment: Text.AlignHCenter
      verticalAlignment: Text.AlignVCenter
      elide: Text.ElideRight
    }
    AppIcon {
      color: control.textColor
      name: control.trailingIcon
      visible: !!control.trailingIcon
      Layout.preferredWidth: 20
      Layout.preferredHeight: 20
    }
  }
  background: Item {
    Rectangle {
      anchors.centerIn: parent
      width: parent.width
      height: control.variant === 'primary' ? parent.height : 32
      radius: control.variant === 'primary' ? 12 : 8
      color: {
        if (control.variant === 'primary')
          return !control.enabled ? Theme.disabled : control.down ? Theme.primaryPressed : Theme.primary
        if (control.down)
          return Theme.primarySoftPressed
        if ((control.variant === 'chip' && control.selected) || control.hovered)
          return Theme.primaryLight
        return 'transparent'
      }
    }
    Rectangle {
      anchors.bottom: parent.bottom
      anchors.horizontalCenter: parent.horizontalCenter
      width: parent.width - 24
      height: 2
      color: Theme.primary
      visible: control.visualFocus
    }
  }
}
