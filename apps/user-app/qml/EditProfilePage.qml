import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Loader {
  id: root
  objectName: 'editProfilePage'
  function chooseAvatar() {
    if (Qt.platform.os === 'android' && avatarEditor.item) avatarEditor.item.choose()
    else mobile.chooseAvatar()
  }
  Loader {
    id: avatarEditor
    source: Qt.platform.os === 'android' ? 'AvatarEditor.qml' : ''
  }
  active: mobile.signedIn
  sourceComponent: Flickable {
    id: screen
    contentWidth: width
    contentHeight: content.height + Theme.pagePadding * 2
    boundsBehavior: Flickable.StopAtBounds
    clip: true
    ScrollBar.vertical: ScrollBar {
      policy: ScrollBar.AsNeeded
    }
    Column {
      id: content
      x: Theme.pagePadding
      y: 0
      width: parent.width - Theme.pagePadding * 2
      spacing: Theme.sectionSpace
      Column {
        width: parent.width
        spacing: Theme.space
        Button {
          id: avatarButton
          objectName: 'chooseAvatarButton'
          anchors.horizontalCenter: parent.horizontalCenter
          width: 96
          height: 96
          padding: 12
          enabled: !mobile.busy
          Accessible.name: '更换头像'
          onClicked: root.chooseAvatar()
          background: Rectangle {
            radius: Theme.heroRadius
            color: avatarButton.down || avatarButton.visualFocus ? Theme.primarySoftPressed : Theme.disabled
          }
          contentItem: Image {
            source: mobile.avatarSource || appearance.iconSource(':/icons/user.svg', Theme.ink)
            sourceSize.width: 144
            sourceSize.height: 144
            fillMode: Image.PreserveAspectFit
            opacity: mobile.avatarSource ? 1 : 0.5
          }
        }
        ActionButton {
          anchors.horizontalCenter: parent.horizontalCenter
          text: '更换头像'
          variant: 'text'
          enabled: !mobile.busy
          onClicked: root.chooseAvatar()
        }
        AppText {
          width: parent.width
          text: '选择照片后可调整头像范围'
          horizontalAlignment: Text.AlignHCenter
          color: Theme.muted
          font.pixelSize: Theme.labelSize
        }
      }
      Column {
        width: parent.width
        spacing: Theme.space
        AppText {
          text: '昵称'
          font.pixelSize: Theme.bodyLargeSize
          font.weight: Font.Medium
        }
        AppField {
          id: nickname
          objectName: 'nicknameInput'
          width: parent.width
          Component.onCompleted: text = mobile.user.nickname
          maximumLength: 24
          placeholderText: '1 至 24 个字符'
          Accessible.name: '昵称'
          onAccepted: mobile.updateNickname(text)
        }
      }
      Column {
        width: parent.width
        spacing: Theme.space
        AppText {
          text: '手机号'
          font.pixelSize: Theme.bodyLargeSize
          font.weight: Font.Medium
        }
        AppField {
          width: parent.width
          text: mobile.user.phone
          readOnly: true
          color: Theme.muted
          Accessible.name: '登录手机号，不可修改'
        }
      }
      ActionButton {
        objectName: 'saveProfileButton'
        width: parent.width
        text: '保存修改'
        enabled: !mobile.busy
        onClicked: mobile.updateNickname(nickname.text)
      }
      ActionButton {
        objectName: 'logoutButton'
        width: parent.width
        text: '退出登录'
        variant: 'text'
        textColor: Theme.danger
        enabled: !mobile.busy
        onClicked: logoutConfirm.open()
      }
    }
    Popup {
      id: logoutConfirm
      parent: Overlay.overlay
      anchors.centerIn: parent
      width: Math.min(400, screen.width - Theme.pagePadding * 2)
      padding: Theme.cardPadding
      modal: true
      background: Rectangle { color: Theme.card; radius: Theme.heroRadius }
      Overlay.modal: Rectangle { color: Theme.overlay }
      contentItem: ColumnLayout {
        spacing: Theme.cardPadding
        AppText {
          text: '退出当前账号？'
          font.pixelSize: Theme.titleSize
          font.weight: Font.DemiBold
          Layout.fillWidth: true
        }
        AppText {
          Layout.fillWidth: true
          visible: mobile.activeOrder.id !== undefined
          text: '退出后充电仍会计费，预约仍保留。'
          color: Theme.muted
          wrapMode: Text.Wrap
        }
        RowLayout {
          Layout.fillWidth: true
          spacing: 12
          ActionButton {
            Layout.fillWidth: true
            text: '取消'
            variant: 'text'
            onClicked: logoutConfirm.close()
          }
          ActionButton {
            objectName: 'confirmLogoutButton'
            Layout.fillWidth: true
            text: '确认退出'
            onClicked: {
              logoutConfirm.close()
              mobile.logout()
            }
          }
        }
      }
    }
  }
}
