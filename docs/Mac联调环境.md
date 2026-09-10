# Mac 联调环境

部署日期：2026-09-10。服务端基于 PR #15 合并提交 `e957212`。

## 使用

- 服务及运营大屏：<https://www.u910784.nyat.app:40004>
- 健康检查：<https://www.u910784.nyat.app:40004/api/health>
- [Android APK 下载](https://www.u910784.nyat.app:40004/charging-mac-arm64.apk)：ARM64、Android 6.0 及以上，使用上述 HTTPS 地址，无需修改手机配置。Windows 副本为 `D:\下载\智充出行-mac-arm64-修复版.apk`。
- 用户演示账号：`13800000001`，手机号模拟登录；管理员：`admin / 123456`。
- 桌面端设置环境变量 `CHARGING_SERVER_URL=https://www.u910784.nyat.app:40004`。

Node 网关监听 Mac 的 80 端口，由已有 Sakura 隧道提供 HTTPS；业务请求转发至 `127.0.0.1:18080` 的 Qt 服务。APK 由网关流式下载，避免业务服务的静态文件大小限制。服务使用演示数据、模拟支付与模拟充电。Mac 必须保持联网，Sakura 隧道须保持运行。

## Mac 上的文件与进程

- 源码、构建：`/Users/charlie/bydxxq-integration/source`
- SQLite 数据与预测结果：`/Users/charlie/bydxxq-integration/data`
- 日志与测试结果：`/Users/charlie/bydxxq-integration/logs`
- 启动项：`~/Library/LaunchAgents/com.charlie.bydxxq.server.plist`
- 下载及转发网关：`/Users/charlie/bydxxq-integration/mac-gateway.mjs`，启动项 `~/Library/LaunchAgents/com.charlie.bydxxq.gateway.plist`

launchd 在用户登录后启动服务，并在进程退出后重启。数据库独立于构建目录保存。原来的 `com.charlie.qr-relay` 已停止并禁用自动启动，防止重新登录后争抢 80 端口。

Windows PowerShell 可用 `ssh mac` 登录，再运行：

```sh
# 重启平台
launchctl kickstart -k gui/$(id -u)/com.charlie.bydxxq.server
launchctl kickstart -k gui/$(id -u)/com.charlie.bydxxq.gateway

# 查看服务日志
tail -n 50 ~/bydxxq-integration/logs/server.log

# 停止平台并恢复原二维码服务
launchctl bootout gui/$(id -u) ~/Library/LaunchAgents/com.charlie.bydxxq.server.plist
launchctl disable gui/$(id -u)/com.charlie.bydxxq.server
launchctl bootout gui/$(id -u) ~/Library/LaunchAgents/com.charlie.bydxxq.gateway.plist
launchctl disable gui/$(id -u)/com.charlie.bydxxq.gateway
launchctl enable gui/$(id -u)/com.charlie.qr-relay
launchctl bootstrap gui/$(id -u) ~/Library/LaunchAgents/com.charlie.qr-relay.plist
```

## Android 构建

构建提供两个 CMake 参数：`CHARGING_DEFAULT_SERVER_URL` 指定客户端默认地址；`CHARGING_ANDROID_OPENSSL_ROOT` 指向 [KDAB android_openssl](https://github.com/KDAB/android_openssl)，将 HTTPS 所需动态库打入 APK。运行时 `CHARGING_SERVER_URL` 仍优先于编译默认值。

本次工具链：Qt 6.7.3、NDK r26b、SDK/build-tools 34、JDK 17；OpenSSL 集成使用 `b71f1470962019bd89534a2919f5925f93bc5779`。

在当前 Linux 构建机的工作目录执行：

```sh
export JAVA_HOME=/home/charlie/android-toolchain/jdk
export ANDROID_SDK_ROOT=/home/charlie/android-toolchain/sdk
export ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk/26.1.10909125
export PATH="$JAVA_HOME/bin:$PATH"
/home/charlie/android-toolchain/Qt/6.7.3/android_arm64_v8a/bin/qt-cmake \
  -S . -B build/android-ndk26 -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SERVER=OFF -DBUILD_ADMIN_APP=OFF -DBUILD_TESTING=OFF \
  -DANDROID_SDK_ROOT="$ANDROID_SDK_ROOT" \
  -DANDROID_NDK_ROOT="$ANDROID_NDK_ROOT" \
  -DQT_HOST_PATH=/home/charlie/android-toolchain/Qt/6.7.3/gcc_64 \
  -DCHARGING_DEFAULT_SERVER_URL=https://www.u910784.nyat.app:40004 \
  -DCHARGING_ANDROID_OPENSSL_ROOT=/home/charlie/android-toolchain/android_openssl
cmake --build build/android-ndk26 --target apk -j 4
```

Release 构建产物需签名后安装。本次使用本机 `/home/charlie/android-toolchain/signing/integration.jks` 的 `integration` 测试签名，先用 build-tools 34 的 `zipalign -p 4` 对齐，再用 `apksigner sign` 签名。后续测试包使用同一签名以便覆盖安装；若手机上已安装其他签名的旧包，需先卸载旧包。

交付 APK：34 MB，包名 `com.chargingplatform.user`，应用列表名称 `智充出行`，版本 `1.0.3`（4）。SHA-256：`eb21a93aea7820ac00286838b232e811a8185f253c26835e8b1662bfde698d6d`。

## 已验证

Mac 上执行服务和预测测试：28 通过、2 跳过。公网 HTTPS 健康检查、用户登录、站点查询、管理员概览均通过；两个独立登录会话可读取同一笔模拟充值后的余额。运营大屏和自动预测均可读取五个站点。APK 通过 v1/v2/v3 签名验证，包内包含预期服务地址、OpenSSL 3 与 Qt TLS 插件。

首版 r25c APK 在 Android 模拟器上复现启动崩溃：`libQt6Core` 缺少 `std::pmr::monotonic_buffer_resource` 符号。改用 NDK r26b 重建，并在 CMake 阶段拒绝 r25 及更旧的 NDK。真机为 vivo X200 Pro / Android 16，用户远程操作 Windows；Android 运行验证使用 Mac 上的 ARM64 模拟器。


## 1.0.2 移动端适配

登录页采用简洁品牌区和圆角表单，输入框没有聚焦下划线，键盘弹出时缩短品牌区；统一自绘控件使用 Basic 样式，避免 Android Material 内边距挤压文字和刷新图标。底栏图标与文字整体垂直居中，站点等长文字允许换行。登录凭据保存在应用私有目录，重新启动自动校验并恢复；退出登录或凭据失效时清除。系统返回优先回到上一页或首页，首页两秒内再次返回退出。

Android 导航原先仅发出未连接的信号，现直接调用[高德官方导航 URI](https://lbs.amap.com/api/amap-mobile/guide/android/route)，传递真实站点 GCJ-02 坐标，进入驾车路线确认页后可开始导航，起点由地图 App 定位或用户手动选择；未安装高德时打开[官方网页版路线](https://lbs.amap.com/api/uri-api/guide/travel/route)。网页版可能需要授予浏览器定位权限或手动填写起点；语音导航由高德 App 提供。

移动端自动化回归 10 项通过，包含会话恢复/失效/退出清除、返回路径、正常及窄屏完整业务流程。Mac Android 16 ARM64 模拟器实际安装、键盘展开截图、主页/个人页/底栏截图和升级恢复登录通过。截图保存在 Mac `~/bydxxq-integration/logs/v3-*.png`。

最终 APK 实测：Android 16 边缘滑动由“我的”返回首页；安装更新后保留登录。点击站点导航打开官方高德 App，确认终点为“人民广场充电站”；禁用高德 App 后正确回退 Chrome 官方路线页。模拟器的 GPS 定位未被高德接受，因此另用明确的测试起点验证高德真实算路，返回 1.6 公里 / 8 分钟及备选路线，并显示“开始导航”。测试起点仅用于模拟器验证，未写入 APK；手机起点由高德定位或用户选择。路线截图：`v3-navigation-calculated.png`。


## 1.0.3 交互及系统能力

本轮由三个子代理分别完成头像、登录注册、页面布局，主代理集成原生定位、地图选点、Toast、转场与下拉刷新。

- 头像：Android 原生 ACTION_OPEN_DOCUMENT 选择图片（仅单张 URI 授权，无全相册权限）；按 EXIF 旋转，可拖动、双指或滑块缩放裁切，再上传512×512 JPEG，后端规范化存为256×256 PNG。选择源文件限32 MB、6400万像素，防止超大图片导致内存不足。覆盖安装保留头像和登录。
- 登录：白色表单、Logo和标题300ms联动上移。新版登录先传 allowRegistration:false；不存在的手机号弹出横向“取消 / 注册并登录”二次确认，确认前后端不落库。旧客户端省略参数保持兼容。Mac后端已更新并重启。
- 系统：Android Toast替代自绘通知；Android LocationManager及运行时权限提供真实当前位置，支持近两分钟且精度300m内的位置缓存和25秒超时；定位失败保留用户选择，不伪装成预设区域。系统原始坐标和地图选点使用WGS-84，传给站点查询/高德前转换为GCJ-02。
- 地图：内置Leaflet1.9.4及OpenStreetMap真实底图，可拖动或点击、确认后返回应用。瓦片遵守系统WebView缓存，标注版权和应用User-Agent；未加载底图时不允许确认。无需地图开发者Key。坐标转换引用eviltransform提交03ba58d92dfda57f8a1635f3805483c8fc10bd77，BSD许可及Leaflet许可随APK assets/map一起打包。
- 布局：子页进出280ms左右滑动；首页、订单、个人页和站点详情支持下拉刷新并去掉顶部刷新；排序三选一胶囊保持48dp点击区域；充电桩紧凑分隔列表；空订单在可用视口居中；充值主色；退出移动到个人信息；个人页去掉重复充电记录。移除页内多余顶部间距，系统栏随明暗主题衔接并保留挖孔安全区。

验证：桌面360×800与430×860全流程共11项通过；头像独立测试包含EXIF旋转、大图缩放、裁切和32MB/64MP边界；Android16 ARM64模拟器实际系统授权、选图、拖动缩放、上传、原生Toast、地图加载/拖动/确认回传均通过。公网读取上传后的256²头像像素为(0,0,254)，与测试裁切出的蓝色区域一致。注册取消后再次登录仍要求确认；确认后已有账号正常登录。录屏逐帧检查确认登录动画有连续中间帧；截图和录屏位于Mac logs/v4-*，最终交付副本在Windows下载目录。

服务账户及注册回归：20 passed。APK v1/v2/v3 签名校验通过。

最新首页：定位与刷新移动到右上角、删除定位图标和搜索筛选外层卡片；搜索与筛选间距16dp，胶囊视觉40dp/选中32dp，触摸仍48dp；推荐电站使用独立主色浅背景卡片。最新布局11项回归再次通过。按用户最终要求，1.0.3仅交付本地 D:\下载\智充出行-1.0.3.apk，不替换公网APK（公网仍是1.0.2）。Mac上的版本文件仅用于模拟器安装验证。

## 1.0.4 个人页与键盘闪屏

个人页增加服务器全量统计：已结束充电次数、累计电量与钱包实际扣除的充电费用。待支付订单只计次数和电量，取消订单与充值不计入消费统计；统计按用户隔离且不受订单分页影响。新增可操作的深色模式开关、使用帮助、关于入口；充值按钮为主色全圆角，余额标题 18 px、余额数值 40 px。

Android 11 及以上通过系统 WindowInsets/WindowInsetsAnimation 同步 IME 高度，仅调整 QML 页面可用高度，避免 adjustResize 重建 Qt 绘图 Surface 导致黑帧。Android 6–10 由版本限定资源保留原有 adjustResize 避让。原版 Android 16 模拟器同场景录屏在 30 fps 采样中检测到 3 个黑帧（打开键盘 2 帧、关闭 1 帧）。旧版系统的避让路径保留，尚未在旧版设备实测。

APK 按用户要求仅交付 Windows 下载目录，不更新公网 APK 下载。服务器已部署真实统计字段，无数据库迁移或测试数据写入真实账户。

最终验证：Android 16 同设备同场景录屏以 30 fps 采样 250 帧，黑帧数从旧版 3 帧降为 0。模拟器登录、深色开关、帮助与关于入口通过；界面回归 11 项、后端相关回归 22 项通过。用户最后要求移除个人页顶部资料卡片背景，已仅将该背景改为透明并重新签名打包，不影响入口行为。最终 APK 为 `D:\下载\智充出行-1.0.4.apk`。

## 1.0.5 地图主题与紧凑布局

地图选点从应用 Appearance 传递当前深浅色和主色等调色板，在 WebView 首次绘制前应用，地图控件、文字、确认按钮、归属信息及系统栏保持同一主题；深色底图仅对真实 OSM 瓦片做显示配色处理。个人资料区上下内边距改为 8 px；移除推荐电站标题及其占位，推荐卡保留不同底色；订单右下改为统一右对齐的“小票 / 详情 / 处理”轻量胶囊，整卡点击仍进入原有业务页面。界面回归 11 项通过。

APK 本地交付 `D:\下载\智充出行-1.0.5.apk`，SHA-256：`86816135f196df91af5b36f708c46a4f737e15ae97cea35fcc03fc7c1de48ba4`，未更新公网下载。

Android 16 模拟器已覆盖安装并实测地图浅色/深色，截图确认地图控件、底图、顶部和底部系统栏均同步；日志未出现 Java/JNI 或脚本异常。截图副本在 `D:\下载\智充出行-1.0.5-验证`。

## 1.0.6 详细位置与大额排版

系统定位组合城市、区县、道路、门牌和地标；系统仅有粗城市字段时保留更详细的完整地址，过滤仅国家名及重复地标。没有详细地址时显示实际坐标。首页位置区最多两行显示真实名称，不再把长地址替换为“当前位置”。地址格式化边界 fixture 14 项通过。

个人页三列统计使用一致的数字行、说明行高度和按最长数字统一计算的字号，数字不换行；钱包金额根据卡片可用宽度适配字号，保留完整金额。界面回归 11 项通过。APK 版本 1.0.6，仍仅保存本地下载目录。

金额 fixture 使用截图数据 `442 / 10843.0 / 12693.18` 与余额 `1000000.00`，320、360 dp 验证三列同字号、同基线、不截断，余额界内完整显示；3 项测试通过。最终 APK SHA-256：`2c670fb121c6110fe5ef1a780d14f185cf5d27a74b69cecdf9a344ba422b4e60`。

Android 16 覆盖安装验证通过；模拟器系统未返回详细地址时，顶部显示实际 `31.2359°N · 121.4805°E`，不会回退到“中国”或“当前位置”。


## 多端共用服务及合并前验证

管理端编译默认地址与 Web 开发代理统一为当前 HTTPS 入口，仍可通过环境变量覆盖；管理端登录页可编辑地址。Web 生产构建保持同源 API。修复 Vite 拒绝共享字体路径，以及 TLS 代理后 Qt 服务错误拒绝同 Host 的 HTTPS Origin 导致的大屏白屏；跨主机或不同端口 Origin 仍拒绝。

本地完整 CTest 5/5、格式/静态检查通过；新增 HTTPS 来源边界回归通过；管理端 20 项及 Web 开发代理真实数据浏览器测试通过。另补 Android 6 地图回调 API 兼容性，重新签名的本地 1.0.6 APK SHA-256 为 `41054a73b42a15642e0a75071680874272625b9a627f6e628137746f42cde7e9`。
