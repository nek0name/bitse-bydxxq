import { fileURLToPath } from 'node:url'
import { defineConfig, loadEnv } from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '')
  const apiTarget = env.CHARGING_API_PROXY_TARGET || 'https://www.u910784.nyat.app:40004'

  return {
    plugins: [vue()],
    optimizeDeps: {
      // ECharts has many internal entry points; excluding it keeps the
      // development server from pre-bundling the whole chart library.
      exclude: ['echarts'],
    },
    server: {
      host: '0.0.0.0',
      port: 5173,
      fs: { allow: [process.cwd(), fileURLToPath(new URL('../shared/fonts', import.meta.url))] },
      proxy: { '/api': { target: apiTarget, changeOrigin: true } },
    },
  }
})
