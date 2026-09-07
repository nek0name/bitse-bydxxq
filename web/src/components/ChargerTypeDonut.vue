<script setup lang="ts">
import { useChart } from '../composables/useChart'
import type { ChargerTypeRatioItem } from '../types/dashboard'

const props = defineProps<{ data: ChargerTypeRatioItem[] }>()

const chartElement = useChart(() => {
  return {
    tooltip: { trigger: 'item', formatter: '{b}<br/><b>{c}</b> 台（{d}%）' },
    legend: {
      bottom: 0,
      left: 'center',
      icon: 'circle',
      itemWidth: 8,
      itemHeight: 8,
      textStyle: { color: '#AAA8B8', fontSize: 11 },
    },
    series: [
      {
        type: 'pie',
        radius: ['50%', '72%'],
        center: ['50%', '43%'],
        label: { show: false },
        itemStyle: { borderColor: '#23242F', borderWidth: 3 },
        data: props.data.map((item) => ({
          name: item.label,
          value: item.count,
          itemStyle: { color: item.type === 'dc' ? '#857AD6' : '#C8C3EB' },
        })),
      },
    ],
  }
})
</script>

<template><div ref="chartElement" class="chart" /></template>
