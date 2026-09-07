# 充电运营指挥中心

Vue 3 + TypeScript + ECharts，1920×1080 设计尺寸等比适配，1366×768 无页面滚动。
大屏读取统一 C++ 服务 `/api/dashboard`，每 5 秒刷新。连接失败保留最近快照并显示
中断提示，统计快照超过 30 秒标记过期，预测超过 1 小时标记过期。

从仓库根目录执行：

```sh
pnpm --dir web install --frozen-lockfile
pnpm --dir web build
```

统一后端直接提供 `web/dist`。开发时先启动默认 8080 后端，再运行
`pnpm --dir web dev`，Vite 会代理 `/api` 到后端，无需另外配置跨域。

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
