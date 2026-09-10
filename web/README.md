# 充电运营指挥中心

Vue 3 + TypeScript + ECharts，1920×1080 设计尺寸等比适配，1366×768 无页面滚动。
大屏读取统一 C++ 服务 `/api/dashboard`，每 5 秒刷新。连接失败保留最近快照并显示
中断提示，统计快照超过 30 秒标记过期，预测超过 1 小时标记过期。

从仓库根目录执行：

```sh
pnpm --dir web install --frozen-lockfile
pnpm --dir web build
```

统一后端直接提供 `web/dist`，生产页面通过同源 `/api/dashboard` 获取数据。
当前联调入口为 https://www.u910784.nyat.app:40004，与 APK 共用 Mac 上的服务和数据库。

开发时运行 `pnpm --dir web dev`，Vite 默认将 `/api` 代理到上述 HTTPS 入口，并保留证书校验，
无需启动本地后端或配置浏览器跨域。代理目标仅用于开发服务，不会写入生产前端包。
需要连接本地服务时可覆盖环境变量（也可写入 `web/.env.local`）：

```sh
CHARGING_API_PROXY_TARGET=http://127.0.0.1:8080 pnpm --dir web dev
```

前端不使用静态模拟数据；断线时保留最近一次真实快照并显示连接异常，首次加载前显示空状态。

```sh
pnpm --dir web lint
pnpm --dir web format:check
pnpm --dir web validate:data
python3 web/scripts/export_dashboard.py
# 其他后端端口：
pnpm --dir web validate:data http://127.0.0.1:18080/api/dashboard
```

中心空间图以站点经纬度绘制散点，点击光点或站名切换详情。四个 KPI、状态分布、排名、快慢充、营收趋势、时段热力图、预测和动态
均来自业务数据库聚合。预测方法与回测结果见[预测说明](../ml/README.md)。

第三方来源与许可见[来源说明](third-party/README.md)。
