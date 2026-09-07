import { createApp } from 'vue'
import App from './App.vue'
import './style.css'

Promise.all(
  [400, 500, 700].map((weight) => document.fonts.load(`${weight} 14px "HarmonyOS Sans SC"`)),
).then(() => createApp(App).mount('#app'))
