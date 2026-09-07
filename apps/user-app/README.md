# 智充出行用户端

Qt Quick 用户端运行在 Linux 桌面，通过共享 `ApiClient` 访问 C++ 服务。`QQuickWidget` 承载页面，`QWebEngineView` 加载腾讯驾车与步行导航。

支持手机号登录、个人资料与头像、模拟充值、找站与筛选、预约、充电和钱包结算。订单每页加载 30 条，支持状态筛选、下拉刷新和返回位置保留。服务端持续保存充电状态，重新登录可恢复未完成订单。

“我的 → 设置”调整主色、副色和明暗模式，偏好即时保存。`Appearance` 管理外观，`Theme.qml` 提供样式与尺寸，`RollingNumber.qml` 展示数值变化。电站和电桩的分段条显示可用、占用和异常状态。

## 运行与测试

从仓库根目录执行，安装与构建见[初始化指南](../../docs/00_初始化指南.md)：

```bash
bash scripts/run.sh server
bash scripts/run.sh user
bash scripts/test.sh -R 'mobile-'
```

`CHARGING_SERVER_URL` 指定服务地址，默认 `http://127.0.0.1:8080`。测试覆盖页面加载、完整业务流程、订单分页、主题切换和两种手机尺寸的布局。

在线地图测试需要图形会话与网络：

```bash
source scripts/env.sh
CHARGING_SERVER_URL=http://127.0.0.1:8080 CHARGING_UI_TEST_MAP=1 \
  build/full/apps/user-app/charging-user-flow-test embeddedNavigation -v1
```

`charging-user --smoke-test` 检查页面加载，`--screenshot /tmp/mobile.png` 保存截图，`--phone 13900000000 --page home` 指定演示账号与页面。设置 `CHARGING_UI_ARTIFACT_DIR=/tmp/mobile-review` 可保存测试截图。

## 素材

布局与配色见[界面设计规范](../../docs/界面设计规范.md)。品牌图见 [brand.svg](assets/brand.svg)，插图见[素材说明](assets/illustrations/README.md)。HarmonyOS Sans SC 字体和 Lucide 图标的许可随源码保留。
