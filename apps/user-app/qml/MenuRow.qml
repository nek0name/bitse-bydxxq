import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
  id: control
  property string title: ''
  property string description: ''
  property string iconName: ''
  implicitHeight: description ? 80 : 64
  padding: Theme.cardPadding
  Accessible.name: title + (description ? '，' + description : '')
  background: Rectangle {
    radius: Theme.cardRadius
    color: control.down || control.visualFocus ? Theme.primaryLight : Theme.card
  }
  contentItem: RowLayout {
    spacing: Theme.cardPadding
    AppIcon {
      name: control.iconName
      Layout.preferredWidth: 24
      Layout.preferredHeight: 24
    }
    Column {
      Layout.fillWidth: true
      spacing: Theme.microSpace
      AppText {
        width: parent.width
        text: control.title
        font.pixelSize: Theme.bodyLargeSize
        font.weight: Font.Medium
        elide: Text.ElideRight
      }
      AppText {
        width: parent.width
        text: control.description
        visible: !!control.description
        font.pixelSize: Theme.labelSize
        color: Theme.muted
        elide: Text.ElideRight
      }
    }
    AppIcon {
      name: 'chevron-right'
      Layout.preferredWidth: 24
      Layout.preferredHeight: 24
    }
  }
}
