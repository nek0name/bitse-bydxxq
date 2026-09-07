import QtQuick
import QtQuick.Controls

Flickable {
  objectName: 'settingsPage'
  contentWidth: width
  contentHeight: content.height + Theme.pagePadding * 2
  clip: true
  boundsBehavior: Flickable.StopAtBounds
  ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
  Column {
    id: content
    x: Theme.pagePadding
    y: Theme.pagePadding
    width: parent.width - Theme.pagePadding * 2
    spacing: Theme.cardPadding
    Rectangle {
      width: parent.width
      height: modes.height + Theme.cardPadding * 2
      radius: Theme.cardRadius
      color: Theme.card
      Column {
        id: modes
        x: Theme.cardPadding
        y: Theme.cardPadding
        width: parent.width - Theme.cardPadding * 2
        spacing: Theme.space
        AppText { text: '外观'; font.weight: Font.Medium }
        Row {
          width: parent.width
          spacing: Theme.space
          Repeater {
            model: [{key: 'system', label: '跟随系统'}, {key: 'light', label: '浅色'}, {key: 'dark', label: '深色'}]
            delegate: ActionButton {
              required property var modelData
              objectName: 'appearanceMode_' + modelData.key
              width: (modes.width - Theme.space * 2) / 3
              text: modelData.label
              variant: 'chip'
              selected: appearance.mode === modelData.key
              onClicked: appearance.mode = modelData.key
            }
          }
        }
      }
    }
    Repeater {
      model: ['primary', 'secondary']
      delegate: Rectangle {
        id: group
        required property string modelData
        width: content.width
        height: choices.height + Theme.cardPadding * 2
        radius: Theme.cardRadius
        color: Theme.card
        Column {
          id: choices
          x: Theme.cardPadding
          y: Theme.cardPadding
          width: parent.width - Theme.cardPadding * 2
          spacing: Theme.space
          AppText {
            text: group.modelData === 'primary' ? '主色' : '副色'
            font.weight: Font.Medium
          }
          Row {
            width: parent.width
            Repeater {
              model: appearance.swatches
              delegate: Button {
                id: swatch
                required property string modelData
                required property int index
                objectName: group.modelData + 'Color_' + index
                width: choices.width / 6
                height: Theme.touchSize
                readonly property bool selected: Qt.colorEqual(group.modelData === 'primary' ? appearance.primaryColor : appearance.secondaryColor, modelData)
                Accessible.name: (group.modelData === 'primary' ? '主色 ' : '副色 ') + modelData
                Accessible.selected: selected
                onClicked: {
                  if (group.modelData === 'primary') appearance.primaryColor = modelData
                  else appearance.secondaryColor = modelData
                }
                background: Item {
                  Rectangle {
                    anchors.centerIn: parent
                    width: 38
                    height: 38
                    radius: 19
                    color: 'transparent'
                    border.width: 1
                    border.color: Theme.ink
                    visible: swatch.selected || swatch.visualFocus
                  }
                  Rectangle {
                    anchors.centerIn: parent
                    width: 28
                    height: 28
                    radius: 14
                    color: swatch.modelData
                  }
                }
                contentItem: Item {}
              }
            }
          }
        }
      }
    }
  }
}
