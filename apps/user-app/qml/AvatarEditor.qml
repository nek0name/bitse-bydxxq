import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Charging.Native 1.0

Popup {
  id: editor
  parent: Overlay.overlay
  anchors.centerIn: parent
  width: parent ? parent.width : 360
  height: parent ? parent.height : 640
  padding: Theme.pagePadding
  modal: true
  focus: true
  closePolicy: Popup.CloseOnEscape
  property real zoom: 1
  function choose() { picker.open() }
  function resetCrop() {
    zoom = 1
    photo.x = (viewport.width - photo.width) / 2
    photo.y = (viewport.height - photo.height) / 2
  }
  function constrain() {
    photo.x = Math.max(viewport.width - photo.width, Math.min(0, photo.x))
    photo.y = Math.max(viewport.height - photo.height, Math.min(0, photo.y))
  }
  function changeZoom(value) {
    var oldWidth = photo.width
    var oldHeight = photo.height
    var centerX = (viewport.width / 2 - photo.x) / oldWidth
    var centerY = (viewport.height / 2 - photo.y) / oldHeight
    zoom = Math.max(1, Math.min(4, value))
    photo.x = viewport.width / 2 - centerX * photo.width
    photo.y = viewport.height / 2 - centerY * photo.height
    constrain()
  }
  onClosed: imageData.clear()
  background: Rectangle { color: Theme.paper }
  AvatarImage {
    id: imageData
    onFailed: function(message) { mobile.notification(message) }
  }
  FileDialog {
    id: picker
    title: '选择头像'
    nameFilters: ['图片 (*.jpg *.jpeg *.png *.webp)']
    onAccepted: {
      if (imageData.load(selectedFile)) {
        editor.open()
        Qt.callLater(editor.resetCrop)
      }
    }
  }
  contentItem: ColumnLayout {
    spacing: 20
    RowLayout {
      Layout.fillWidth: true
      IconButton {
        iconName: 'x'
        Accessible.name: '取消裁切'
        onClicked: editor.close()
      }
      AppText {
        text: '裁切头像'
        Layout.fillWidth: true
        font.pixelSize: Theme.titleSize
        font.weight: Font.DemiBold
      }
      ActionButton {
        text: '重选'
        variant: 'text'
        onClicked: picker.open()
      }
    }
    Item { Layout.fillHeight: true }
    Item {
      id: viewport
      Layout.alignment: Qt.AlignHCenter
      Layout.preferredWidth: Math.min(editor.availableWidth, editor.availableHeight - 240)
      Layout.preferredHeight: width
      clip: true
      Image {
        id: photo
        source: imageData.preview
        width: imageData.imageWidth * Math.max(viewport.width / Math.max(1, imageData.imageWidth), viewport.height / Math.max(1, imageData.imageHeight)) * editor.zoom
        height: imageData.imageHeight * Math.max(viewport.width / Math.max(1, imageData.imageWidth), viewport.height / Math.max(1, imageData.imageHeight)) * editor.zoom
        fillMode: Image.Stretch
        smooth: true
      }
      PinchArea {
        anchors.fill: parent
        property real initialZoom: 1
        onPinchStarted: initialZoom = editor.zoom
        onPinchUpdated: function(pinch) { editor.changeZoom(initialZoom * pinch.scale) }
        MouseArea {
          anchors.fill: parent
          drag.target: photo
          drag.minimumX: viewport.width - photo.width
          drag.maximumX: 0
          drag.minimumY: viewport.height - photo.height
          drag.maximumY: 0
        }
      }
      Rectangle {
        anchors.fill: parent
        color: 'transparent'
        border.color: '#ffffff'
        border.width: 2
      }
      Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: 'transparent'
        border.color: '#aaffffff'
        border.width: 1
      }
    }
    AppText {
      text: '拖动调整位置，双指缩放'
      Layout.fillWidth: true
      horizontalAlignment: Text.AlignHCenter
      color: Theme.muted
    }
    Slider {
      objectName: 'avatarZoomSlider'
      Layout.fillWidth: true
      from: 1
      to: 4
      value: editor.zoom
      Accessible.name: '头像缩放'
      onMoved: editor.changeZoom(value)
    }
    Item { Layout.fillHeight: true }
    RowLayout {
      Layout.fillWidth: true
      spacing: 12
      ActionButton {
        text: '取消'
        variant: 'text'
        Layout.fillWidth: true
        onClicked: editor.close()
      }
      ActionButton {
        objectName: 'confirmAvatarButton'
        text: '使用头像'
        Layout.fillWidth: true
        enabled: !mobile.busy && imageData.imageWidth > 0
        onClicked: {
          var encoded = imageData.crop(-photo.x / photo.width, -photo.y / photo.height, viewport.width / photo.width)
          if (encoded.length) {
            mobile.uploadAvatar(encoded)
            editor.close()
          }
        }
      }
    }
  }
}
