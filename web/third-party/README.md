# 第三方来源

`src/components/vendor/BorderBox.vue` 的 SVG 边框路径与结构直接改编自
[daidaibg/IofTV-Screen-Vue3](https://github.com/daidaibg/IofTV-Screen-Vue3/blob/f1383bcf078d66329dc7fa6ca6a4e2befaf83215/src/components/datav/border-box-13/border-box-13.vue)，
提交 `f1383bcf078d66329dc7fa6ca6a4e2befaf83215`，MIT 许可见同目录完整许可文件。
修改：移除 lodash/VueUse/SCSS 依赖，使用原生 ResizeObserver，修正 stroke 绑定，
调整颜色和透明度，加入无障碍标记。项目的三栏指挥中心布局与标题装饰也参考该项目。

Vue 和 ECharts 版本由 `package.json` 与 `pnpm-lock.yaml` 管理。中央空间图按业务站点经纬度绘制散点。

界面使用华为 HarmonyOS Sans SC。原始字体和授权位于 `shared/fonts/`，大屏与 Qt 客户端共用。
